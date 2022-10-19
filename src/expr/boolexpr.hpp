#pragma once

#include "option.hpp"
#include "itheory.hpp"
#include "thset.hpp"
#include "conjunction.hpp"
#include "rel.hpp"
#include "boollit.hpp"
#include "literaltemplates.hpp"

#include <type_traits>
#include <memory>
#include <set>

template <ITheory... Th>
class BoolTheoryLit;
template <ITheory... Th>
class BoolJunction;
template <IBaseTheory... Th>
class BoolExpression;
class Quantifier;
template <ITheory... Th>
class QuantifiedFormula;
template <ITheory... Th>
using BExpr = std::shared_ptr<const BoolExpression<Th...>>;

namespace literal {

template<std::size_t I = 0, ITheory... Th>
inline BExpr<Th...> subsImpl(const typename Theory<Th...>::Lit &lit, const theory::Subs<Th...> &s) {
    if constexpr (I < sizeof...(Th)) {
        if (lit.index() == I) {
            using T = typename std::tuple_element_t<I, std::tuple<Th...>>;
            if constexpr (std::same_as<typename T::Var, BoolVar>) {
                return s.template get<T>().subs(std::get<I>(lit));
            } else {
                return BoolExpression<Th...>::buildTheoryLit(std::get<I>(lit).subs(s.template get<T>()));
            }
        } else {
            return subsImpl<I+1, Th...>(lit, s);
        }
    } else {
        throw std::logic_error("unknown theory");
    }
}

template<ITheory... Th>
BExpr<Th...> subs(const typename Theory<Th...>::Lit &lit, const theory::Subs<Th...> &s) {
    return subsImpl<0, Th...>(lit, s);
}

}

template <ITheory... Th>
struct BExpr_compare {
    bool operator() (const BExpr<Th...> a, const BExpr<Th...> b) const {
        return a->compare(b) < 0;
    }
};

template <ITheory... Th>
using BoolExpressionSet = std::set<BExpr<Th...>, BExpr_compare<Th...>> ;

enum ConcatOperator { ConcatAnd, ConcatOr };

template <IBaseTheory... Th>
class BoolExpression: public std::enable_shared_from_this<BoolExpression<Th...>> {

    static_assert(sizeof...(Th) > 0);

    friend class BoolTheoryLit<Th...>;
    friend class BoolJunction<Th...>;

    using T = Theory<Th...>;
    using Var = typename T::Var;
    using Lit = typename T::Lit;
    using VS = theory::VarSet<Th...>;
    using LS = theory::LitSet<Th...>;
    using G = Conjunction<Th...>;
    using BE = BExpr<Th...>;
    using BES = BoolExpressionSet<Th...>;
    using QF = QuantifiedFormula<Th...>;
    using Subs = theory::Subs<Th...>;

public:

    static const BE True;
    static const BE False;

    template <class Lits>
    static const BE buildFromLits(const Lits &lits, ConcatOperator op) {
        BES children;
        for (const Lit &lit: lits) {
            children.insert(buildTheoryLit(lit));
        }
        return BE(new BoolJunction(children, op));
    }

    template <class Children>
    static const BE build(const Children &lits, ConcatOperator op) {
        std::stack<BE> todo;
        for (const auto &lit: lits) {
            todo.push(lit);
        }
        BES children;
        while (!todo.empty()) {
            BE current = todo.top();
            if ((op == ConcatAnd && current->isAnd()) || (op == ConcatOr && current->isOr())) {
                const BES &currentChildren = current->getChildren();
                todo.pop();
                for (const BE &c: currentChildren) {
                    todo.push(c);
                }
            } else {
                children.insert(current);
                todo.pop();
            }
        }
        if (children.size() == 1) {
            return *children.begin();
        }
        return BE(new BoolJunction(children, op));
    }

    template <class Lits>
    static const BE buildAndFromLits(const Lits &lits) {
        return buildFromLits(lits, ConcatAnd);
    }

    template <class Children>
    static const BE buildAnd(const Children &lits) {
        return build(lits, ConcatAnd);
    }

    template <class Lits>
    static const BE buildOrFromLits(const Lits &lits) {
        return buildFromLits(lits, ConcatOr);
    }

