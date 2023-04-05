#pragma once

#include "itsproblem.hpp"
#include "linearizingsolver.hpp"
#include "itsproblem.hpp"
#include "proof.hpp"
#include "result.hpp"
#include "redundanceviaautomata.hpp"
#include "complexity.hpp"

#include <limits>
#include <list>
#include <optional>

/**
 * Possible improvements:
 * - remove original clauses that are subsumed by learned clauses
 * - allow disjunctive clauses on the trace and project wrt. the current model before acceleration, not after every step
 */
namespace reachability {

/**
 * Represents a resolution step, i.e., an element of the trace.
 */
struct Step {

    const TransIdx clause_idx;

    /**
     * a conjunction that implies the condition of the clause
     */
    const BoolExpr implicant;

    /**
     * renames the program variables to fresh variables that serve as input for the next step
     */
    const Subs var_renaming;

    const Subs model;

    const Rule resolvent;

    Step(const TransIdx transition, const BoolExpr &sat, const Subs &var_renaming, const Subs &model, const Rule &resolvent);

};

/**
 * Stores which looping sequences of CHCs should be treated as if they were non-looping.
 * Used for sequences whose resolvent is something like f(x) -> f(0). For such clauses,
 * the learned clause would be equivalent to the original clause, and we want to avoid
 * learning many of those useless clauses.
 *
 * TODO Should be sound for SAT, but we have to check that carefully.
 *
 * Additionally, we treat looping sequences where acceleration fails as if they were
 * non-looping.
 *
 * Note that we still mark the corresponding language as redundant. So if [1,2,3] is
 * a non-loop, then we keep using "Step" such that we may eventually obtain [1,2,3,1,2,3].
 * Then the latter is redundant and we backtrack, i.e., non-loops should not be a problem
 * w.r.t. termination.
 */
class NonLoops {

    std::map<std::pair<TransIdx, BoolExpr>, long> alphabet;
    long next_char = 0;
    std::set<std::vector<long>> non_loops;
    const ITSProblem &chcs;

public:

    NonLoops(const ITSProblem &chcs);

    std::vector<long> build(const std::vector<Step> &trace, int backlink);

    void add(const std::vector<Step> &trace, int backlink);

    bool contains(const std::vector<long> &sequence);

    void append(std::vector<long> &sequence, const Step &step);

};

std::ostream& operator<<(std::ostream &s, const Step &step);

// forward declarations of acceleration states
class Succeeded;
class Covered;
class Dropped;
class Failed;
class ProvedUnsat;

/**
 * When learning clauses, an instance of this class is returned.
 * It indicates whether a clause has been learned or not,
 * as well as the reason for not learning a clause.
 */
class LearningState {

protected:
    LearningState();

public:
    /**
     * true if a clause was learned
     */
    virtual std::optional<Succeeded> succeeded();
    /**
     * true if no clause was learned since it would have been redundant
     */
    virtual std::optional<Covered> covered();
    /**
     * True if the learned clause could not be added to the trace without introducing
     * inconsistencies. This may happen when our acceleration technique returns an
     * under-approximation.
     * TODO For sat, the current handling of this case is not sound.
     */
    virtual std::optional<Dropped> dropped();
    /**
     * true if no clause was learned for some other reason
     * TODO We have to think about ways to deal with this case when trying to prove sat.
     */
    virtual std::optional<Failed> failed();

    virtual std::optional<ProvedUnsat> unsat();
};

class Succeeded final: public LearningState {
    /**
     * the indices of the learned clause
     */
    Result<std::vector<std::pair<TransIdx, BoolExpr>>> idx;

public:
    Succeeded(const Result<std::vector<std::pair<TransIdx, BoolExpr>>> &idx);
    std::optional<Succeeded> succeeded() override;
    Result<std::vector<std::pair<TransIdx, BoolExpr>>>& operator*();
    Result<std::vector<std::pair<TransIdx, BoolExpr>>>* operator->();
};

class Covered final: public LearningState {
    std::optional<Covered> covered() override;
};

class Dropped final: public LearningState {

    ITSProof proof;

public:
    Dropped(const ITSProof &proof);
    std::optional<Dropped> dropped() override;
    const ITSProof& get_proof() const;
};

class Failed final: public LearningState {
    std::optional<Failed> failed() override;
};

class ProvedUnsat final: public LearningState {
    ITSProof proof;

public:
    ProvedUnsat(const ITSProof &proof);
    std::optional<ProvedUnsat> unsat() override;
    ITSProof& operator*();
    ITSProof* operator->();
};

/**
 * auxiliary class for incremental SMT via RAII
 */
struct PushPop {
    LinearizingSolver<IntTheory, BoolTheory> &solver;
    PushPop(LinearizingSolver<IntTheory, BoolTheory> &solver);
    ~PushPop();
};

class Reachability {

