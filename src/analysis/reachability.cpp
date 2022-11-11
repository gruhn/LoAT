#include "reachability.hpp"
#include "chainstrategy.hpp"
#include "chain.hpp"
#include "preprocess.hpp"
#include "loopacceleration.hpp"
#include "result.hpp"
#include "yices.hpp"
#include "smt.hpp"
#include "export.hpp"
#include "expression.hpp"
#include "boolexpression.hpp"
#include "vector.hpp"

#include <numeric>

bool Reachability::log = false;

LearningState::LearningState() {}

option<Succeeded> LearningState::succeeded() {
    return {};
}

option<Covered> LearningState::covered() {
    return {};
}

option<Dropped> LearningState::dropped() {
    return {};
}

option<Failed> LearningState::failed() {
    return {};
}

Succeeded::Succeeded(const Result<TransIdx> &idx): idx(idx) {}

option<Succeeded> Succeeded::succeeded() {
    return *this;
}

Result<TransIdx>& Succeeded::operator*() {
    return idx;
}

Result<TransIdx>* Succeeded::operator->() {
    return &idx;
}

option<Covered> Covered::covered() {
    return *this;
}

option<Dropped> Dropped::dropped() {
    return *this;
}

option<Failed> Failed::failed() {
    return *this;
}

Reachability::Reachability(ITSProblem &its): chcs(its), z3(its) {
    z3.enableModels();
}

Step::Step(const TransIdx transition, const BoolExpr &sat, const Subs &var_renaming, const ThModel &model):
    clause_idx(transition),
    implicant(sat),
    var_renaming(var_renaming),
    model(model.toSubs()){}

std::ostream& operator<<(std::ostream &s, const std::vector<Step> &step) {
    for (auto it = step.begin(); it != step.end(); ++it) {
        s << " [" << it->implicant << "] " << it->clause_idx;
    }
    return s;
}

ResultViaSideEffects Reachability::remove_irrelevant_clauses() {
    std::set<LocationIdx> keep;
    std::stack<LocationIdx> todo;
    todo.push(*chcs.getSink());
    do {
        const LocationIdx current = todo.top();
        todo.pop();
        keep.insert(current);
        for (const auto p: chcs.getPredecessorLocations(current)) {
            if (keep.find(p) == keep.end()) {
                todo.push(p);
            }
        }
    } while (!todo.empty());
    std::set<TransIdx> deleted;
    for (const auto idx: chcs.getAllTransitions()) {
        const LocationIdx target = chcs.getRule(idx).getRhsLoc(0);
        if (keep.find(target) == keep.end()) {
            const std::set<TransIdx> d = chcs.removeLocationAndRules(target);
            deleted.insert(d.begin(), d.end());
        }
    }
    ResultViaSideEffects ret;
    if (!deleted.empty()) {
        ret.succeed();
        ret.deletionProof(deleted);
    }
    return ret;
}

ResultViaSideEffects Reachability::simplify() {
    ResultViaSideEffects ret;
    for (const TransIdx idx: chcs.getAllTransitions()) {
        const auto res = Preprocess::preprocessRule(chcs, chcs.getRule(idx));
        if (res) {
            ret.succeed();
            chcs.replaceRules({idx}, {res.get()});
            ret.concat(res.getProof());
        }
    }
    return ret;
}


// TODO under-approximation?
ResultViaSideEffects Reachability::unroll() {
    ResultViaSideEffects ret;
    for (const TransIdx idx: chcs.getAllTransitions()) {
        const Rule &r = chcs.getRule(idx);
        if (r.isSimpleLoop()) {
            const auto [res, period] = LoopAcceleration::chain(r.toLinear(), chcs);
            if (period > 1) {
                const auto simplified = Preprocess::simplifyRule(chcs, res, true);
                ret.succeed();
                ret.ruleTransformationProof(r, "unrolling", res, chcs);
                if (simplified) {
                    chcs.replaceRules({idx}, {*simplified});
                    ret.concat(simplified.getProof());
                } else {
                    chcs.replaceRules({idx}, {res});
                }
            }
        }
    }
    return ret;
}

bool Reachability::leaves_scc(const TransIdx idx) const {
    const Rule &r = chcs.getRule(idx);
    return sccs.getSccIndex(r.getLhsLoc()) != sccs.getSccIndex(r.getRhsLoc(0));
}

