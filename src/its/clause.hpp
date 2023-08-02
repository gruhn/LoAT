
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
    const std::vector<FunApp> lhs;
    const FunApp rhs;
    const BoolExpr guard;

    Clause(const std::vector<FunApp> &lhs, const FunApp &rhs, const BoolExpr &guard): lhs(lhs), rhs(rhs), guard(guard) {}
};

// TODO
// const FunApp unify(FunApp fun1, FunApp fun2);

const std::optional<std::map<Var, Var>> getUnifier(const FunApp &fun1, const FunApp &fun2);

const Clause rename(Clause &chc, std::map<Var, Var> renaming);

const std::optional<Clause> resolutionWith(Clause &fst, Clause &snd, FunApp &snd_lhs_literal);

const std::optional<Rule> toRule(Clause &chc);

// -----------------------

// const std::optional<std::pair<FunApp, Clause>> extractFromLhs(Clause &c, LocationIdx loc);

std::ostream& operator<<(std::ostream &s, const FunApp &fun_app);

std::ostream& operator<<(std::ostream &s, const Clause &chc);