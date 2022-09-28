#include "reachability.hpp"
#include "chainstrategy.hpp"
#include "chain.hpp"
#include "preprocess.hpp"
#include "loopacceleration.hpp"
#include "result.hpp"
#include "yices.hpp"
#include "export.hpp"

#include <numeric>

Reachability::Reachability(ITSProblem &its): its(its), z3(its) {
    z3.enableModels();
}

Step::Step(const TransIdx transition, const BoolExpr &sat): transition(transition), sat(sat) {}

std::ostream& operator<<(std::ostream &s, const std::vector<Step> &step) {
    for (auto it = step.begin(); it != step.end(); ++it) {
        s << " [" << it->sat << "] " << it->transition;
    }
    return s;
}

ResultViaSideEffects Reachability::removeIrrelevantTransitions() {
    std::set<LocationIdx> keep;
    std::stack<LocationIdx> todo;
    todo.push(*its.getSink());
    do {
        const LocationIdx current = todo.top();
        todo.pop();
        keep.insert(current);
        for (const auto p: its.getPredecessorLocations(current)) {
            if (keep.find(p) == keep.end()) {
                todo.push(p);
            }
        }

    } while (!todo.empty());
    std::set<TransIdx> deleted;
    for (const auto idx: its.getAllTransitions()) {
        const LocationIdx target = its.getRule(idx).getRhsLoc(0);
        if (keep.find(target) == keep.end()) {
            const std::set<TransIdx> d = its.removeLocationAndRules(target);
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
    for (const TransIdx idx: its.getAllTransitions()) {
        const auto res = Preprocess::preprocessRule(its, its.getRule(idx));
        if (res) {
            ret.succeed();
            its.replaceRules({idx}, {res.get()});
            ret.concat(res.getProof());
        }
    }
    return ret;
}

Rule Reachability::rename_tmp_vars(const Rule &rule) {
    Subs sigma;
    for (const auto &x: rule.vars()) {
        if (its.isTempVar(x)) {
            sigma.put(x, its.addFreshTemporaryVariable(x.get_name()));
        }
    }
    return rule.subs(sigma);
}

Result<Rule> Reachability::unroll(const Rule &rule) {
    Result<Rule> res(rule);
    // chain if there are updates like x = -x + p
    for (const auto &p: rule.getUpdate(0).getExprSubs()) {
        const Var var = p.first;
        const Expr up = p.second.expand();
        const VarSet upVars = up.vars();
        if (upVars.find(var) != upVars.end()) {
            if (up.isPoly() && up.degree(var) == 1) {
                const Expr coeff = up.coeff(var);
                if (coeff.isRationalConstant() && coeff.toNum().is_negative()) {
                    const Rule c = *Chaining::chainRules(its, *res, rename_tmp_vars(*res), false);
                    res.set(c.toLinear());
                    break;
                }
            }
        }
    }
    // chain if there are updates like x = y; y = x
    VarMap<unsigned> cycleLength;
    auto up = res->getUpdate(0).getExprSubs();
    for (const auto &p: up) {
        VarSet vars = p.second.vars();
        unsigned oldSize = 0;
        unsigned count = 0;
        while (oldSize != vars.size() && vars.find(p.first) == vars.end()) {
            oldSize = vars.size();
            count++;
            for (const auto& var: vars) {
                const auto it = up.find(var);
                if (it != up.end()) {
                    const auto newVars = it->second.vars();
                    vars.insert(newVars.begin(), newVars.end());
                }
            }
        }
        if (vars.find(p.first) != vars.end() && count > 0) {
            cycleLength[p.first] = count;
        }
    }
    if (!cycleLength.empty()) {
        unsigned lcm = 1;
        for (const auto &e: cycleLength) {
            lcm = std::lcm(lcm, e.second);
        }
        for (unsigned i = 0; i < lcm; ++i) {
            const Rule c = *Chaining::chainRules(its, *res, rename_tmp_vars(*res), false);
            res.set(c.toLinear());
        }
    }
    // chain if it eliminates variables from an update
    bool changed;
    do {
        changed = false;
        up = res->getUpdate(0).getExprSubs();
        for (const auto &p: up) {
            VarSet varsOneStep = p.second.vars();
            VarSet varsTwoSteps;
            for (const auto &var: varsOneStep) {
                const auto it = up.find(var);
                if (it != up.end()) {
                    const auto newVars = it->second.vars();
                    varsTwoSteps.insert(newVars.begin(), newVars.end());
                } else {
                    varsTwoSteps.insert(var);
                }
            }
            if (varsTwoSteps.size() < varsOneStep.size() && std::includes(varsOneStep.begin(), varsOneStep.end(), varsTwoSteps.begin(), varsTwoSteps.end())) {
                const Rule c = *Chaining::chainRules(its, *res, rename_tmp_vars(*res), false);
                res.set(c.toLinear());
                changed = true;
                break;
            }
        }
    } while (changed);
    if (res) {
        res.ruleTransformationProof(rule, "unrolling", *res, its);
    }
    return res;
}

ResultViaSideEffects Reachability::unroll() {
    ResultViaSideEffects ret;
    for (const TransIdx idx: its.getAllTransitions()) {
        const Rule &r = its.getRule(idx);
        if (r.isSimpleLoop()) {
            const auto res = unroll(r);
            if (res) {
                const auto simplified = Preprocess::simplifyRule(its, *res, true);
                ret.succeed();
                ret.concat(res.getProof());
                if (simplified) {
                    its.replaceRules({idx}, {*simplified});
                    ret.concat(simplified.getProof());
                } else {
                    its.replaceRules({idx}, {*res});
                }
            }
        }
    }
    return ret;
}

BoolExpr Reachability::project(const TransIdx idx) {
    RelSet res;
    const Subs model = sigmas.back().compose(z3.model().toSubs());
    for (const auto &rel: its.getRule(idx).getGuard()->lits()) {
        if (model(rel).isTriviallyTrue()) {
            res.insert(rel);
        }
    }
    return buildAnd(res);
}

bool Reachability::covers(const Subs &model, const BoolExpr &rels) const {
    return Smt::check(model(rels), its) == Smt::Sat;
}

bool Reachability::leaves_scc(const TransIdx idx) const {
    const Rule &r = its.getRule(idx);
    return sccs.getSccIndex(r.getLhsLoc()) != sccs.getSccIndex(r.getRhsLoc(0));
}

int Reachability::is_loop(const TransIdx idx) {
    const Subs model = z3.model().toSubs();
    const LocationIdx dst = its.getRule(idx).getRhsLoc(0);
    for (int pos = trace.size() - 1; pos >= 0; --pos) {
        const Step step = trace[pos];
        if (leaves_scc(step.transition) || !covers(sigmas[pos].concat(model), step.sat)) {
            return -1;
        }
        if (its.getRule(step.transition).getLhsLoc() == dst) {
            bool non_simple = static_cast<unsigned>(pos) < trace.size() - 1;
            bool orig_rule = idx <= lastOrigRule;
            if ((non_simple || orig_rule)) {
                return pos;
            }
        }
    }
    return -1;
}

Automaton Reachability::singleton_automaton(const TransIdx idx) const {
    Automaton res;
    res.StateNamesEnabled(false);
    faudes::EventSet events;
    events.Insert(res.InsEvent(std::to_string(idx)));
    faudes::AlphabetLanguage(events, res);
    return res;
}

bool Reachability::is_covered(const Automaton &automaton, const Subs &model) const {
    return std::any_of(covered.begin(), covered.end(), [&](const auto &p) {
        return faudes::LanguageInclusion(automaton, p.first) && covers(model, p.second);
    });
}

void Reachability::handle_update(const TransIdx idx) {
    const Rule &r = its.getRule(idx);
    RelSet eqs;
    const Subs &oldSigma = sigmas.back();
    Subs newSigma;
    const Subs up = r.getUpdate(0);
    for (const auto &var: prog_vars) {
        const Var x = its.getFreshUntrackedSymbol(var.get_name(), Expr::Int);
        z3.add(buildTheoryLit(Rel::buildEq(x, oldSigma(up(var)))));
        newSigma.put(var, x);
    }
    for (const auto &var: r.vars()) {
        if (its.isTempVar(var)) {
            newSigma.put(var, its.getFreshUntrackedSymbol(var.get_name(), Expr::Int));
        }
    }
    sigmas.push_back(newSigma);
}

void Reachability::do_block(const Step &step) {
    if (log) std::cout << "blocking " << step.transition << ", " << step.sat << std::endl;
    auto block = blocked.back().find(step.transition);
    if (block == blocked.back().end()) {
        blocked.back()[step.transition] = {step.sat};
    } else {
        block->second.insert(step.sat);
    }
}

void Reachability::pop() {
    sigmas.pop_back();
    blocked.pop_back();
    trace.pop_back();
    z3.pop();
    proof.pop();
}

void Reachability::backtrack() {
    if (log) std::cout << "backtracking" << std::endl;
    const Step step = trace.back();
    pop();
    do_block(step);
}

void Reachability::extend_trace(const TransIdx idx, const BoolExpr &sat) {
    if (!trace.empty()) {
        assert(its.getRule(trace.back().transition).getRhsLoc(0) == its.getRule(idx).getLhsLoc());
    }
    trace.emplace_back(idx, sat);
    proof.push();
    proof.headline("extended trace");
    proof.append(std::stringstream() << "added " << idx << " to trace");
    Proof subProof;
    subProof.section("sat");
    subProof.append(std::stringstream() << sat);
    subProof.section("trace");
    subProof.append(std::stringstream() << trace);
    proof.storeSubProof(subProof);
}

void Reachability::store(const TransIdx idx, const BoolExpr &sat) {
    extend_trace(idx, sat);
    blocked.push_back({});
    z3.add(sigmas.back()(sat));
    handle_update(idx);
}

void Reachability::print_run(std::ostream &s) {
    auto it = sigmas.begin();
    const Subs model = z3.model().toSubs();
    for (const auto &step: trace) {
        s << " [";
        for (const auto &x: prog_vars) {
            s << " " << x << "=" << model((*it)(x));
        }
        ++it;
        s << " ] " << step.transition;
    }
    s << std::endl;
}

void Reachability::preprocess() {
    ResultViaSideEffects res = removeIrrelevantTransitions();
    if (res) {
        proof.majorProofStep("removed irrelevant transitions", its);
        proof.storeSubProof(res.getProof());
    }
    res = Chaining::chainLinearPaths(its);
    if (res) {
        proof.majorProofStep("chained linear paths", its);
        proof.storeSubProof(res.getProof());
    }
    res = simplify();
    if (res) {
        proof.majorProofStep("simplified transitions", its);
        proof.storeSubProof(res.getProof());
    }
    res = unroll();
    if (res) {
        proof.majorProofStep("unrolled loops", its);
        proof.storeSubProof(res.getProof());
    }
    if (log) {
        std::cout << "simplified ITS" << std::endl;
        ITSExport::printForProof(its, std::cout);
    }
}

void Reachability::update_transitions(const LocationIdx loc) {
    auto it = cross_scc.find(loc);
    std::list<LocationIdx> trans;
    if (it != cross_scc.end()) {
        trans.insert(trans.end(), it->second.begin(), it->second.end());
    }
    it = accelerated.find(loc);
    if (it != accelerated.end()) {
        trans.insert(trans.end(), it->second.begin(), it->second.end());
    }
    it = in_scc.find(loc);
    if (it != in_scc.end()) {
        trans.insert(trans.end(), it->second.begin(), it->second.end());
    }
    transitions[loc] = trans;
}

void Reachability::init() {
    Subs sigma;
    for (const auto &x: its.getVars()) {
        if (!its.isTempVar(x)) {
            sigma.put(x, x);
            prog_vars.insert(x);
        }
    }
    sigmas.push_back(sigma);
    for (const TransIdx idx: its.getAllTransitions()) {
        regexes[idx] = singleton_automaton(idx);
        lastOrigRule = std::max(lastOrigRule, idx);
        const TransIdx src = its.getRule(idx).getLhsLoc();
        auto *to_insert = leaves_scc(idx) ? &cross_scc : &in_scc;
        const auto it = to_insert->find(src);
        if (it == to_insert->end()) {
            to_insert->emplace(src, std::vector<TransIdx>{idx});
        } else {
            it->second.push_back(idx);
        }
    }
    for (const auto loc: its.getLocations()) {
        update_transitions(loc);
    }
}

void Reachability::unsat() {
    std::cout << "unsat" << std::endl << std::endl;
    std::stringstream trace_stream, counterexample;
    trace_stream << trace;
    print_run(counterexample);
    if (log) {
        std::cout << "final ITS:" << std::endl;
        ITSExport::printForProof(its, std::cout);
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

option<BoolExpr> Reachability::do_step(const TransIdx idx) {
    Subs sigma = sigmas.back();
    const Rule r = its.getRule(idx);
    BoolExpr g = sigma(r.getGuard());
    z3.add(g);
    const auto block = blocked.back().find(idx);
    if (block != blocked.back().end()) {
        for (const auto &b: block->second) {
            z3.add(!sigma(b));
        }
    }
    if (z3.check() == Smt::Sat) {
        if (log) std::cout << "found model for " << idx << std::endl;
        return project(idx);
    } else {
        if (log) std::cout << "could not find a model for " << idx << std::endl;
        return {};
    }
}

void Reachability::drop_loop(const int backlink) {
    while (trace.size() > static_cast<unsigned>(backlink)) {
        pop();
    }
}

std::pair<Rule, Automaton> Reachability::build_loop(const int backlink) {
    Automaton automaton = regexes[trace.back().transition];
    Rule loop = its.getRule(trace.back().transition).withGuard(trace.back().sat);
    for (int i = trace.size() - 2; i >= backlink; --i) {
        const Step &step = trace[i];
        const TransIdx t = step.transition;
        const Rule &r = its.getRule(t);
        loop = *Chaining::chainRules(its, r.withGuard(step.sat), loop, false);
        faudes::LanguageConcatenate(regexes[t], automaton, automaton);
    }
    assert(loop.getLhsLoc() == loop.getRhsLoc(0));
    if (log) {
        std::cout << "found loop:" << std::endl;
        ITSExport::printRule(loop, its, std::cout);
        std::cout << std::endl;
    }
    // this is required to bypass a bug in faudes:
    // KleeneClosure does not work if the initial
    // state is not marked
    automaton.SetMarkedState(automaton.InitState());
    faudes::KleeneClosure(automaton);
    faudes::StateMin(automaton, automaton);
    return {loop, automaton};
}

Result<Rule> Reachability::preprocess_loop(const Rule &loop) {
    Result<Rule> res(loop);
    const Result<Rule> simplified = Preprocess::simplifyRule(its, *res, true);
    if (simplified) {
        res = *simplified;
        res.storeSubProof(simplified.getProof());
        if (log) {
            std::cout << "simplified loop:" << std::endl;
            ITSExport::printRule(*res, its, std::cout);
            std::cout << std::endl;
        }
    }
    const Result<Rule> unrolled = unroll(*res);
    if (unrolled) {
        res = *unrolled;
        res.storeSubProof(unrolled.getProof());
        if (log) {
            std::cout << "unrolled loop:" << std::endl;
            ITSExport::printRule(*res, its, std::cout);
            std::cout << std::endl;
        }
    }
    return res;
}

Reachability::State Reachability::handle_loop(const int backlink) {
    const Step step = trace[backlink];
    const auto p = build_loop(backlink);
    Rule loop = p.first;
    const Automaton automaton = p.second;
    const Subs old_model = sigmas[backlink].concat(z3.model().toSubs());
    if (is_covered(automaton, old_model)) {
        if (log) std::cout << "loop already covered" << std::endl;
        return Failed;
    }
    covered.emplace_back(automaton, loop.getGuard());
    Proof accel_proof;
    const Result<Rule> preprocessed = preprocess_loop(loop);
    if (preprocessed) {
        accel_proof.concat(preprocessed.getProof());
        loop = *preprocessed;
    }
    const AccelerationResult accel_res = LoopAcceleration::accelerate(its, loop.toLinear(), -1, Complexity::Const);
    if (accel_res.rules.empty()) {
        if (Smt::check(Chaining::chainRules(its, loop, loop, false)->getGuard(), its) == Smt::Unsat) {
            if (log) std::cout << "pseudo loop" << std::endl;
            return PseudoLoop;
        } else {
            if (log) std::cout << "acceleration failed" << std::endl;
            return Failed;
        }
    }
    assert(accel_res.rules.size() == 1);
    accel_proof.storeSubProof(accel_res.proof);
    Rule accel = accel_res.rules.front();
    const auto simplified = Preprocess::simplifyRule(its, accel, true);
    if (simplified) {
        accel = *simplified;
        accel_proof.storeSubProof(simplified.getProof());
    }
    if (log) {
        std::cout << "accelerated rule:" << std::endl;
        ITSExport::printRule(accel, its, std::cout);
        std::cout << std::endl;
    }
    if (accel.getUpdate(0) == loop.getUpdate(0)) {
        if (log) std::cout << "acceleration yielded equivalent rule" << std::endl;
        return Failed;
    }
    const auto loop_idx = its.addRule(accel);
    if (!loop_idx) {
        if (log) std::cout << "adding rule failed" << std::endl;
        return Failed;
    }
    drop_loop(backlink);
    z3.push();
    z3.add((sigmas.back())(accel.getGuard()));
    if (z3.check() != Smt::Sat) {
        if (log) std::cout << "applying accelerated rule failed" << std::endl;
        z3.pop();
        its.removeRule(*loop_idx);
        return DroppedLoop;
    }
    regexes[*loop_idx] = automaton;
    const auto src = accel.getLhsLoc();
    auto it = accelerated.find(src);
    if (it == accelerated.end()) {
        accelerated.emplace(src, std::vector<TransIdx>{*loop_idx});
    } else {
        it->second.push_back(*loop_idx);
    }
    update_transitions(src);
    store(*loop_idx, accel.getGuard());
    proof.majorProofStep("accelerated loop", its);
    proof.storeSubProof(accel_proof);
    return Successful;
}

void Reachability::analyze() {
    proof.majorProofStep("initial ITS", its);
    preprocess();
    init();
    do {
        if (log) std::cout << "trace: " << trace << std::endl;
        State state = Failed;
        const TransIdx current = trace.empty() ? its.getInitialLocation() : its.getRule(trace.back().transition).getRhsLoc(0);
        auto &trans = transitions[current];
        auto it = trans.begin();
        std::vector<TransIdx> append;
        while (it != trans.end()) {
            z3.push();
            const option<BoolExpr> sat = do_step(*it);
            if (!sat) {
                z3.pop();
                append.push_back(*it);
                it = trans.erase(it);
                continue;
            }
            const Rule &r = its.getRule(*it);
            if (r.getRhsLoc(0) == sink) {
                extend_trace(*it, *sat);
                unsat();
                return;
            }
            store(*it, *sat);
            const int backlink = is_loop(*it);
            if (backlink < 0) {
                if (r.isSimpleLoop()) {
                    // this can only happen if we applied an accelerated transition, make sure that we do not apply it again straight away
                    do_block(trace.back());
                }
                state = Successful;
                ++it;
                break;
            }
            const Step step = trace[backlink];
            bool simple_loop = static_cast<unsigned>(backlink) == trace.size() - 1;
            state = handle_loop(backlink);
            switch (state) {
            case PseudoLoop:
                state = Successful;
                ++it;
                break;
            case Failed:
                // do not increment 'it' here, just block the model that we got and hope for others
                backtrack();
                continue;
            case Successful:
                if (simple_loop) {
                    do_block(step);
                }
                // make sure that we do not apply the accelerated transition again straight away, which is redundant
                do_block(trace.back());
                break;
            case DroppedLoop:
                do_block(step);
                break;
            }
            break;
        }
        trans.insert(trans.end(), append.begin(), append.end());
        if (state == Failed && !trace.empty()) {
            backtrack();
        }
    } while (!trace.empty());
    std::cout << "unknown" << std::endl << std::endl;
}

void Reachability::analyze(ITSProblem &its) {
    Yices::init();
    Reachability(its).analyze();
    Yices::exit();
}