int Reachability::has_looping_suffix() {
    if (trace.empty()) {
        return -1;
    }
    const LocationIdx dst = chcs.getRule(trace.back().clause_idx).getRhsLoc(0);
    for (int pos = trace.size() - 1; pos >= 0; --pos) {
        const Step step = trace[pos];
        if (leaves_scc(step.clause_idx)) {
            return -1;
        }
        if (chcs.getRule(step.clause_idx).getLhsLoc() == dst) {
            bool looping = static_cast<unsigned>(pos) < trace.size() - 1 || is_orig_clause(step.clause_idx);
            if (looping) {
                return pos;
            }
        }
    }
    return -1;
}

Subs Reachability::handle_update(const TransIdx idx) {
    const Rule &r = chcs.getRule(idx);
    RelSet eqs;
    const Subs old_sigma = trace.empty() ? Subs() : trace.back().var_renaming;
    Subs new_sigma;
    const Subs up = r.getUpdate(0);
    for (const auto &v: prog_vars) {
        if (std::holds_alternative<NumVar>(v)) {
            const auto &var = std::get<NumVar>(v);
            const auto x = chcs.getFreshUntrackedSymbol<IntTheory>(var.getName(), Expr::Int);
            z3.add(boolExpression::build(Rel::buildEq(x, up.get<IntTheory>(var).subs(old_sigma.get<IntTheory>()))));
            new_sigma.put<IntTheory>(var, x);
        } else if (std::holds_alternative<BoolVar>(v)) {
            const auto &var = std::get<BoolVar>(v);
            const auto x = chcs.getFreshUntrackedSymbol<BoolTheory>(var.getName(), Expr::Bool);
            const auto lhs = boolExpression::build(x);
            const auto rhs = up.get<BoolTheory>(var)->subs(old_sigma);
            z3.add((lhs & rhs) | ((!lhs) & (!rhs)));
            new_sigma.put<BoolTheory>(var, lhs);
        }
    }
    for (const auto &var: r.vars()) {
        if (chcs.isTempVar(var)) {
            if (std::holds_alternative<NumVar>(var)) {
                new_sigma.put<IntTheory>(std::get<NumVar>(var), chcs.getFreshUntrackedSymbol<IntTheory>(variable::getName(var), Expr::Int));
            } else if (std::holds_alternative<BoolVar>(var)) {
                new_sigma.put<BoolTheory>(std::get<BoolVar>(var), boolExpression::build(chcs.getFreshUntrackedSymbol<BoolTheory>(variable::getName(var), Expr::Bool)));
            } else {
                throw std::logic_error("unsupported theory");
            }
        }
    }
    return new_sigma;
}

void Reachability::block(const Step &step) {
    if (log) std::cout << "blocking " << step.clause_idx << ", " << step.implicant << std::endl;
    if (chcs.getRule(step.clause_idx).getGuard()->isConjunction()) {
        blocked_clauses.back()[step.clause_idx] = {};
    }
    auto block = blocked_clauses.back().find(step.clause_idx);
    if (block == blocked_clauses.back().end()) {
        blocked_clauses.back()[step.clause_idx] = {step.implicant};
    } else {
        block->second.insert(step.implicant);
    }
}

void Reachability::pop() {
    blocked_clauses.pop_back();
    trace.pop_back();
    z3.pop();
    proof.pop();
}

void Reachability::backtrack() {
    if (log) std::cout << "backtracking" << std::endl;
    const Step step = trace.back();
    pop();
    block(step);
}

void Reachability::add_to_trace(const Step &step) {
    if (!trace.empty()) {
        assert(chcs.getRule(trace.back().clause_idx).getRhsLoc(0) == chcs.getRule(step.clause_idx).getLhsLoc());
    }
    trace.emplace_back(step);
    proof.push();
    proof.headline("extended trace");
    proof.append(std::stringstream() << "added " << step.clause_idx << " to trace");
    Proof subProof;
    subProof.section("implicant");
    subProof.append(std::stringstream() << step.implicant);
    subProof.section("trace");
    subProof.append(std::stringstream() << trace);
    proof.storeSubProof(subProof);
}

void Reachability::store_step(const TransIdx idx, const BoolExpr &implicant) {
    const auto model = z3.model();
    if (trace.empty()) {
        z3.add(implicant);
    } else {
        z3.add(implicant->subs(trace.back().var_renaming));
    }
    const auto new_var_renaming = handle_update(idx);
    const Step step(idx, implicant, new_var_renaming, model);
    add_to_trace(step);
    blocked_clauses.push_back({});
}

void Reachability::print_trace(std::ostream &s) {
    for (const auto &step: trace) {
        s << " [";
        for (const auto &x: prog_vars) {
            s << " " << x << "=" << expression::subs(step.var_renaming.get(x), step.model);
        }
        s << " ] " << step.clause_idx;
    }
    s << std::endl;
}

