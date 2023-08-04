#include "rule.hpp"
#include "theory.hpp"
#include "types.hpp"
#include <vector>

class FunApp {

public:
    const LocationIdx loc;
    const std::vector<Var> args;

    FunApp(const LocationIdx loc, const std::vector<Var> args): loc(loc), args(args) {}

    const FunApp renameWith(const Subs &renaming) const;
};

class Clause {

public:
    const std::set<FunApp> lhs;
    const FunApp rhs;
    const BoolExpr guard;

    Clause(const std::set<FunApp> &lhs, const FunApp &rhs, const BoolExpr &guard);

    const Clause renameWith(const Subs &renaming) const;

    const std::optional<Clause> resolutionWith(const Clause &chc, const FunApp &pred) const;

    bool isLinear() const;

};

// implement comparison operator for FunApp so we can store them in std::set
bool operator<(const FunApp &fun1, const FunApp &fun2);

std::ostream& operator<<(std::ostream &s, const FunApp &fun_app);

std::ostream& operator<<(std::ostream &s, const Clause &chc);