    ITSProblem &chcs;

    ITSProof proof;

    unsigned smt_timeout = 500u;

    LinearizingSolver<IntTheory, BoolTheory> solver;

    std::vector<Step> trace;

    /**
     * A conjunctive clause x is blocked if find(x) != end().
     * A conjunctive variant y of a non-conjunctive clause x is blocked if cond(y) implies an element of at(x).
     * Maybe it would be better to subdivide the blocking formulas w.r.t. pairs of predicates instead of clauses.
     */
    std::vector<std::map<TransIdx, std::set<BoolExpr>>> blocked_clauses{{}};

    VarSet prog_vars;

    /**
     * clauses up to this one are original ones, all other clauses are learned
     */
    TransIdx last_orig_clause {0};

    std::pair<int, int> luby {1,1};

    unsigned luby_unit {10};

    unsigned luby_loop_count {0};

    void luby_next();

    using Red = RedundanceViaAutomata;
    std::unique_ptr<Red> redundance {std::make_unique<Red>()};
    std::map<std::pair<std::vector<TransIdx>, BoolExpr>, BoolExpr> conditional_redundance;

    NonLoops non_loops;

    Complexity cpx = Complexity::Const;

    bool is_learned_clause(const TransIdx idx) const;

    bool is_orig_clause(const TransIdx idx) const;

    void update_cpx();

    Result<Rule> instantiate(const NumVar &n, const Rule &rule) const;

    /**
     * removes clauses that are not on a CFG-path from a fact to a query
     */
    ResultViaSideEffects remove_irrelevant_clauses();

    /**
     * applies some very basic simplifications
     * TODO Should be sound for sat, but we should check it to be sure.
     */
    ResultViaSideEffects simplify();

    /**
     * resolves recursive clauses with themselves in cases where
     * the resulting clause might be easier to accelerate
     * TODO Not sound for sat.
     */
    ResultViaSideEffects unroll();

    ResultViaSideEffects refine_dependency_graph();

    /**
     * preprocesses the CHC problem
     */
    void preprocess();

    /**
     * initializes all data structures after preprocessing
     */
    void init();

    /**
     * finishes the analysis when we were able to prove unsat
     */
    void unsat();

    /**
     * tries to resolve the trace with the given clause
     */
    std::optional<BoolExpr> resolve(const TransIdx idx);

    /**
     * drops a suffix of the trace, up to the given new size
     */
    void drop_until(const int new_size);

    /**
     * computes (an approximation of) the language associated with the clause used for the given step
     */
    Automaton get_language(const Step &step);

    /**
     * computes (an approximation of) the language associated with the clause that can be learned
     * from the looping suffix of the trace
     * @param backlink the start of the looping suffix of the trace
     */
    Automaton build_language(const int backlink);

    /**
     * computes a clause that is equivalent to the looping suffix of the trace
     * @param backlink the start of the looping suffix of the trace
     */
    std::pair<Rule, Subs> build_loop(const int backlink);

    /**
     * adds a learned clause to all relevant data structures
     * @param lang (an approximation of) the language associated with the learned clause
     */
    TransIdx add_learned_clause(const Rule &clause, const unsigned backlink);

    /**
     * tries to accelerate the given clause
     * @param lang the language associated with the learned clause.
     */
    std::unique_ptr<LearningState> learn_clause(const Rule &rule, const Subs &sample_point, const unsigned backlink);

    /**
     * does everything that needs to be done if the trace has a looping suffix
     */
    std::unique_ptr<LearningState> handle_loop(const unsigned backlink);

    /**
     * @return the start position of the looping suffix of the trace, if any, or -1
     */
    std::optional<unsigned> has_looping_suffix();

    /**
     * Generates a fresh copy of the program variables and fixes their value according to the update of the
     * given clause by adding corresponding constraints to the SMT solver.
     * @return a variable renaming from the program variables to the fresh copy
     */
    Subs handle_update(const TransIdx idx);

    /**
     * blocks the given step
     */
    void block(const Step &step);

    void backtrack();

    /**
     * remove the last element of the trace, and from all other data structures that have to have the same
     * size as the trace
     */
    void pop();

    void add_to_trace(const Step &step);

    Rule compute_resolvent(const TransIdx idx, const BoolExpr &implicant) const;

    /**
     * Assumes that the trace can be resolved with the given clause.
     * Does everything that needs to be done to apply the rule "Step".
     */
    bool store_step(const TransIdx idx, const BoolExpr &implicant);

    void print_trace(std::ostream &s);

    void print_state();

    bool try_to_finish();

    Reachability(ITSProblem &its);

    void analyze();

public:

    static bool log;
    static void analyze(ITSProblem &its);

};

}