void Reachability::preprocess() {
    ResultViaSideEffects res = remove_irrelevant_clauses();
    if (res) {
        proof.majorProofStep("removed irrelevant transitions", chcs);
        proof.storeSubProof(res.getProof());
    }
    res = Chaining::chainLinearPaths(chcs);
    if (res) {
        proof.majorProofStep("chained linear paths", chcs);
        proof.storeSubProof(res.getProof());
    }
    res = simplify();
    if (res) {
        proof.majorProofStep("simplified transitions", chcs);
        proof.storeSubProof(res.getProof());
    }
    res = unroll();
    if (res) {
        proof.majorProofStep("unrolled loops", chcs);
        proof.storeSubProof(res.getProof());
    }
    if (log) {
        std::cout << "simplified ITS" << std::endl;
        ITSExport::printForProof(chcs, std::cout);
    }
}

void Reachability::update_rules(const LocationIdx idx) {
    auto it = cross_scc.find(idx);
    std::list<LocationIdx> trans;
    if (it != cross_scc.end()) {
        trans.insert(trans.end(), it->second.begin(), it->second.end());
    }
    it = learned_clauses.find(idx);
    if (it != learned_clauses.end()) {
        trans.insert(trans.end(), it->second.begin(), it->second.end());
    }
    it = in_scc.find(idx);
    if (it != in_scc.end()) {
        trans.insert(trans.end(), it->second.begin(), it->second.end());
    }
    rules[idx] = trans;
}

void Reachability::init() {
    for (const auto &x: chcs.getVars()) {
        if (!chcs.isTempVar(x)) {
            prog_vars.insert(x);
        }
    }
    for (const TransIdx idx: chcs.getAllTransitions()) {
        last_orig_clause = std::max(last_orig_clause, idx);
        const auto rule = chcs.getRule(idx);
        const auto src = rule.getLhsLoc();
        const auto dst = rule.getRhsLoc(0);
        if (src == chcs.getInitialLocation() && dst == chcs.getSink()) {
            conditional_empty_clauses.push_back(idx);
        } else {
            std::map<LocationIdx, std::vector<TransIdx>> *to_insert;
            if (dst == chcs.getSink()) {
                to_insert = &queries;
            } else if (leaves_scc(idx)) {
                to_insert = &cross_scc;
            } else {
                to_insert = &in_scc;
            }
            const auto it = to_insert->find(src);
            if (it == to_insert->end()) {
                to_insert->emplace(src, std::vector<TransIdx>{idx});
            } else {
                it->second.push_back(idx);
            }
        }
    }
    for (const auto loc: chcs.getLocations()) {
        update_rules(loc);
    }
}

void Reachability::unsat() {
    std::cout << "unsat" << std::endl << std::endl;
    std::stringstream trace_stream, counterexample;
    trace_stream << trace;
    print_trace(counterexample);
    if (log) {
        std::cout << "final ITS:" << std::endl;
        ITSExport::printForProof(chcs, std::cout);
        std::cout << std::endl << "final trace:" << trace_stream.str() << std::endl << std::endl;
        std::cout << "counterexample:" << counterexample.str();
    }
    proof.headline("proved unsatisfiability");
    Proof subProof;
    subProof.section("final trace");
    subProof.append(trace_stream);
    subProof.section("counterexample");
    subProof.append(counterexample);
    proof.storeSubProof(subProof);
    proof.print();
}

option<BoolExpr> Reachability::resolve(const TransIdx idx) {
    const auto sigma = trace.empty() ? Subs() : trace.back().var_renaming;
    const Rule r = chcs.getRule(idx);
    const auto block = blocked_clauses.back().find(idx);
    if (block != blocked_clauses.back().end()) {
        if (r.getGuard()->isConjunction()) {
            return {};
        }
        for (const auto &b: block->second) {
            z3.add(!b->subs(sigma));
        }
    }
    z3.add(r.getGuard()->subs(sigma));
    if (z3.check() == Sat) {
        if (log) std::cout << "found model for " << idx << std::endl;
        // Z3's models get huge, but we are only interested in those variables that occur in
        // the guard or sigma
        VarSet vars = r.getGuard()->vars();
        substitution::collectVariables(sigma, vars);
        const auto implicant = r.getGuard()->implicant(sigma.compose(z3.model().toSubs(vars)));
        if (implicant) {
            return BExpression::buildAndFromLits(*implicant);
        } else {
            throw std::logic_error("model, but no implicant");
        }
    } else {
        if (log) std::cout << "could not find a model for " << idx << std::endl;
        return {};
    }
}

