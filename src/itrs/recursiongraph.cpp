/*  This file is part of LoAT.
 *  Copyright (c) 2015-2016 Matthias Naaf, RWTH Aachen University, Germany
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses>.
 */

#include "recursiongraph.h"

using namespace std;

const NodeIndex RecursionGraph::NULLNODE = -1;

RecursionGraph::RecursionGraph(ITRSProblem &itrs)
    : itrs(itrs), nextRightHandSide(0) {
    nodes.insert(NULLNODE);

    for (FunctionSymbolIndex i = 0; i < itrs.getFunctionSymbolCount(); ++i) {
        nodes.insert((NodeIndex)i);
    }

    initial = itrs.getStartFunctionSymbol();

    for (const ITRSRule &rule : itrs.getRules()) {
        addRule(rule);
    }
}


void RecursionGraph::print(ostream &s) const {
    auto printVar = [&](VariableIndex vi) {
        s << vi;
        s << "[" << itrs.getVarname(vi) << "]";
    };
    auto printNode = [&](NodeIndex ni) {
        s << ni;
        s << "[";
        if (ni >= 0) {
            itrs.printLhs((FunctionSymbolIndex)ni, s);
        } else {
            s << "null";
        }
        s << "]";
    };

    s << "Nodes:";
    for (NodeIndex n : nodes) {
        s << " " << n;
        if (n == initial) s << "*";
    }
    s << endl;

    s << "Transitions:" << endl;
    for (NodeIndex n : nodes) {
        for (TransIndex trans : getTransFrom(n)) {
            printNode(n);
            s << " -> ";
            printNode(getTransTarget(trans));
            const RightHandSideIndex index = getTransData(trans);
            printRhs(rightHandSides.at(index), s);
            s << endl;
        }
    }
}


void RecursionGraph::printForProof() const {
    auto printNode = [&](NodeIndex ni) {
        proofout << ni;
        proofout << "[";
        if (ni >= 0) {
            itrs.printLhs((FunctionSymbolIndex)ni, proofout);
        } else {
            proofout << "null";
        }
        proofout << "]";
    };

    proofout << "  Start location: "; printNode(initial); proofout << endl;
    if (getTransCount() == 0) proofout << "    <empty>" << endl;

    for (NodeIndex n : nodes) {
        for (TransIndex trans : getTransFrom(n)) {
            proofout << "    ";
            proofout << setw(3) << trans << ": ";
            printNode(n);
            proofout << " -> ";
            printNode(getTransTarget(trans));
            proofout << " : ";
            const RightHandSideIndex index = getTransData(trans);
            printRhs(rightHandSides.at(index), proofout);
        }
    }
    proofout << endl;
}


void RecursionGraph::printDot(ostream &s, int step, const string &desc) const {
    auto printNodeName = [&](NodeIndex ni) {
        if (ni >= 0) {
            itrs.printLhs((FunctionSymbolIndex)ni, s);
        } else {
            s << "null";
        }
    };
    auto printNode = [&](NodeIndex n) {
        s << "node_" << step << "_";
        if (n >= 0) {
            s << n;
        }
    };

    s << "subgraph cluster_" << step << " {" << endl;
    s << "sortv=" << step << ";" << endl;
    s << "label=\"" << step << ": " << desc << "\";" << endl;
    for (NodeIndex n : nodes) {
        printNode(n); s << " [label=\""; printNodeName(n); s << "\"];" << endl;
    }
    for (NodeIndex n : nodes) {
        for (NodeIndex succ : getSuccessors(n)) {
            printNode(n); s << " -> "; printNode(succ); s << " [label=\"";
            for (TransIndex trans : getTransFromTo(n,succ)) {
                const RightHandSideIndex index = getTransData(trans);
                const RightHandSide &rhs = rightHandSides.at(index);

                s << "(" << index << "): ";
                rhs.term->print(itrs, s);
                s << ", [";
                for (int i = 0; i < rhs.guard.size(); ++i) {
                    if (i > 0) {
                        s << ", ";
                    }

                    s << rhs.guard[i];
                }
                s << "], ";
                s << rhs.cost;

                s << "\\l";
            }
            s << "\"];" << endl;
        }
    }
    s << "}" << endl;
}


void RecursionGraph::printDotText(ostream &s, int step, const string &txt) const {
    s << "subgraph cluster_" << step << " {" << endl;
    s << "sortv=" << step << ";" << endl;
    s << "label=\"" << step << ": " << "Result" << "\";" << endl;
    s << "node_" << step << "_result [label=\"" << txt << "\"];" << endl;
    s << "}" << endl;
}

void RecursionGraph::printRhs(const RightHandSide &rhs, std::ostream &os) const {
    os << "right-hand side(";
    rhs.term->print(itrs, os);
    os << "| ";
    for (auto expr : rhs.guard) {
        os << expr << ", ";
    }
    os << "| ";
    os << rhs.cost;
    os << ")";
}


void RecursionGraph::addRule(const ITRSRule &rule) {
    RightHandSide rhs;
    rhs.guard = rule.guard;
    rhs.term = itrs.getTerm(rule.rhs);
    rhs.cost = rule.cost;

    NodeIndex src = (NodeIndex)rule.lhs;
    std::vector<NodeIndex> dsts = (std::vector<NodeIndex>)rhs.term->getFunctionSymbolsAsVector();
    if (dsts.empty()) {
        dsts.push_back(NULLNODE);
    }

    RightHandSideIndex rhsIndex = nextRightHandSide++;
    rightHandSides.insert(std::make_pair(rhsIndex, rhs));

    for (NodeIndex dst : dsts) {
        addTrans(src, dst, rhsIndex);
    }
}