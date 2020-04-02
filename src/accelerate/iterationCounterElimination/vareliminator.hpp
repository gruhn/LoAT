#ifndef VARELIMINATOR_HPP
#define VARELIMINATOR_HPP

#include "../../its/itsproblem.hpp"

/**
 * Computes substitutions that are suitable to eliminate the given temporary variable from the rule by replacing it with its bounds.
 */
class VarEliminator
{
public:

    VarEliminator(const BoolExpr &guard, const Var &N, VariableManager &varMan);

    const std::set<Subs> getRes() const;

private:

    /**
     * Dependencies are other temporary variables that render a bound on N useless.
     * For example, if we have N * M <= X, then we cannot instantiate N with X/M, as the bound must always evaluate to an integer.
     * Thus, in this case M is a dependency of N.
     */
    void findDependencies(const BoolExpr &guard);

    /**
     * Tries to eliminate a single dependency by instantiating it with a constant bound.
     * Creates a new branch (i.e., a new entry in todoDeps) for every possible instantiation.
     */
    const std::set<std::pair<Subs, BoolExpr>> eliminateDependency(const Subs &subs, const BoolExpr &guard) const;

    /**
     * Eliminates as many dependencies as possible by instantiating them with constant bounds.
     */
    void eliminateDependencies();

    /**
     * First eliminates as many dependencies as possible, then eliminates N, if possible.
     */
    void eliminate();

    VariableManager &varMan;

    Var N;

    /**
     * Each entry represents one branch in the search for suitable instantiations of dependencies.
     * Entries that do not allow for further instantiation are moved to todoN.
     */
    std::stack<std::pair<Subs, BoolExpr>> todoDeps;

    /**
     * Each entry represents one possibility to instantiate dependencies exhaustively.
     * N still needs to be eliminated.
     */
    std::set<std::pair<Subs, BoolExpr>> todoN;

    /**
     * Substitutions that are suitable to eliminate N.
     */
    std::set<Subs> res;

    VarSet dependencies;

};

#endif // VARELIMINATOR_HPP
