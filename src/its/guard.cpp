#include "guard.hpp"
#include "boolexpr.hpp"

void Guard::collectVariables(VarSet &res) const {
    for (const Lit &lit : *this) {
        res.collectVars(lit);
    }
}

Guard Guard::subs(const ThSubs &sigma) const {
    Guard res;
    for (const Lit &rel: *this) {
        res.push_back(sigma.subs(rel));
    }
    return res;
}

bool operator<(const Guard &m1, const Guard &m2);

std::ostream& operator<<(std::ostream &s, const Guard &l) {
    return s << buildAnd(l);
}

Guard operator&(const Guard &fst, const Guard &snd) {
    Guard res(fst);
    res.insert(res.end(), snd.begin(), snd.end());
    return res;
}

Guard operator&(const Guard &fst, const Rel &snd) {
    Guard res(fst);
    res.push_back(snd);
    return res;
}