    template <class Children>
    static const BE buildOr(const Children &lits) {
        return build(lits, ConcatOr);
    }

    static const BE buildTheoryLit(const Lit &lit) {
        return BE(new BoolTheoryLit<Th...>(lit));
    }

    virtual option<Lit> getTheoryLit() const = 0;
    virtual bool isAnd() const = 0;
    virtual bool isOr() const = 0;
    virtual BES getChildren() const = 0;
    virtual const BE negation() const = 0;
    virtual bool forall(const std::function<bool(const Lit&)> &pred) const = 0;
    virtual LS universallyValidLits() const = 0;
    virtual bool isConjunction() const = 0;
    virtual void collectLits(LS &res) const = 0;
    virtual std::string toRedlog() const = 0;
    virtual size_t size() const = 0;
    virtual unsigned hash() const = 0;
    virtual void getBounds(const Var &n, Bounds &res) const = 0;
    virtual int compare(const BE that) const = 0;

    bool isTriviallyTrue() const {
        if (getTheoryLit()) {
            return std::visit([](const auto &lit){return lit.isTriviallyTrue();}, *getTheoryLit());
        } else {
            const auto children = getChildren();
            if (isAnd()) {
                return std::all_of(children.begin(), children.end(), [](const auto &c){return c->isTriviallyTrue();});
            } else if (isOr()) {
                return std::any_of(children.begin(), children.end(), [](const auto &c){return c->isTriviallyTrue();});
            } else {
                throw std::logic_error("unknown junctor");
            }
        }
    }

    void iter(const std::function<void(const Lit&)> &f) const {
        if (getTheoryLit()) {
            f(*getTheoryLit());
        } else {
            for (const auto &c: getChildren()) {
                c->iter(f);
            }
        }
    }

    template <ITheory... Th_>
    BExpr<Th_...> map(const std::function<BExpr<Th_...>(const Lit&)> &f) const {
        if (isAnd()) {
            BoolExpressionSet<Th_...> newChildren;
            for (const auto &c: getChildren()) {
                const auto simp = c->map(f);
                if (simp == BoolExpression<Th_...>::False) {
                    return BoolExpression<Th_...>::False;
                } else {
                    if (simp != BoolExpression<Th_...>::True) {
                        newChildren.insert(simp);
                    }
                }
            }
            if (newChildren.empty()) {
                return BoolExpression<Th_...>::True;
            } else {
                return BoolExpression<Th_...>::buildAnd(newChildren);
            }
        } else if (isOr()) {
            BoolExpressionSet<Th_...> newChildren;
            for (const auto &c: getChildren()) {
                const auto simp = c->map(f);
                if (simp == BoolExpression<Th_...>::True) {
                    return BoolExpression<Th_...>::True;
                } else {
                    if (simp != BoolExpression<Th_...>::False) {
                        newChildren.insert(simp);
                    }
                }
            }
            if (newChildren.empty()) {
              return BoolExpression<Th_...>::False;
            } else {
                return BoolExpression<Th_...>::buildOr(newChildren);
            }
        } else if (getTheoryLit()) {
            return f(*getTheoryLit());
        }
        throw std::logic_error("unknown boolean expression");
    }

    BE map(const std::function<BE(const Lit&)> &f) const {
        return map<Th...>(f);
    }

    BE subs(const Subs &subs) const {
        return map([&subs](const auto &lit) {
            return literal::subs<Th...>(lit, subs);
        });
    }

    void collectVars(VS &vars) const {
        iter([&vars](const auto &lit) {
            literal::collectVars<Th...>(lit, vars);
        });
    }

    template <ITheory T>
    void collectVars(std::set<typename T::Var> &vars) const {
        VS res;
        res.template get<typename T::Var>() = vars;
        collectVars(res);
    }

    VS vars() const {
        VS res;
        collectVars(res);
        return res;
    }

    BE simplify() const {
        return map([](const Lit &lit) -> BE {
            return std::visit(Overload{
                                  [](const Rel &lit) -> BE {
                                      if (lit.isTriviallyTrue()) {
                                          return True;
                                      } else if (lit.isTriviallyFalse()) {
                                          return False;
                                      } else if (lit.isNeq()) {
                                          return buildTheoryLit(Rel(lit.lhs(), Rel::lt, lit.rhs())) | (Rel(lit.lhs(), Rel::gt, lit.rhs()));
                                      } else {
                                          return buildTheoryLit(lit);
                                      }
                                  },
                                  [](const auto &lit) -> BE {return buildTheoryLit(lit);}
                              }, lit);
        });
    }

