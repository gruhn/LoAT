#ifndef SMT_H
#define SMT_H

#include "../expr/expression.hpp"
#include "../expr/boolexpr.hpp"
#include "../its/variablemanager.hpp"
#include "model.hpp"
#include "../config.hpp"

class Smt
{
public:

    enum Result {Sat, Unknown, Unsat};
    enum Logic {QF_LA, QF_NA, QF_ENA, LA, NA, ENA};

    uint add(const ForAllExpr &e);
    uint add(const BoolExpr &e);
    void push();
    void pop();
    uint add(const Rel &e);
    void resetSolver();
    void enableModels();
    void enableUnsatCores();
    void setTimeout(unsigned int timeout);

    virtual Result check() = 0;
    virtual Model model() = 0;
    virtual std::vector<uint> unsatCore() = 0;
    virtual ~Smt();

    static Smt::Result check(const BoolExpr &e, const VariableManager &varMan);
    static bool isImplication(const BoolExpr &lhs, const BoolExpr &rhs, const VariableManager &varMan);
    static Logic chooseLogic(const std::vector<BoolExpr> &xs, const std::vector<Subs> &up = {});
    static Logic chooseLogic(const std::vector<ForAllExpr> &xs, const std::vector<Subs> &up = {});

    template<class RELS, class UP> static Logic chooseLogic(const std::vector<RELS> &g, const std::vector<UP> &up) {
        Logic res = QF_LA;
        for (const RELS &rels: g) {
            for (const Rel &rel: rels) {
                if (!rel.isLinear()) {
                    if (!rel.isPoly()) {
                        return QF_ENA;
                    }
                    res = QF_NA;
                }
            }
        }
        for (const UP &t: up) {
            if (!t.isLinear()) {
                if (!t.isPoly()) {
                    return QF_ENA;
                }
                res = QF_NA;
            }
        }
        return res;
    }

protected:

    std::vector<BoolExpr> marker;
    std::map<BoolExpr, uint> markerMap;

    bool unsatCores = false;
    bool models = false;
    uint timeout = Config::Smt::DefaultTimeout;

    virtual void _add(const ForAllExpr &e) = 0;
    virtual void _push() = 0;
    virtual void _pop() = 0;
    virtual void _resetSolver() = 0;
    virtual void _resetContext() = 0;
    virtual void updateParams() = 0;

private:

    uint markerCount = 0;
    std::stack<uint> markerStack;

};

#endif // SMT_H