void Reachability::drop_until(const int new_size) {
    while (trace.size() > static_cast<unsigned>(new_size)) {
        pop();
    }
}

Reachability::Red::T Reachability::get_language(const Step &step) {
    if (is_orig_clause(step.clause_idx)) {
        return redundance->get_singleton_language(step.clause_idx, step.implicant->conjunctionToGuard());
    } else {
        return redundance->get_language(step.clause_idx);
    }
}

Reachability::Red::T Reachability::build_language(const int backlink) {
    std::vector<long> automaton = get_language(trace.back());
    for (int i = trace.size() - 2; i >= backlink; --i) {
        redundance->concat(automaton, get_language(trace[i]));
    }
    redundance->transitive_closure(automaton);
    return automaton;
}

Rule Reachability::build_loop(const int backlink) {
    Rule loop = chcs.getRule(trace.back().clause_idx).withGuard(trace.back().implicant);
    for (int i = trace.size() - 2; i >= backlink; --i) {
        const Step &step = trace[i];
        auto sigma = step.var_renaming;
        sigma.erase(prog_vars); // rename temporary variables before chaining
        const TransIdx t = step.clause_idx;
        const Rule &r = chcs.getRule(t);
        loop = *Chaining::chainRules(chcs, r.withGuard(step.implicant).subs(sigma), loop, false);
    }
    assert(loop.getLhsLoc() == loop.getRhsLoc(0));
    if (log) {
        std::cout << "found loop of length " << (trace.size() - backlink) << ":" << std::endl;
        ITSExport::printRule(loop, chcs, std::cout);
        std::cout << std::endl;
    }
    return loop;
}

TransIdx Reachability::add_learned_clause(const Rule &accel, const Red::T &lang) {
    const auto loop_idx = chcs.addRule(accel);
    redundance->set_language(loop_idx, lang);
    const auto src = accel.getLhsLoc();
    auto acc_it = learned_clauses.find(src);
    if (acc_it == learned_clauses.end()) {
        learned_clauses.emplace(src, std::vector<TransIdx>{loop_idx});
    } else {
        acc_it->second.push_back(loop_idx);
    }
    update_rules(src);
    return loop_idx;
}

bool Reachability::is_learned_clause(const TransIdx idx) const {
    return idx > last_orig_clause;
}

bool Reachability::is_orig_clause(const TransIdx idx) const {
    return idx <= last_orig_clause;
}

std::unique_ptr<LearningState> Reachability::learn_clause(const Rule &rule, const Red::T &automaton, const int backlink) {
    Result<Rule> res {Preprocess::simplifyRule(chcs, rule, true)};
    AccelerationResult accel_res = LoopAcceleration::accelerate(chcs, res->toLinear(), -1, Complexity::Const);
    if (accel_res.rule) {
        // acceleration succeeded, simplify the result
        const auto simplified = Preprocess::simplifyRule(chcs, *accel_res.rule, true);
        // TODO problematic for SAT
        if (simplified->getUpdate(0) == res->getUpdate(0)) {
            bool orig = true;
            for (size_t i = backlink; i < trace.size(); ++i) {
                if (is_learned_clause(trace[i].clause_idx)) {
                    orig = false;
                    break;
                }
            }
            if (orig) {
                if (log) std::cout << "acceleration yielded equivalent rule, keeping preprocessed rule" << std::endl;
                res.succeed();
            } else {
                if (log) std::cout << "acceleration yielded equivalent rule -> dropping it" << std::endl;
                return std::make_unique<Failed>();
            }
        } else {
            // accelerated rule differs from the original one, update the result
            res = *accel_res.rule;
            res.storeSubProof(accel_res.proof);
            res.concat(simplified);
        }
        if (log) {
            std::cout << "accelerated rule:" << std::endl;
            ITSExport::printRule(*res, chcs, std::cout);
            std::cout << std::endl;
        }
    } else {
        if (log) std::cout << "acceleration failed" << std::endl;
        return std::make_unique<Failed>();
    }
    return std::make_unique<Succeeded>(res.map<TransIdx>([this, &automaton](const auto &x){
        return add_learned_clause(x, automaton);
    }));
}

