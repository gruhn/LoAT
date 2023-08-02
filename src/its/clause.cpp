#include "clause.hpp"
#include "expr.hpp"
#include <cassert>
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

		assert(num_expr.isVar());
		return num_expr.toVar();
	} else if (std::holds_alternative<BoolVar>(var)) {
	    const BoolVar bool_var = std::get<BoolVar>(var);
	    const BoolExpr bool_expr = subs.get<BoolTheory>().get(bool_var); 

	    // TODO: there's probably a simpler way to restore a BoolVar from a BoolExpr?
	    const auto bool_expr_vars = bool_expr->vars();
		assert(bool_expr_vars.size() == 1);
        const BoolVar target_var = std::get<BoolVar>(*bool_expr_vars.begin());
		return target_var;
	} else {
	    throw std::logic_error("unsupported theory");
	}
}

/**
 * Tries to compute a unifier for the two given predicates. Because predicate arguments are 
 * always simple variables (instead of expressions), the unifier is simply a variable renaming. 
 * If the predicates don't have the same predicate symbol or different arity or the argument types
 * don't match, unification is not possible. For example:
 *
 *    F(x,x) and G(y,z) : not unifiable
 *    F(x)   and F(y,z) : not unifiable
 * 	  F(w,x) and F(y,z) : unifier = { w -> y, x -> z }
 *
 */
const std::optional<Subs> computeUnifier(const FunApp &pred1, const FunApp &pred2) {
	if (pred1.loc == pred2.loc && pred1.args.size() == pred2.args.size()) {
		// Renaming renaming;
		Subs subs;

	    size_t size = pred1.args.size();
	    for (size_t i = 0; i < size; ++i) {
			const auto var1 = pred1.args[i];
			const auto var2 = pred2.args[i];

			// QUESTION: I guess some C++ template magic can make this prettier
			bool both_nums = std::holds_alternative<NumVar>(var1) && std::holds_alternative<NumVar>(var2);
			bool both_bools = std::holds_alternative<BoolVar>(var1) && std::holds_alternative<BoolVar>(var2);

			if (both_nums) {
				subs.put(std::make_pair(
					std::get<NumVar>(var1), 
					std::get<NumVar>(var2)
				));
			} else if (both_bools) {
				subs.put(std::make_pair(
					std::get<BoolVar>(var1), 
					BExpression::buildTheoryLit(std::get<BoolVar>(var2))
				));
			} else {
				// argument types don't => not unifiable
				return {};
			}
	    }	

		return subs;
	} else {
		return {};
	}
}

const FunApp renameWith(const FunApp &pred, Subs renaming) {
	std::vector<Var> args;

	for (const Var &var: pred.args) {
		const auto target_var = varAt(var, renaming);

		if (target_var.has_value()) {
			args.push_back(target_var.value());
		} else {
			args.push_back(var);
		}
	}

	return FunApp(pred.loc, args);
}

const Clause renameWith(const Clause &chc, Subs renaming) {
	std::vector<FunApp> lhs_renamed;
	for (const FunApp &pred: chc.lhs) {
		lhs_renamed.push_back(renameWith(pred, renaming));
	}

	FunApp rhs_renamed = renameWith(chc.rhs, renaming);

	const auto guard_renamed = chc.guard->subs(renaming);

	return Clause(lhs_renamed, rhs_renamed, guard_renamed);
}

// QUESTION: is there no really pre-implemented version of this?
template <typename T>
const std::vector<T> concat(const std::vector<T> vec1, const std::vector<T> vec2) {
	std::vector<T> result;
	result.reserve(vec1.size() + vec2.size());

    // Copy elements from vec1 and vec2 to the concatenated vector
    std::copy(vec1.begin(), vec1.end(), std::back_inserter(result));
    std::copy(vec2.begin(), vec2.end(), std::back_inserter(result));

	return result;
}

/**
 *
 */
const std::optional<Clause> removeFromLHS(const Clause &chc, const FunApp &pred) {
	// auto pred_pos = std::find(chc.lhs.begin(), chc.lhs.end(), &pred);

	// if (pred_pos == chc.lhs.end()) {
	// 	return {};
	// } else {
		
	// }

	while (pred_pos < chc.lhs.end()) {
		if (&(*pred_pos) == &pred) { // TODO: does this make sense? I want to compare by reference
			break;
		}
		pred_pos++;
	}

	if (pred_pos == chc.lhs.end()) {
		return {};
	} else {
		auto lhs_without_pred = chc.lhs;
		lhs_without_pred.erase(pred_pos);

		// TODO
		return {};
	}
}

/**
 * Compute resolution of `fst` and `snd` using the RHS of `fst` and `pred`, which is 
 * assumed to be on the LHS of `snd`. So the caller is responsible for choosing the literal
 * to do resolution with. If `pred` is not on the LHS of `snd` we throw an error.
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
	std::cout << "test1" << std::endl;
	const auto snd_without_pred_optional = removeFromLHS(snd, pred);
	std::cout << "test2" << std::endl;
	assert(snd_without_pred_optional.has_value() && "Given `pred` is not on the LHS of `snd`");
	std::cout << "test3" << std::endl;
	const Clause snd_without_pred = snd_without_pred_optional.value();

	const auto unifier = computeUnifier(fst.rhs, pred);

	// If the predicates are not unifiable, we don't throw an error but return nullopt.
	// That way the caller can filter out unifiable predicates using this function.
	// We could throw an error here too, but that implies that we expect the caller to 
	// check unifiablility first and implementing that check would be redundant.
	// On the other hand, choosing a literal from the LHS of `snd` is trivial, so we make 
	// require the caller to do that correctly.
	if (!unifier.has_value()) {
		return {};
	} 

	const auto fst_renamed = renameWith(fst, unifier.value());
	std::cout << "test9" << std::endl;

	return Clause(
		concat(fst_renamed.lhs, snd_without_pred.lhs), // resolvent LHS
		snd_without_pred.rhs,                          // resolvent RHS
		fst_renamed.guard & snd_without_pred.guard	   // resolvent guard
	);
}

std::ostream& operator<<(std::ostream &s, const FunApp &fun) {
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

std::ostream& operator<<(std::ostream &s, const Clause &chc) {
    for (const FunApp &fun_app: chc.lhs) {
        s << fun_app << " /\\ ";
    }

    s << chc.guard << " ==> " << chc.rhs;
    // s << "<guard>" << " ==> " << chc.rhs;

	return s;
}