    template <ITheory T>
    BE replaceLits(const std::map<typename T::Lit, BE> &m) const {
        return map(Overload{
                       [&m](const Rel &lit) -> BE {
                           const auto it = m.find(lit);
                           if (it == m.end()) {
                               return buildTheoryLit(Lit(lit));
                           } else {
                               return it->second;
                           }
                       },
                       [&m](const auto &lit) -> BE {
                           return buildTheoryLit(Lit(lit));
                       }
                   });
    }

    virtual ~BoolExpression() {};

    LS lits() const {
        LS res;
        collectLits(res);
        return res;
    }

    bool isLinear() const {
        return forall(Overload {
                          [](const Rel &rel){return rel.isLinear();},
                          [](const BoolLit &lit){return true;},
                          [](const auto &lit){return false;}
                      });
    }

    bool isPoly() const{
        return forall(Overload {
                          [](const Rel &rel){return rel.isPoly();},
                          [](const BoolLit &lit){return true;},
                          [](const auto &lit){return false;}
                      });
    }

private:

    void findConsequences(const BExpr<Th...> &lit, BES &res) const {
        if (isAnd()) {
            for (const auto &c: getChildren()) {
                c->findConsequences(lit, res);
            }
        } else if (isOr()) {
            BES children = getChildren();
            bool trivial = true;
            for (auto it = children.begin(); it != children.end();) {
                const BE c = *it;
                if ((c == lit) || (c->isAnd() && c->getChildren().contains(lit))) {
                    it = children.erase(it);
                    trivial = false;
                } else {
                    ++it;
                }
            }
            if (!trivial) {
                res.insert(buildOr(children));
            }
        }
    }

public:

    BES findConsequences(const Lit &lit) const {
        BES res;
        findConsequences(buildTheoryLit(lit), res);
        return res;
    }

    std::vector<G> dnf() const {
        std::vector<G> res;
        dnf(res);
        return res;
    }

    G conjunctionToGuard() const{
        const LS &lits = this->lits();
        return G(lits.begin(), lits.end());
    }

    QF quantify(const std::vector<Quantifier> &prefix) const {
        return QuantifiedFormula(prefix, this->shared_from_this());
    }

    BE toG() const {
        const std::function<BE(const Lit &lit)> mapper = [](const Lit &lit) {
            return std::visit(Overload{
                                  [](const Rel &lit) -> BE {
                                      return buildTheoryLit(lit.toG());
                                  },
                                  [](const auto &lit) -> BE {
                                      return buildTheoryLit(lit);
                                  }
                              }, lit);};
        return map(mapper);
    }

    template <ITheory T>
    BExpr<T> transform() const {
        static const std::function<BExpr<T>(const Lit&)> mapper = [](const Lit &lit) {
            return std::visit(Overload{
                                  [](const typename T::Lit &rel) -> BExpr<T> {return BoolExpression<T>::buildTheoryLit(rel);},
                                  [](const auto &rel) -> BExpr<T> {throw std::logic_error("transform failed");},
                              }, lit);};
        return map(mapper);
    }

protected:
    virtual void dnf(std::vector<G> &res) const = 0;
};

template <IBaseTheory... Th>
const BExpr<Th...> BoolExpression<Th...>::True = BE(new BoolJunction(BoolExpressionSet<Th...>{}, ConcatAnd));

template <IBaseTheory... Th>
const BExpr<Th...> BoolExpression<Th...>::False = BE(new BoolJunction(BoolExpressionSet<Th...>{}, ConcatOr));

template <ITheory... Th>
class BoolTheoryLit: public BoolExpression<Th...> {

    static_assert(sizeof...(Th) > 0);

    using T = Theory<Th...>;
    using Var = typename T::Var;
    using Lit = typename T::Lit;
    using VS = theory::VarSet<Th...>;
    using LS = theory::LitSet<Th...>;
    using C = Conjunction<Th...>;
    using BE = BExpr<Th...>;
    using BES = BoolExpressionSet<Th...>;
    using Subs = theory::Subs<Th...>;

