#include "clause.hpp"
#include "boolexpr.hpp"
#include "expr.hpp"
#include "smtfactory.hpp"
#include "theory.hpp"
#include <memory>
#include <stdexcept>
#include <tuple>
#include <utility>

/**
 * TODO docs
 */
const Var varAt(const Var &var, const Subs &subs) {  
    if (!subs.contains(var)) {
        return var;
    } else {
        const auto optional_var = expr::toVar(subs.get(var));

        if (optional_var.has_value()) {
            return optional_var.value();
        } else {
            throw std::logic_error("Renaming contains non-variable expression");
        }
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
 * 	  F(x,x) and F(y,z) returns { x -> y, z -> y }
 *
 * In practice we normalize all predicates to have the same arity with same argument 
 * types in the same order. So only differing predicate symbols should be a reason
 * for two predicates to not be unifiable. But this is a non-local assumption (that might 
 * change) so for "local correctness" we also check arity and argument types.
 */
const std::optional<Subs> computeUnifier(const FunApp &pred1, const FunApp &pred2) {
    if (pred1.name == pred2.name) {
        return computeUnifier(pred1.args, pred2.args);
    } else {
        return {};
    }
}
const std::optional<Subs> computeUnifier(const std::vector<Var> &args1, const std::vector<Var> &args2) {
    if (args1.size() == args2.size()) {
        Subs subs;

        for (size_t i = 0; i < args1.size(); ++i) {
            const Var var1 = args1[i];
            const Var var2 = args2[i];

            if (subs.contains(var1)) {
                // If `var1` is already contained in `subs`, then `pred1` must 
                // have `var1` multiple times as an argument. This is an implict
                // equality. We assume at this point that this doesn't occur and 
                // that all equalities are explicitly mentioned in the constraint.                
                std::cout << FunApp("P1", args1) << std::endl;
                std::cout << FunApp("P2", args2) << std::endl;
                throw std::logic_error("computeUnifier: predicate has duplicate arguments");
            } else {
                // QUESTION: I suspect some C++ template magic can make this prettier
                bool both_nums = std::holds_alternative<NumVar>(var1) &&
                               std::holds_alternative<NumVar>(var2);
                bool both_bools = std::holds_alternative<BoolVar>(var1) &&
                                std::holds_alternative<BoolVar>(var2);

                if (both_nums) {
                    subs.put(std::make_pair(
                        std::get<NumVar>(var1), 
                        std::get<Expr>(expr::toExpr(var2))
                    ));
                } else if (both_bools) {
                    subs.put(std::make_pair(
                        std::get<BoolVar>(var1),
                        std::get<BoolExpr>(expr::toExpr(var2))
                    ));
                } else {
                    // argument types don't match ==> not unifiable
                    return {};
                }
            }
        }

        return subs;
    } else {
        return {};
    }
}

/**
 * TODO
 */
const std::optional<Subs> computeUnifier(const Clause &chc1, const Clause &chc2) {
    Subs unifier;

    if (chc1.rhs.has_value() && chc2.rhs.has_value()) {
        const auto &rhs_unifier = computeUnifier(chc1.rhs.value(), chc2.rhs.value());

        if (rhs_unifier.has_value()) {
            unifier = rhs_unifier.value(); // RHS unifiable
        } else {
            return {}; // RHS not unifiable ==> entire clauses not unifiable
        }
    }

    std::list<FunApp> lhs_preds2(chc2.lhs.begin(), chc2.lhs.end());

    for (const auto& pred1: chc1.lhs) {
        auto it = lhs_preds2.begin();
        while (true) {
            if (it == lhs_preds2.end()) {
                return {}; // no partner for `pred1` in `chc2.lhs`
            }

            auto &pred_unifier = computeUnifier(pred1, *it);
            if (pred_unifier.has_value()) {
                unifier = expr::concat(unifier, pred_unifier.value());
                lhs_preds2.erase(it);
                break;
            }

            it++;
        }
    }

    if (!lhs_preds2.empty()) {
        return {}; // `chc1.lhs` has fewer LHS predicates than `chc2.lhs`
    }

    return unifier;
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
    args_renamed.push_back(target_var);
  }

  return FunApp(name, args_renamed);
}

/**
 * Create a copy of predicate with argument vector replaced.
 */
const FunApp FunApp::withArgs(const std::vector<Var> new_args) const {
    return FunApp(name, new_args);
}

/**
 * Collect set of variables used in predicate arguments.
 */
const VarSet FunApp::vars() const {
    VarSet vs;
    for (const auto &arg: args) {
        vs.insert(arg);
    }
    return vs;
}

/**
 * Count number arguments with type integer valued arguments.
 */
unsigned long FunApp::intArity() const {
    unsigned arity {0};
    for (const auto &arg: args) {
        if (std::holds_alternative<NumVar>(arg)) {
            arity++;
        }
    }
    return arity;
}

/**
 * Count number arguments with type integer valued arguments.
 */
unsigned long FunApp::boolArity() const {
    unsigned arity {0};
    for (const auto &arg: args) {
        if (std::holds_alternative<BoolVar>(arg)) {
            arity++;
        }
    }
    return arity;
}

/**
 * Apply `renaming` to all variables in the clause, i.e variables in the guard and the 
 * arguments of all predicates on both LHS and RHS. Will throw an error if the renaming 
 * maps to compound expressions 
 */
const Clause Clause::renameWith(const Subs &renaming) const {
    std::vector<FunApp> lhs_renamed;
    for (const FunApp &pred : lhs) {
        lhs_renamed.push_back(pred.renameWith(renaming));
    }

    // TODO: bottleneck applying large subsitution with 20_000 vars
    if (renaming.size() > 15000) {
        std::cout << "subs size: " << renaming.size() << std::endl;
        std::cout << guard << std::endl;
    }
    const auto guard_renamed = guard->subs(renaming);

    if (rhs.has_value()) {
        return Clause(
            lhs_renamed,
            rhs.value().renameWith(renaming),
            guard_renamed
        );
    } else {
        return Clause(lhs_renamed, {}, guard_renamed);
    }
}

/**
 * Collect set of variables used in predicate arguments and guard.
 */
const VarSet Clause::vars() const {
    VarSet vs;
    for (const auto &pred: lhs) {
        vs.insertAll(pred.vars());
    }
    if (rhs.has_value()) {
        vs.insertAll(rhs.value().vars());
    }
    guard->collectVars(vs);
    return vs;
}

/**
 * TODO
 */
const Signature Clause::getSignature() const {
    std::vector<std::string> lhs;
    lhs.reserve(this->lhs.size());

    for (const auto& pred: this->lhs) {
        lhs.push_back(pred.name);
    }

    std::sort(lhs.begin(), lhs.end());

    if (this->rhs.has_value()) {
        return Signature(lhs, this->rhs.value().name);
    } else {
        return Signature(lhs);
    }
}

const std::vector<FunApp> deletePredAt(const std::vector<FunApp>& preds, unsigned index) {
    std::vector<FunApp> new_preds;
    for (unsigned i=0; i<preds.size(); i++) {
        if (i != index) {
            new_preds.push_back(preds.at(i));
        }
    }
    return new_preds;
}

/**
 * TODO
 */
const Clause Clause::makeVarsDisjointTo(const Clause& other_chc) const {
    Subs renaming;

    const VarSet this_vars {this->vars()};
    for (const auto &var: other_chc.vars()) {
        if (this_vars.contains(var)) {
            renaming.put(var, expr::toExpr(expr::next(var)));
        }
    }

    return this->renameWith(renaming);
}

/**
 * Compute resolution of `this` and `chc` using the RHS of `this` and `pred`,
 * which is assumed to be on the LHS of `chc`. So the caller is responsible for
 * choosing the literal to do resolution with. If `pred` is not on the LHS of
 * `chc` we throw an error.
 *
 * For example, with
 *
 * 	 this       : F(x) ==> G(x)
 * 	 chc        : G(y) /\ G(z) /\ y < z ==> H(y,z)
 *   pred_index :  ^--- points on first predicate (i.e. pred_index = 0)
 *
 * the returned resolvent should be
 *
 * 	 G(y) /\ F(z) /\ y < z ==> H(y,z)
 *
 */
const std::optional<std::tuple<Clause, BoolExpr>> Clause::resolutionWith(const Clause &chc, unsigned pred_index) const {
    if (pred_index >= chc.lhs.size()) {
        throw std::logic_error("Clause::resolutionWith: `pred_index` out-of-bounds");
    }
    const auto& resolved_pred = chc.lhs.at(pred_index);

    // No resolvent if `this` is a query, ie has no RHS predicate.
    if (this->isQuery()) {
        return {};
    }

    // Make sure variables in `this` and `chc` are disjoint.
    const Clause this_with_disjoint_vars = this->makeVarsDisjointTo(chc);

    const auto unifier = computeUnifier(this_with_disjoint_vars.rhs.value(), resolved_pred);

    // If the predicates are not unifiable, we don't throw an error but return
    // nullopt. That way the caller can filter out unifiable predicates using this
    // function. We could throw an error here too, but that implies that we expect
    // the caller to check unifiablility first and implementing that check would
    // be redundant. On the other hand, choosing a literal from the LHS of `chc`
    // is trivial, so we require the caller to do that correctly.
    if (!unifier.has_value()) {
        return {};
    }

    const Clause this_unified = this_with_disjoint_vars.renameWith(unifier.value());

    // LHS of resolvent is the union of the renamed LHS of `this` ...
    std::vector<FunApp> resolvent_lhs = this_unified.lhs;
    // ... and the LHS of `chc` where `pred` is removed.
    for (unsigned i=0; i<chc.lhs.size(); i++) {
        if (i != pred_index) {             
            resolvent_lhs.push_back(chc.lhs.at(i));
        }
    }

    const auto new_guard = this_unified.guard & chc.guard;
    const auto resolvent = Clause(resolvent_lhs, chc.rhs, new_guard);

    return std::make_tuple(resolvent, this_unified.guard);
}

/**
 * Partition clauses into linear- and non-linear. The first tuple component holds the linear
 * clauses while second component holds the non-linear clauses.
 */
const std::tuple<std::set<Clause>, std::set<Clause>> partitionByDegree(const std::set<Clause>& chcs) {
    std::set<Clause> linear_chcs;
    std::set<Clause> non_linear_chcs;

    for (const Clause& chc: chcs) {
        if (chc.isLinear()) {
            linear_chcs.insert(chc);
        } else {
            non_linear_chcs.insert(chc);
        }
    }

    return std::make_tuple(linear_chcs, non_linear_chcs);
}

/**
 * Partition given `chcs` by their RHS predicate. Returns a map where the keys are RHS predicate names and 
 * values are the sets of clauses with that RHS predicate. The key is optional because clauses may have no 
 * RHS predicate (i.e. querys).
 */
const std::map<std::optional<std::string>, std::vector<Clause>> partitionByRHS(const std::set<Clause>& chcs) {
    std::map<std::optional<std::string>, std::vector<Clause>> partition;

    for (const auto& chc: chcs) {
        std::optional<std::basic_string<char>> rhs_name;
        if (chc.rhs.has_value()) {
            rhs_name = chc.rhs.value().name;
        }

        if (partition.contains(rhs_name)) {
            std::vector<Clause>& group = partition.at(chc.rhs->name);
            group.push_back(chc);
        } else {
            partition.emplace(rhs_name, std::vector({ chc }));
        }
    }

    return partition;
}

/**
 * Filters facts from `chcs` and returns a tuple, where the first component contains all facts and 
 * the second component contains the remaining clauses.  
 */
const std::tuple<std::set<Clause>, std::set<Clause>> partitionFacts(const std::set<Clause>& chcs) {
    std::set<Clause> facts;
    std::set<Clause> non_facts;

    for (const auto& chc: chcs) {
        if (chc.isFact()) {
            facts.insert(chc);
        } else {
            non_facts.insert(chc);
        }
    }

    return std::make_tuple(facts, non_facts);
}

/**
 * TODO
 */
const std::map<Signature, std::vector<Clause>> partitionBySignature(const std::set<Clause>& chcs) {
    std::map<Signature, std::vector<Clause>> partition;

    for (const auto& chc: chcs) {
        const auto& sig = chc.getSignature();

        if (partition.contains(sig)) {           
            partition.at(sig).push_back(chc);
        } else {
            partition.emplace(sig, std::vector({ chc }));
        }
    }

    return partition;
}

/**
 * TODO
 */
void normalize_aux(
    const VarSet &all_original_vars,
    const FunApp &pred,
    unsigned &var_index,
    std::vector<BoolExpr> &implicit_equalities,
    Subs &renaming
) {

    for (const Var arg: pred.args) {
        Var new_arg = expr::varWithSameTypeAs(var_index, arg);
        var_index++;

        if (all_original_vars.contains(new_arg)) {
            renaming.put(new_arg, expr::toExpr(expr::next(new_arg)));
        }

        if (renaming.contains(arg)) { // already contained ==> implict equality
            implicit_equalities.push_back(
                expr::mkEq(renaming.get(arg), expr::toExpr(new_arg))
            );
        } else {
            renaming.put(arg, expr::toExpr(new_arg));
        }           
    }
}

/**
 * Normalize variable names to detect syntactic equivalence of clauses up-to-renaming.
 *
 * For that the RHS predicate arguments are renamed to always have the indices 0,1,2,3,...    
 * For example
 *
 *     i44 > i9 /\ b23 ==> F(i44,b23,i9)
 *
 * is renamed to
 * 
 *     i0 > i2 /\ b1 ==> F(i0,b1,i2)
 *
 * When a variable occurs multiple times in the arguments of the original RHS predicate. 
 * For example:
 * 
 *     i44 > i9 /\ b23 ==> F(i44,b23,i9,i44)
 *
 * then that consitutes an implicit equality that must be preserved. So we add it 
 * explicitly to the constraint:
 *
 *     i0 > i2 /\ b1 /\ i0=i3 ==> F(i0,b1,i2,i3)
 *
 */
const Clause Clause::normalize() const {
    const VarSet all_original_vars = this->vars();
    std::unique_ptr<Clause> result_chc = std::make_unique<Clause>(*this);

    unsigned var_index = 1;
    std::vector<BoolExpr> implicit_equalities;

    if (this->rhs.has_value()) {
        Subs renaming;

        normalize_aux(
            all_original_vars, 
            this->rhs.value(), 
            var_index, 
            implicit_equalities,
            renaming
        );

        result_chc = std::make_unique<Clause>(result_chc->renameWith(renaming));
    }

    Subs lhs_renaming;
    for (const auto &lhs_pred: result_chc->lhs) {
        normalize_aux(
            all_original_vars, 
            lhs_pred, 
            var_index, 
            implicit_equalities,
            lhs_renaming
        );
    }
    result_chc = std::make_unique<Clause>(result_chc->renameWith(lhs_renaming));

    implicit_equalities.push_back(result_chc->guard);
    const auto new_guard = BExpression::buildAnd(implicit_equalities)->simplify();

    return Clause(result_chc->lhs, result_chc->rhs, new_guard);
}

/**
 * Returns true iff the clause has at most one LHS predicate. For example:
 * 
 *     F(x) /\ x > 1 ==> F(x)          (linear)
 *
 *     F(x) /\ x > 1 ==> false         (linear)
 *
 *     x > 1 ==> F(x)                  (linear)
 *
 *     F(x) /\ G(x) ==> false          (non-linear)
 *
 */
bool Clause::isLinear() const {
    return lhs.size() <= 1;
}

/**
 * Returns true iff the clause has a RHS predicte but no LHS predicates. For example:
 * 
 *     x > 0 ==> F(x)                 (fact)
 *
 *     F(x) /\ x > 0 ==> F(x)         (not fact)
 *
 *     x > 0 ==> false                (not fact)
 *
 */
bool Clause::isFact() const {
    return lhs.size() == 0 && rhs.has_value();
}

/**
 * Returns true iff the clause has no RHS predicate. For example:
 *
 *     F(x) /\ x > 1 ==> false        (query)
 *
 *     x > 1 ==> F(x)                 (not query)
 *
 */
bool Clause::isQuery() const {
    return !rhs.has_value();
}

/**
 * Return first predicate with given `name` if it occurs on the LHS of the clause.
 * Returns nullopt if there is no predicate with given `name`. 
 */
std::optional<unsigned> Clause::indexOfLHSPred(const std::string name) const {
    for (unsigned i=0; i<lhs.size(); i++) {
        if (lhs.at(i).name == name) {
            return i;
        }
    }

    return {};
}

const Clause Clause::withGuard(const BoolExpr &new_guard) const {
    return Clause(this->lhs, this->rhs, new_guard);
}

/**
 * Find maximum number of int/bool-valued variables that appear in the arguments of any
 * any predicate in any clause of `chc_problem`. Returns counts as pair:
 * 
 *     [ max_int_arity, max_bool_arity ] = maxArity(chc_problem);
 * 
 */
const std::pair<unsigned long, unsigned long> maxArity(const std::vector<Clause>& chc_problem) {
    unsigned long max_int_arity {0};
    unsigned long max_bool_arity {0};

    for (const auto &chc: chc_problem) {
        if (chc.rhs.has_value()) {
            const auto& rhs = chc.rhs.value();
            max_int_arity = std::max(max_int_arity, rhs.intArity());
            max_bool_arity = std::max(max_bool_arity, rhs.boolArity());
        }

        for (const auto &pred: chc.lhs) {
            max_int_arity = std::max(max_int_arity, pred.intArity());
            max_bool_arity = std::max(max_bool_arity, pred.boolArity());
        }
    }

    return std::pair(max_int_arity, max_bool_arity);
}

// QUESTION: does C++ have no iterable type of somehting so we can abstract over the collection type?
bool allLinear(const std::vector<Clause>& chcs) {
    for (const auto& chc: chcs) {
        if (!chc.isLinear()) {
            return false;
        }
    }
    
    return true;
}
bool allLinear(const std::set<Clause>& chcs) {
    chcs.begin();

    for (const auto& chc: chcs) {
        if (!chc.isLinear()) {
            return false;
        }
    }
    
    return true;
}

bool operator<(const FunApp &fun1, const FunApp &fun2) {
    if (fun1.name < fun2.name) {
        return true;
    } else if (fun2.name < fun1.name) {
        return false;
    } else {
        return fun1.args < fun2.args;
    }
}

bool operator<(const Clause &c1, const Clause &c2) {
    if (c1.lhs < c2.lhs) {
        return true;
    } else if (c2.lhs < c1.lhs) {
        return false;
    } else if (c1.rhs < c2.rhs) {
        return true;
    } else if (c2.rhs < c1.rhs) {
        return false;
    } else if (c1.guard < c2.guard) {
        return true;
    } else if (c2.guard < c1.guard) {
        return false;
    } else {
        return false;
    }
}

bool operator==(const Signature &sig1, const Signature &sig2) {
    return sig1.rhs == sig2.rhs && sig1.lhs == sig2.lhs;
}

bool operator!=(const Signature &sig1, const Signature &sig2) {
    return !(sig1 == sig2);
}

bool operator<(const Signature &sig1, const Signature &sig2) {
    if (sig1.rhs < sig2.rhs) {
        return true;
    } else if (sig2.rhs < sig1.rhs) {
        return false;
    } else if (sig1.lhs < sig2.lhs) {
        return true;
    } else {
        return false;
    }
}

std::ostream &operator<<(std::ostream &s, const FunApp &fun) {
    s << fun.name;

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

    s << chc.guard << " ==> ";

    if (chc.rhs.has_value()) {
        s << chc.rhs.value();
    } else {
        s << "false";
    }

    return s;
}

std::ostream &operator<<(std::ostream &s, const Signature &sig) {
    for (const std::string& pred: sig.lhs) {
        s << pred << " /\\ ";
    }

    s << "(...) ==> ";

    if (sig.rhs.has_value()) {
        s << sig.rhs.value();
    } else {
        s << "false";
    }

    return s;
}
