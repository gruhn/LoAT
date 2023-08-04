#include "clause.hpp"
#include "expr.hpp"
#include <stdexcept>

/**
 * TODO docs
 */
const std::optional<Var> varAt(const Var &var, const Subs &subs) {
    if (!subs.contains(var)) {
        return {};
    } else if (std::holds_alternative<NumVar>(var)) {
        const NumVar num_var = std::get<NumVar>(var);
        const Expr num_expr = subs.get<IntTheory>().get(num_var);

        if (num_expr.isVar()) {
            return num_expr.toVar();
        } else {
            throw std::logic_error("Substitution contains non-variable expression");
        }
    } else if (std::holds_alternative<BoolVar>(var)) {
        const BoolVar bool_var = std::get<BoolVar>(var);
        const BoolExpr bool_expr = subs.get<BoolTheory>().get(bool_var);

        // TODO: is there a simpler way to restore a BoolVar from a BoolExpr?
        const auto bool_expr_vars = bool_expr->vars();
        if (bool_expr_vars.size() == 1) {
            const BoolVar target_var = std::get<BoolVar>(*bool_expr_vars.begin());
            return target_var;
        } else {
            throw std::logic_error("Substitution contains non-variable expression");
        }
    } else {
        throw std::logic_error("unsupported theory");
    }
}

/**
 * Tries to compute a unifier for the two given predicates. Because predicate
 * arguments are always simple variables (instead of expressions), the unifier
 * is simply a variable renaming. If the predicates don't have the same
 * predicate symbol or different arity or the argument types don't match,
 * unification is not possible. For example:
 *
 *    F(x,x) and G(y,z) are not unifiable
 *    F(x)   and F(y,z) are not unifiable
 * 	  F(w,x) and F(y,z) returns { w -> y, x -> z }
 *
 * In practice we normalize all predicates to have the same arity with same argument 
 * types in the same order. So only differing predicate symbols should be a reason
 * for two predicates to not be unifiable. But for "local" correctness we also check
 * arity and argument types.
 */
const std::optional<Subs> computeUnifier(const FunApp &pred1, const FunApp &pred2) {
    if (pred1.loc == pred2.loc && pred1.args.size() == pred2.args.size()) {
        Subs subs;

        size_t size = pred1.args.size();
        for (size_t i = 0; i < size; ++i) {
            const auto var1 = pred1.args[i];
            const auto var2 = pred2.args[i];

            // QUESTION: I suspect some C++ template magic can make this prettier
            bool both_nums = std::holds_alternative<NumVar>(var1) &&
                           std::holds_alternative<NumVar>(var2);
            bool both_bools = std::holds_alternative<BoolVar>(var1) &&
                            std::holds_alternative<BoolVar>(var2);

            if (both_nums) {
                subs.put(
                std::make_pair(std::get<NumVar>(var1), std::get<NumVar>(var2)));
            } else if (both_bools) {
                subs.put(std::make_pair(
                std::get<BoolVar>(var1),
                BExpression::buildTheoryLit(std::get<BoolVar>(var2))));
            } else {
                // argument types don't match ==> not unifiable
                return {};
            }
        }

        return subs;
    } else {
        return {};
    }
}

/**
 * Apply `renaming` to all arguments of the predicate. 
 * Will throw an error if the renaming maps to compound expressions 
 * instead of variables.
 */
const FunApp FunApp::renameWith(const Subs &renaming) const {
  std::vector<Var> args_renamed;

  for (const Var &var : args) {
    const auto target_var = varAt(var, renaming);

    if (target_var.has_value()) {
      args_renamed.push_back(target_var.value());
    } else {
      args_renamed.push_back(var);
    }
  }

  return FunApp(loc, args_renamed);
}

/**
 * Constructor for a Constrained Horn Clause (CHC). For example:
 * 
 *    F(x1) /\ G(x2,x3) /\ (x1 < x2 /\ x2 < x3)   ==>   H(x1,x2,x3)
 *    
 *    ^^^^^^^^^^^^^^^^^    ^^^^^^^^^^^^^^^^^^^^         ^^^^^^^^^^^
 *           lhs                  guard                     rhs
 *
 * @param lhs - a set of predicates on the left-hand-side (LHS) of the implication.
 * @param rhs - a single predicate on the right-hand-side (RHS) of the implication.
 * @param guard - a boolean expression describing the clause constraint.
 */