    Lit lit;

public:

    BoolTheoryLit(const Lit &lit) : lit(std::visit([](const auto &lit){
        return Lit(lit.normalize());
    }, lit)) {}

    bool isAnd() const override {
        return false;
    }

    bool isOr() const override {
        return false;
    }

    option<Lit> getTheoryLit() const override {
        return {lit};
    }

    BES getChildren() const override {
        return {};
    }

    const BE negation() const override {
        return BoolExpression<Th...>::buildTheoryLit(std::visit([](const auto &lit){return Lit(!lit);}, lit));
    }

    bool forall(const std::function<bool(const Lit&)> &pred) const override {
        return pred(lit);
    }

    ~BoolTheoryLit() override {}

    bool isConjunction() const override {
        return true;
    }

    LS universallyValidLits() const override {
        LS res;
        res.insert(lit);
        return res;
    }

    void collectLits(LS &res) const override {
        res.insert(lit);
    }

    size_t size() const override {
        return 1;
    }

    std::string toRedlog() const override {
        return std::visit([](const auto &lit){
            return lit.toRedlog();
        }, lit);
    }

    unsigned hash() const override {
        return std::visit([](const auto &lit){
            return lit.hash();
        }, lit);
    }

    void getBounds(const Var &var, Bounds &res) const override {
        std::visit(Overload{
                       [&res](const Rel &lit, const NumVar &var){
                           lit.getBounds(var, res);
                       },
                       [](const auto &lit, const auto &var){}
                   }, lit, var);
    }

    int compare(const BE that) const override {
        if (!that->getTheoryLit()) {
            return -1;
        }
        if (lit == *that->getTheoryLit()) {
            return 0;
        }
        return lit < *that->getTheoryLit() ? -1 : 1;
    }

protected:

    void dnf(std::vector<C> &res) const override {
        if (res.empty()) {
            res.push_back({lit});
        } else {
            for (auto &g: res) {
                g.push_back(lit);
            }
        }
    }

};

template <ITheory... Th>
class BoolJunction: public BoolExpression<Th...> {

    static_assert(sizeof...(Th) > 0);

    using T = Theory<Th...>;
    using Var = typename T::Var;
    using Lit = typename T::Lit;
    using VS = theory::VarSet<Th...>;
    using LS = theory::LitSet<Th...>;
    using C = Conjunction<Th...>;
    using BE = BExpr<Th...>;
    using BES = BoolExpressionSet<Th...>;
    using Subs = theory::Subs<Th...>;

    BE True = BoolExpression<Th...>::True;
    BE False = BoolExpression<Th...>::False;

    BES children;
    ConcatOperator op;

public:

    BoolJunction(const BES &children, ConcatOperator op): children(children), op(op) { }

    bool isAnd() const override {
        return op == ConcatAnd;
    }

    bool isOr() const override {
        return op == ConcatOr;
    }

    option<Lit> getTheoryLit() const override {
        return {};
    }

    BES getChildren() const override {
        return children;
    }

    const BE negation() const override {
        BES newChildren;
        for (const BE &c: children) {
            newChildren.insert(c->negation());
        }
        switch (op) {
        case ConcatOr: return BoolExpression<Th...>::buildAnd(newChildren);
        case ConcatAnd: return BoolExpression<Th...>::buildOr(newChildren);
        }
        throw std::invalid_argument("unknown junction");
    }

    bool forall(const std::function<bool(const Lit&)> &pred) const override {
        for (const BE &e: children) {
            if (!e->forall(pred)) {
                return false;
            }
        }
        return true;
    }

    ~BoolJunction() override {}

    bool isConjunction() const override {
        return isAnd() && std::all_of(children.begin(), children.end(), [](const BE c){
            return c->isConjunction();
        });
    }

    LS universallyValidLits() const override {
        LS res;
        if (isAnd()) {
            for (const BE &c: children) {
                const option<Lit> lit = c->getTheoryLit();
                if (lit) {
                    res.insert(*lit);
                }
            }
        }
        return res;
    }

    void collectLits(LS &res) const override {
        for (const BE &c: children) {
            c->collectLits(res);
        }
    }

    size_t size() const override {
        size_t res = 1;
        for (const BE &c: children) {
            res += c->size();
        }
        return res;
    }

