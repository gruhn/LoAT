#pragma once

#include "boolvar.hpp"

class BoolLit {
    BoolVar var;
    bool negated;

public:

    BoolLit(const BoolVar &var, bool negated = false);
    std::string toRedlog() const;
    unsigned hash() const;
    bool isNegated() const;
    bool isWellformed() const;
    BoolVar getBoolVar() const;
    int compare(const BoolLit &that) const;
    void collectVars(std::set<BoolVar> &res) const;
    BoolLit normalize() const;
    bool isTriviallyTrue() const;

};

bool operator<(const BoolLit &l1, const BoolLit &l2);
bool operator==(const BoolLit &l1, const BoolLit &l2);
BoolLit operator!(const BoolLit &l);

std::ostream& operator<<(std::ostream &s, const BoolLit &e);