std::unique_ptr<LearningState> Reachability::handle_loop(const int backlink) {
    const Step step = trace[backlink];
    const auto automaton = build_language(backlink);
    if (redundance->is_redundant(automaton)) {
        if (log) std::cout << "loop already covered" << std::endl;
        return std::make_unique<Covered>();
    } else if (log) {
        std::cout << "learning clause for the following language:" << std::endl;
        std::cout << automaton << std::endl;
    }
    redundance->mark_as_redundant(automaton);
    const auto loop = build_loop(backlink);
    if (loop.getUpdate(0).empty()) {
        std::cout << "dropping trivial loop" << std::endl;
        return std::make_unique<Covered>();
    }
    // TODO bad idea for SAT
    // for large-degree polynomials, Z3 tends to get stalled, irrespectively of its timeout
    if (loop.getUpdate(0).isPoly() && !loop.getUpdate(0).isPoly(10)) {
        return std::make_unique<Failed>();
    }
    bool orig = static_cast<size_t>(backlink) == trace.size() - 1;
    auto state = learn_clause(loop, automaton, orig);
    if (!state->succeeded()) {
        return state;
    }
    auto accel_state = *state->succeeded();
    const auto idx = **accel_state;
    const auto subproof = accel_state->getProof();
    const auto accel = chcs.getRule(idx);
    drop_until(backlink);
    proof.majorProofStep("accelerated loop", chcs);
    proof.storeSubProof(subproof);
    z3.push();
    // TODO this makes little sense for loops with more than one clause
    z3.add(accel.getGuard()->subs(trace.back().var_renaming));
    if (z3.check() == Sat) {
        store_step(idx, accel.getGuard());
        proof.append(std::stringstream() << "added " << idx << " to trace");
        return state;
    } else {
        if (log) std::cout << "applying accelerated rule failed" << std::endl;
        z3.pop();
        return std::make_unique<Dropped>();
    }
}

LocationIdx Reachability::get_current_predicate() const {
    return trace.empty() ? chcs.getInitialLocation() : chcs.getRule(trace.back().clause_idx).getRhsLoc(0);
}

bool Reachability::try_queries(const std::vector<TransIdx> &queries) {
    for (const auto &q: queries) {
        z3.push();
        const option<BoolExpr> sat = resolve(q);
        if (sat) {
            // no need to compute the model and the variable renaming for the next step, as we are done
            const ThModel model;
            const Subs new_var_renaming;
            const Step step(q, *sat, new_var_renaming, model);
            add_to_trace(step);
            unsat();
            return true;
        }
        z3.pop();
    }
    return false;
}

bool Reachability::try_queries() {
    return try_queries(queries[get_current_predicate()]);
}

bool Reachability::try_conditional_empty_clauses() {
    return try_queries(conditional_empty_clauses);
}

void Reachability::analyze() {
    proof.majorProofStep("initial ITS", chcs);
    if (log) {
        std::cout << "initial ITS" << std::endl;
        ITSExport::printForProof(chcs, std::cout);
    }
    preprocess();
    init();
    if (try_conditional_empty_clauses()) {
        return;
    }
    do {
        if (log) std::cout << "trace: " << trace << std::endl;
        for (int backlink = has_looping_suffix(); backlink >= 0; backlink = has_looping_suffix()) {
            const Step step = trace[backlink];
            bool simple_loop = static_cast<unsigned>(backlink) == trace.size() - 1;
            auto state = handle_loop(backlink);
            if (state->covered()) {
                backtrack();
            } else if (state->succeeded()) {
                if (simple_loop) {
                    block(step);
                }
                // make sure that we do not apply the accelerated transition again straight away, which is redundant
                block(trace.back());
                // try to apply a query before doing another step
                if (try_queries()) {
                    return;
                }
            } else if (state->dropped()) {
                block(step);
            } else if (state->failed()) {
                // acceleration failed, loop has been added to Automaton::covered so that it won't be unrolled again
                // try to continue instead of backtracking immediately
                break;
            }
        }
        auto &trans = rules[get_current_predicate()];
        auto it = trans.begin();
        std::vector<TransIdx> append;
        while (it != trans.end()) {
            z3.push();
            const option<BoolExpr> sat = resolve(*it);
            if (!sat) {
                z3.pop();
                append.push_back(*it);
                it = trans.erase(it);
            } else {
                // block learned clauses after adding them to the trace
                if (is_learned_clause(*it)) {
                    block(trace.back());
                }
                store_step(*it, *sat);
                break;
            }
        }
        trans.insert(trans.end(), append.begin(), append.end());
        if (!trace.empty()) {
            if (it == trans.end()) {
                backtrack();
            } else {
                // check whether a query is applicable after every step and,
                // importantly, before acceleration (which might approximate)
                if (try_queries()) {
                    return;
                }
            }
        }
    } while (!trace.empty());
    std::cout << "unknown" << std::endl << std::endl;
}

void Reachability::analyze(ITSProblem &its) {
    yices::init();
    Reachability(its).analyze();
    yices::exit();
}
