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

const std::optional<Var> varAt(const Var &var, const Subs &subs);

const FunApp renameWith(const FunApp &pred, Subs renaming);

const Clause renameWith(const Clause &chc, Subs renaming);

const std::optional<Clause> resolutionWith(const Clause &fst, const Clause &snd, const FunApp &snd_lhs_literal);


std::ostream& operator<<(std::ostream &s, const FunApp &fun_app);

std::ostream& operator<<(std::ostream &s, const Clause &chc);