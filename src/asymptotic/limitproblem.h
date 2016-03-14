#ifndef LIMITPROBLEM_H
#define LIMITPROBLEM_H

#include <set>
#include <utility>

#include "exceptions.h"
#include "expression.h"
#include "guardtoolbox.h"


enum InftyDirection { POS_INF, NEG_INF, POS_CONS, NEG_CONS, POS };

class InftyExpression : public Expression {
public:

    InftyExpression(InftyDirection dir = POS);
    InftyExpression(const GiNaC::basic &other, InftyDirection dir = POS);
    InftyExpression(const GiNaC::ex &other, InftyDirection dir = POS);

    void setDirection(InftyDirection dir);
    InftyDirection getDirection() const;

private:
    InftyDirection direction;
};

typedef std::set<InftyExpression, GiNaC::ex_is_less> InftyExpressionSet;

EXCEPTION(LimitProblemIsContradictoryException, CustomException);

class LimitProblem {
public:
    LimitProblem();
    LimitProblem(const GuardList &normalizedGuard, const Expression &cost);

    void addExpression(const InftyExpression &ex);

    InftyExpressionSet::const_iterator cbegin() const;
    InftyExpressionSet::const_iterator cend() const;

    // (A)
    void applyLimitVector(const InftyExpressionSet::const_iterator &it, int pos,
                          InftyDirection lvType, InftyDirection first, InftyDirection second);
    // (B)
    void removeConstant(const InftyExpressionSet::const_iterator &it);
    // (C)
    void substitute(const GiNaC::exmap &sub);
    // (D)
    void trimPolynomial(const InftyExpressionSet::const_iterator &it);
    // (E)
    void reducePolynomialPower(const InftyExpressionSet::const_iterator &it);

    bool isSolved() const;
    GiNaC::exmap getSolution() const;
    ExprSymbol getN() const;

    bool removeConstantIsApplicable(const InftyExpressionSet::const_iterator &it);
    bool trimPolynomialIsApplicable(const InftyExpressionSet::const_iterator &it);

private:
    InftyExpressionSet set;
    ExprSymbol variableN;

    //debug dumping
    void dump(const std::string &description) const;
};

#endif //LIMITPROBLEM_H