Clause::Clause(const std::set<FunApp> &lhs, const FunApp &rhs, const BoolExpr &guard) 
    : lhs(lhs), rhs(rhs), guard(guard) {}

/**
 * Apply `renaming` to all variables in the clause, i.e variables in the guard and the 
 * arguments of all predicates on both LHS and RHS. Will throw an error if the renaming 
 * maps to compound expressions 
 */
const Clause Clause::renameWith(const Subs &renaming) const {
  std::set<FunApp> lhs_renamed;
  for (const FunApp &pred : lhs) {
    lhs_renamed.insert(pred.renameWith(renaming));
  }

  const FunApp rhs_renamed = rhs.renameWith(renaming);
  const auto guard_renamed = guard->subs(renaming);
  return Clause(lhs_renamed, rhs_renamed, guard_renamed);
}

/**
 * Compute resolution of `fst` and `snd` using the RHS of `fst` and `pred`,
 * which is assumed to be on the LHS of `snd`. So the caller is responsible for
 * choosing the literal to do resolution with. If `pred` is not on the LHS of
 * `snd` we throw an error.
 *
 * For example, with
 *
 * 	 fst  : F(x) ==> G(x)
 * 	 snd  : G(y) /\ G(z) /\ y < z ==> H(y,z)
 *   pred : G(z)
 *
 * the returned resolvent should be
 *
 * 	 G(y) /\ F(z) /\ y < z ==> H(y,z)
 *
 */
const std::optional<Clause> resolutionWith(const Clause &fst, const Clause &snd, const FunApp &pred) {
    if (!snd.lhs.contains(pred)) {
        throw std::logic_error("Given `pred` is not on the LHS of `snd`");
    }

    // TODO: make sure variables names are disjoint in both predicates

    std::set<FunApp> snd_lhs_without_pred = snd.lhs;
    snd_lhs_without_pred.erase(pred);

    const auto unifier = computeUnifier(fst.rhs, pred);

    // If the predicates are not unifiable, we don't throw an error but return
    // nullopt. That way the caller can filter out unifiable predicates using this
    // function. We could throw an error here too, but that implies that we expect
    // the caller to check unifiablility first and implementing that check would
    // be redundant. On the other hand, choosing a literal from the LHS of `snd`
    // is trivial, so we make require the caller to do that correctly.
    if (!unifier.has_value()) {
        return {};
    }

    const auto fst_renamed = fst.renameWith(unifier.value());

    // LHS of resolvent is the union of the renamed LHS of `fst` ...
    std::set<FunApp> resolvent_lhs = fst_renamed.lhs;
    // ... and the LHS of `snd` where `pred` is removed.
    resolvent_lhs.insert(snd_lhs_without_pred.begin(), snd_lhs_without_pred.end());

    return Clause(
        resolvent_lhs, 
        snd.rhs, 
        fst_renamed.guard & snd.guard
    );
}

/**
 * Returns true iff the clause has at most one LHS predicate.
 */
bool Clause::isLinear() const {
    return lhs.size() <= 1;
}

bool operator<(const FunApp &fun1, const FunApp &fun2) {
    if (fun1.loc < fun2.loc) {
        return true;
    } else if (fun2.loc < fun1.loc) {
        return false;
    } else {
        return fun1.args < fun2.args;
    }
}

std::ostream &operator<<(std::ostream &s, const FunApp &fun) {
    s << "F" << fun.loc;

    if (fun.args.size() > 0) {
        auto iter = fun.args.begin();

        s << "(" << *iter;
        iter++;

        while (iter < fun.args.end()) {
            s << "," << *iter;
            iter++;
        }

        s << ")";
    }

    return s;
}

std::ostream &operator<<(std::ostream &s, const Clause &chc) {
    for (const FunApp &fun_app : chc.lhs) {
        s << fun_app << " /\\ ";
    }

    s << chc.guard << " ==> " << chc.rhs;

    return s;
}
