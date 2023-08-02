#include "clause.hpp"
#include "expr.hpp"
#include <cassert>
#include <stdexcept>

/**
 * Tries to compute a unifier for the two given predicates. Because predicate arguments are 
 * always simple variables (instead of expressions), the unifier is simply a variable renaming. 
 * If the predicates don't have the same predicate symbol or have different arity, unification
 * is not possible. For example:
 * 
 * TODO: also not unifiable if variable type is differnt
 *
 *    F(x,x) and G(y,z) : not unifiable
 *    F(x)   and F(y,z) : not unifiable
 * 	  F(w,x) and F(y,z) : unifier = { w -> y, x -> z }
 *	   
 */
const std::optional<std::map<Var, Var>> getUnifier(const FunApp &pred1, const FunApp &pred2) {
	if (pred1.loc == pred2.loc && pred1.args.size() == pred2.args.size()) {
		// Predicates can't be unified if they have different predicate symbols.
		return {};
	} else {
		// Can't unify predicates with different predicate symbol or different arity.
		// For example: 
		//
		return {};
	}
}

/**
 *
 */
const std::optional<std::pair<Clause, FunApp>> extractFromLHS(Clause &chc, FunApp &pred) {
	// find the position of `lit` on the LHS of `snd`
	auto lit_pos = snd.lhs.begin();
	while (lit_pos < snd.lhs.end()) {
		if (&(*lit_pos) == &lit) { // TODO: does this make sense? I want to compare by reference
			break;
		}
		lit_pos++;
	}

	auto snd_lhs_without_lit = snd.lhs;
	snd_lhs_without_lit.erase(lit_pos);

	// TODO
	return {};
}

/**
 * Compute resolution of `fst` and `snd` using the RHS of `fst` and `pred`, which is 
 * assumed to be on the LHS of `snd`. So the caller is responsible for choosing the literal
 * to do resolution with. If `pred` is not on the LHS of `snd` we throw an error.
 */
const std::optional<Clause> resolutionWith(Clause &fst, Clause &snd, FunApp &pred) {
	const auto snd_without_pred = extractFromLHS(snd, pred);
	assert(snd_without_pred.has_value() && "Given `pred` is not on the LHS of `snd`");

	const auto unifier = getUnifier(fst.rhs, pred);

	// If the predicates are not unifiable, we don't throw an error but return nullopt.
	// That way the caller can filter out unifiable predicates using this function.
	// We could throw an error here too, but that implies that we expect the caller to 
	// check unifiablility first and implementing that check would be redundant.
	// On the other hand, choosing a literal from the LHS of `snd` is trivial, so we make 
	// require the caller to do that correctly.
	if (!unifier.has_value()) {
		return {};
	}

	// TODO: apply unifier

	// const auto resolvent_lhs = 

	return Clause(snd_lhs_without_lit, snd.rhs, snd.guard);
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
