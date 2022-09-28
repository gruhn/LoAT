#pragma once

#include "boolvar.hpp"
#include "boolexpr.hpp"

class BoolSubs {

    BoolVarMap<BoolExpr> map;

public:

    using const_iterator = typename BoolVarMap<BoolExpr>::const_iterator;

    BoolSubs();

    BoolSubs(const BoolVar &key, const BoolExpr &val);

    void put(const BoolVar &key, const BoolExpr &val);

    BoolExpr get(const BoolVar &var) const;

    BoolExpr operator()(const BoolExpr e) const;

    bool contains(const BoolVar &var) const;

    BoolSubs compose(const BoolSubs &that) const;

    BoolSubs concat(const BoolSubs &that) const;

    BoolSubs project(const BoolVarSet &vars) const;

    bool changes(const BoolVar &key) const;

    BoolVarSet domain() const;

    BoolVarSet coDomainVars() const;

    BoolVarSet allVars() const;

    void collectDomain(BoolVarSet &vars) const;

    void collectCoDomainVars(BoolVarSet &vars) const;

    void collectVars(BoolVarSet &vars) const;

    unsigned hash() const;

    bool empty() const;

    const_iterator begin() const;

    const_iterator end() const;

    const_iterator find(const BoolVar &var) const;

    size_t size() const;

    void erase(const BoolVar &var);

    int compare(const BoolSubs& that) const;

    friend bool operator==(const BoolSubs &s1, const BoolSubs &s2);
};

std::ostream& operator<<(std::ostream &s, const BoolSubs &e);
bool operator==(const BoolSubs &s1, const BoolSubs &s2);
bool operator<(const BoolSubs &s1, const BoolSubs &s2);