    std::string toRedlog() const override {
        std::string infix = isAnd() ? " and " : " or ";
        std::string res;
        bool first = true;
        for (auto it = children.begin(); it != children.end(); ++it) {
            if (first) first = false;
            else res += infix;
            res += (*it)->toRedlog();
        }
        return "(" + res + ")";
    }

    unsigned hash() const override {
        unsigned hash = 7;
        for (const BE& c: children) {
            hash = 31 * hash + c->hash();
        }
        hash = 31 * hash + op;
        return hash;
    }

    void getBounds(const Var &n, Bounds &res) const override {
        if (isAnd()) {
            for (const auto &c: children) {
                c->getBounds(n, res);
            }
        } else if (isOr()) {
            bool first = true;
            for (const auto &c: children) {
                if (first) {
                    c->getBounds(n, res);
                    first = false;
                } else {
                    Bounds cres = res;
                    c->getBounds(n, cres);
                    if (res.equality && (!cres.equality || !(*res.equality - *cres.equality).isZero())) {
                        res.equality = {};
                    }
                    if (!res.equality && res.lowerBounds.empty() && res.upperBounds.empty()) {
                        return;
                    }
                }
            }
        }
    }

    int compare(const BE that) const override {
        if (!that->isAnd() && !that->isOr()) {
            return 1;
        }
        if (isAnd() && that->isOr()) {
            return 1;
        }
        if (isOr() && that->isAnd()) {
            return -1;
        }
        const auto c1 = getChildren();
        const auto c2 = that->getChildren();
        if (c1 == c2) return 0;
        return c1 < c2 ? -1 : 1;
    }

protected:

    void dnf(std::vector<C> &res) const override {
        if (isAnd()) {
            for (const BE &e: children) {
                e->dnf(res);
            }
        } else {
            std::vector<C> oldRes(res);
            res.clear();
            for (const BE &e: children) {
                std::vector<C> newRes(oldRes);
                e->dnf(newRes);
                res.insert(res.end(), newRes.begin(), newRes.end());
            }
        }
    }

};

class Quantifier {

public:
    enum class Type { Forall, Exists };

private:
    Type qType;
    std::set<NumVar> vars;
    std::map<NumVar, Expr> lowerBounds;
    std::map<NumVar, Expr> upperBounds;

public:
    Quantifier(const Type &qType, const std::set<NumVar> &vars, const std::map<NumVar, Expr> &lowerBounds, const std::map<NumVar, Expr> &upperBounds);

    Quantifier negation() const;
    const std::set<NumVar>& getVars() const;
    Type getType() const;
    std::string toRedlog() const;
    option<Expr> lowerBound(const NumVar &x) const;
    option<Expr> upperBound(const NumVar &x) const;
    Quantifier remove(const NumVar &x) const;

};

template <ITheory... Th>
class QuantifiedFormula {

    static_assert(sizeof...(Th) > 0);

    using T = Theory<Th...>;
    using Var = typename T::Var;
    using Lit = typename T::Lit;
    using VS = theory::VarSet<Th...>;
    using LS = theory::LitSet<Th...>;
    using BE = BExpr<Th...>;
    using QF = QuantifiedFormula<Th...>;
    using Subs = theory::Subs<Th...>;

    BE True = BoolExpression<Th...>::True;
    BE False = BoolExpression<Th...>::False;

    std::vector<Quantifier> prefix;
    BE matrix;

    template <ITheory... Th_>
    friend std::ostream& operator<<(std::ostream &s, const QuantifiedFormula<Th_...> &f) {
        for (const auto &q: f.prefix) {
            switch (q.getType()) {
            case Quantifier::Type::Exists:
                s << "EX";
                break;
            case Quantifier::Type::Forall:
                s << "ALL";
                break;
            }
            for (const auto &x: q.getVars()) {
                s << " " << x;
                const auto lb = q.lowerBound(x);
                const auto ub = q.upperBound(x);
                if (lb || ub) {
                    s << " in [";
                    if (lb) {
                        s << *lb;
                    } else {
                        s << "-oo";
                    }
                    s << ",";
                    if (ub) {
                        s << *ub;
                    } else {
                        s << "oo";
                    }
                    s << "]";
                }
            }
            s << " . ";
        }
        s << f.matrix;
        return s;
    }

public:

    QuantifiedFormula(std::vector<Quantifier> prefix, const BE &matrix): prefix(prefix), matrix(matrix) {}

    const QF negation() const {
        std::vector<Quantifier> _prefix;
        std::transform(prefix.begin(), prefix.end(), _prefix.begin(), [](const auto &q ){return q.negation();});
        return QF(_prefix, matrix->negation());
    }

    bool forall(const std::function<bool(const Lit&)> &pred) const {
        return matrix->forall(pred);
    }

    QF map(const std::function<BE(const Lit&)> &f) const;

    option<QF> simplify() const {
        return matrix->simplify()->quantify(prefix);
    }

    std::set<NumVar> boundVars() const {
        std::set<NumVar> res;
        for (const Quantifier &q: prefix) {
            res.insert(q.getVars().begin(), q.getVars().end());
        }
        return res;
    }

    QF toG() const {
        return QF(prefix, matrix->toG());
    }

    void collectLits(LS &res) const {
        matrix->collectLits(res);
    }

    std::string toRedlog() const {
        std::string res;
        for (const auto &q: prefix) {
            res += q.toRedlog();
        }
        res += matrix->toRedlog();
        for (const auto &q: prefix) {
            unsigned size = q.getVars().size();
            for (unsigned i = 0; i < size; ++i) {
                res += ")";
            }
        }
        return res;
    }

    bool isTiviallyTrue() const {
        return matrix == True;
    }

    bool isTiviallyFalse() const {
        return matrix == False;
    }

    std::vector<Quantifier> getPrefix() const {
        return prefix;
    }

    BE getMatrix() const {
        return matrix;
    }

    bool isConjunction() const {
        return matrix->isConjunction();
    }

    template <ITheory T>
    QuantifiedFormula<T> transform() const {
        return matrix->template transform<T>().quantify(prefix);
    }

};

template <ITheory... Th>
const BExpr<Th...> operator &(const BExpr<Th...> a, const BExpr<Th...> b) {
    const BoolExpressionSet<Th...> children{a, b};
    return BoolExpression<Th...>::buildAnd(children);
}

template <ITheory... Th>
const BExpr<Th...> operator &(const BExpr<Th...> a, const typename Theory<Th...>::Lit &b) {
    return a & BoolExpression<Th...>::buildTheoryLit(b);
}

template <ITheory... Th>
const BExpr<Th...> operator |(const BExpr<Th...> a, const BExpr<Th...> b) {
    const BoolExpressionSet<Th...> children{a, b};
    return BoolExpression<Th...>::buildOr(children);
}

template <ITheory... Th>
const BExpr<Th...> operator |(const BExpr<Th...> a, const typename Theory<Th...>::Lit b) {
    return a | BoolExpression<Th...>::buildTheoryLit(b);
}

template <ITheory... Th>
const BExpr<Th...> operator !(const BExpr<Th...> a) {
    return a->negation();
}

template <ITheory... Th>
bool operator ==(const BExpr<Th...> a, const BExpr<Th...> b) {
    if (a->getTheoryLit() != b->getTheoryLit()) {
        return false;
    }
    if (a->getTheoryLit()) {
        return true;
    }
    if (a->isAnd() != b->isAnd()) {
        return false;
    }
    return a->getChildren() == b->getChildren();
}

template <ITheory... Th>
bool operator !=(const BExpr<Th...> a, const BExpr<Th...> b) {
    return !(a==b);
}

template <ITheory... Th>
std::ostream& operator<<(std::ostream &s, const BExpr<Th...> e) {
    if (e->getTheoryLit()) {
        std::visit([&s](const auto &lit){s << lit;}, *e->getTheoryLit());
    } else if (e->getChildren().empty()) {
        if (e->isAnd()) {
            s << "TRUE";
        } else {
            s << "FALSE";
        }
    } else {
        bool first = true;
        s << "(";
        for (const auto &c: e->getChildren()) {
            if (first) {
                s << c;
                first = false;
            } else {
                if (e->isAnd()) {
                    s << " /\\ ";
                } else {
                    s << " \\/ ";
                }
                s << c;
            }
        }
        s << ")";
    }
    return s;
}
