#include "rule.hpp"
#include "theory.hpp"
#include "types.hpp"
#include <vector>

struct FunApp {
    const LocationIdx loc;
    const std::vector<Var> args;

    FunApp(const LocationIdx loc, const std::vector<Var> args): loc(loc), args(args) {}
};

// TODO: convert to class I guess
struct Clause {
    const std::set<FunApp> lhs;
    const FunApp rhs;
    const BoolExpr guard;

    Clause(const std::set<FunApp> &lhs, const FunApp &rhs, const BoolExpr &guard): lhs(lhs), rhs(rhs), guard(guard) {}
};

const std::optional<Var> varAt(const Var &var, const Subs &subs);

const FunApp renameWith(const FunApp &pred, const Subs renaming);

const Clause renameWith(const Clause &chc, const Subs renaming);

const std::optional<Clause> resolutionWith(const Clause &fst, const Clause &snd, const FunApp &pred);

// implement comparison operator for FunApp so we can store them in std::set
bool operator<(const FunApp &fun1, const FunApp &fun2);

std::ostream& operator<<(std::ostream &s, const FunApp &fun_app);

std::ostream& operator<<(std::ostream &s, const Clause &chc);