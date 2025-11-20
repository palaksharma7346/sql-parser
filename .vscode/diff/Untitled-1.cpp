// lr0_slr.cpp
// Build LR(0) canonical collection, LR(0) parsing table, compute FIRST/FOLLOW,
// build SLR(1) parsing table, and demonstrate parsing a sample input.
// Simple grammar:
// 0: S' -> S
// 1: S  -> A
// 2: A  -> a A
// 3: A  -> b

#include <bits/stdc++.h>
using namespace std;

struct Production {
    int lhs;           // index of nonterminal
    vector<int> rhs;   // sequence of symbol indices (terminals/nonterminals)
};

struct Item {
    int prod;    // production index
    int dot;     // dot position (0..rhs.size())
    bool operator==(Item const& o) const { return prod==o.prod && dot==o.dot; }
};

struct ItemHash {
    size_t operator()(Item const& it) const noexcept {
        return (it.prod*31u) ^ (it.dot*1000003u);
    }
};

using ItemSet = vector<Item>;

string token_name(const vector<string>& idx2sym, int idx) {
    return idx2sym[idx];
}

// Utility: compare itemsets ignoring order
bool same_itemset(const ItemSet &a, const ItemSet &b) {
    if (a.size()!=b.size()) return false;
    unordered_set<long long> s;
    for (auto &it: a) s.insert(((long long)it.prod<<32) | it.dot);
    for (auto &it: b) {
        long long key = ((long long)it.prod<<32) | it.dot;
        if (!s.count(key)) return false;
    }
    return true;
}

// print itemset
void print_itemset(const ItemSet &I, const vector<Production>& prods, const vector<string>& idx2sym) {
    for (auto &it: I) {
        const Production &p = prods[it.prod];
        cout << "   [" << it.prod << "] " << idx2sym[p.lhs] << " -> ";
        for (int i=0;i<(int)p.rhs.size();++i) {
            if (i==it.dot) cout << ".";
            cout << idx2sym[p.rhs[i]] << " ";
        }
        if (it.dot == (int)p.rhs.size()) cout << ".";
        cout << "\n";
    }
}

// closure(I)
ItemSet closure(const ItemSet &I, const vector<Production>& prods, const vector<vector<int>>& nt_prods_of) {
    ItemSet C = I;
    bool added = true;
    while (added) {
        added = false;
        // for each item A -> α . B β
        for (auto &it : C) {
            const Production &p = prods[it.prod];
            if (it.dot < (int)p.rhs.size()) {
                int sym = p.rhs[it.dot];
                // if sym is nonterminal: add items B -> .γ for each production of B
                if (nt_prods_of[sym].size() > 0) { // we use nonterminals mapped to indices with nt_prods_of non-empty
                    for (int prodIdx : nt_prods_of[sym]) {
                        Item newIt{prodIdx, 0};
                        bool found = false;
                        for (auto &x : C) if (x==newIt) { found = true; break; }
                        if (!found) {
                            C.push_back(newIt);
                            added = true;
                        }
                    }
                }
            }
        }
    }
    return C;
}

// goto(I, X)
ItemSet goto_set(const ItemSet &I, int X, const vector<Production>& prods, const vector<vector<int>>& nt_prods_of) {
    ItemSet moved;
    for (auto &it : I) {
        const Production &p = prods[it.prod];
        if (it.dot < (int)p.rhs.size() && p.rhs[it.dot] == X) {
            moved.push_back(Item{it.prod, it.dot+1});
        }
    }
    return closure(moved, prods, nt_prods_of);
}

// SYMBOL classification helpers
bool is_terminal(int idx, const vector<vector<int>>& nt_prods_of) {
    // terminals have no productions associated
    return nt_prods_of[idx].empty();
}

// compute FIRST sets (only needed for nonterminals and terminals)
void compute_FIRST(const vector<Production>& prods, const vector<string>& idx2sym, const vector<vector<int>>& nt_prods_of,
                   vector<unordered_set<int>>& FIRST) {
    int N = idx2sym.size();
    FIRST.assign(N, {});
    // terminals: FIRST(t) = {t}
    for (int i=0;i<N;i++) {
        if (nt_prods_of[i].empty()) {
            FIRST[i].insert(i);
        }
    }
    bool changed = true;
    while (changed) {
        changed = false;
        for (const Production &p: prods) {
            int A = p.lhs;
            bool eps_possible = true; // we don't have epsilon in this grammar, but keep algorithm generic
            for (int X : p.rhs) {
                // add FIRST(X) - {epsilon} to FIRST(A)
                for (int t : FIRST[X]) {
                    if (!FIRST[A].count(t)) {
                        FIRST[A].insert(t); changed = true;
                    }
                }
                // here we assume no epsilon (empty) productions => break
                eps_possible = false;
                break;
            }
            // if epsilon_possible, add epsilon (we won't use).
        }
    }
}

// compute FOLLOW sets (standard)
void compute_FOLLOW(const vector<Production>& prods, const vector<string>& idx2sym, const vector<vector<int>>& nt_prods_of,
                    int start_nt, vector<unordered_set<int>>& FOLLOW, const vector<unordered_set<int>>& FIRST) {
    int N = idx2sym.size();
    FOLLOW.assign(N, {});
    // Follow only for nonterminals; we assume terminals have empty follow
    FOLLOW[start_nt].insert(N-1); // we will place $ as last symbol index; but here we'll treat '$' as a terminal index passed in
    bool changed = true;
    while (changed) {
        changed = false;
        for (const Production &p: prods) {
            int A = p.lhs;
            for (int i=0;i<(int)p.rhs.size(); ++i) {
                int B = p.rhs[i];
                if (nt_prods_of[B].empty()) continue; // only for nonterminals
                // compute FIRST of beta
                unordered_set<int> first_beta;
                bool beta_has_epsilon = true;
                for (int j=i+1;j<(int)p.rhs.size();++j) {
                    int X = p.rhs[j];
                    // add FIRST(X) to first_beta
                    for (int t : FIRST[X]) first_beta.insert(t);
                    // in our simplified grammar, no epsilon -> stop after first
                    beta_has_epsilon = false;
                    break;
                }
                // add FIRST(beta) - {epsilon} to FOLLOW(B)
                for (int t : first_beta) {
                    if (!FOLLOW[B].count(t)) {
                        FOLLOW[B].insert(t); changed = true;
                    }
                }
                // If beta is empty OR FIRST(beta) contains epsilon then add FOLLOW(A) to FOLLOW(B)
                if (i+1 == (int)p.rhs.size() || beta_has_epsilon) {
                    for (int t : FOLLOW[A]) {
                        if (!FOLLOW[B].count(t)) {
                            FOLLOW[B].insert(t); changed = true;
                        }
                    }
                }
            }
        }
    }
}

// ---------- MAIN ----------
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // Symbol mapping:
    // We'll assign integer indices to symbols. To distinguish terminals vs nonterminals
    // we'll include both in same mapping.
    // We'll append "$" as the last terminal symbol index.

    // Symbol names (we will fill after deciding indices)
    vector<string> idx2sym;
    unordered_map<string,int> sym2idx;

    auto add_symbol = [&](const string &s)->int{
        if (sym2idx.count(s)) return sym2idx[s];
        int id = idx2sym.size();
        idx2sym.push_back(s);
        sym2idx[s] = id;
        return id;
    };

    // Nonterminals first
    int S_p = add_symbol("S'"); // augmented start
    int S   = add_symbol("S");
    int A   = add_symbol("A");

    // Terminals
    int a = add_symbol("a");
    int b = add_symbol("b");
    int dollar = add_symbol("$"); // EOF marker

    // For convenience, we'll store productions; RHS uses indices to symbols.
    vector<Production> prods;
    auto add_prod = [&](const string &lhs, const vector<string>& rhs_str) {
        int lhs_idx = sym2idx[lhs];
        vector<int> rhs_idx;
        for (auto &s: rhs_str) rhs_idx.push_back(sym2idx[s]);
        prods.push_back(Production{lhs_idx, rhs_idx});
    };

    // Build productions according to grammar:
    // 0: S' -> S
    // 1: S  -> A
    // 2: A  -> a A
    // 3: A  -> b
    add_prod("S'", {"S"});
    add_prod("S", {"A"});
    add_prod("A", {"a","A"});
    add_prod("A", {"b"});

    int P = prods.size();

    // Build list of productions per nonterminal for quick lookup
    int SYM = idx2sym.size();
    vector<vector<int>> nt_prods_of(SYM); // for every symbol index, list of production indices where symbol is lhs
    for (int i=0;i<P;++i) {
        nt_prods_of[prods[i].lhs].push_back(i);
    }

    // Compute canonical collection of LR(0) items
    vector<ItemSet> C; // states
    // initial item: [0] S' -> .S
    ItemSet I0 = closure(ItemSet{ Item{0,0} }, prods, nt_prods_of);
    C.push_back(I0);

    bool addedState = true;
    while (addedState) {
        addedState = false;
        int curStates = C.size();
        for (int i=0;i<curStates;++i) {
            // compute goto on every symbol
            // gather all possible symbols that appear after dot in items of C[i]
            unordered_set<int> symbols;
            for (auto &it: C[i]) {
                const Production &p = prods[it.prod];
                if (it.dot < (int)p.rhs.size()) symbols.insert(p.rhs[it.dot]);
            }
            for (int X : symbols) {
                ItemSet g = goto_set(C[i], X, prods, nt_prods_of);
                if (g.empty()) continue;
                // check if g matches an existing state
                bool found=false;
                for (auto &s : C) {
                    if (same_itemset(s,g)) { found=true; break; }
                }
                if (!found) {
                    C.push_back(g);
                    addedState = true;
                }
            }
        }
    }

    cout << "Total LR(0) states: " << C.size() << "\n\n";
    for (int i=0;i<(int)C.size();++i) {
        cout << "I" << i << ":\n";
        print_itemset(C[i], prods, idx2sym);
        cout << "\n";
    }

    // Build LR(0) action/goto tables (tables indexed by state and symbol)
    // We'll represent actions as:
    //  - "sX" shift to state X
    //  - "rY" reduce by production Y
    //  - "acc" accept
    //  - empty = error
    int STATES = C.size();
    int SYMBOLS = SYM;
    vector<unordered_map<int,string>> ACTION_LR0(STATES); // terminal -> action
    vector<unordered_map<int,int>> GOTO_LR0(STATES);      // nonterm -> state

    // Build transitions mapping for goto
    // compute goto transitions (state, X) -> stateIndex
    map<pair<int,int>,int> goto_trans;
    for (int i=0;i<STATES;++i) {
        // possible symbols after dots
        unordered_set<int> symbols;
        for (auto &it: C[i]) {
            auto &p = prods[it.prod];
            if (it.dot < (int)p.rhs.size()) symbols.insert(p.rhs[it.dot]);
        }
        for (int X : symbols) {
            ItemSet g = goto_set(C[i], X, prods, nt_prods_of);
            if (g.empty()) continue;
            for (int s=0;s<STATES;++s) {
                if (same_itemset(C[s], g)) { goto_trans[{i,X}] = s; break; }
            }
        }
    }

    // Fill ACTION_LR0 and GOTO_LR0 (naively: reductions on all terminals for LR(0))
    for (int i=0;i<STATES;++i) {
        for (auto &it : C[i]) {
            const Production &p = prods[it.prod];
            if (it.dot < (int)p.rhs.size()) {
                int aSym = p.rhs[it.dot];
                // shift on symbol aSym to state goto(i,aSym) if terminal
                if (nt_prods_of[aSym].empty()) { // terminal
                    if (goto_trans.count({i,aSym})) {
                        int tstate = goto_trans[{i,aSym}];
                        string act = "s" + to_string(tstate);
                        if (ACTION_LR0[i].count(aSym) && ACTION_LR0[i][aSym] != act) {
                            ACTION_LR0[i][aSym] += "|" + act; // mark conflict
                        } else ACTION_LR0[i][aSym] = act;
                    }
                } else {
                    // nonterminal goto -> GOTO table
                    if (goto_trans.count({i,aSym})) {
                        int tstate = goto_trans[{i,aSym}];
                        GOTO_LR0[i][aSym] = tstate;
                    }
                }
            } else {
                // dot at end -> reduce by production it.prod
                if (it.prod == 0) {
                    // production 0 is S' -> S ; mark accept on $
                    ACTION_LR0[i][dollar] = "acc";
                } else {
                    // Reduce by this production on all terminals (LR(0) naive)
                    for (int t=0;t<SYMBOLS;++t) {
                        if (!nt_prods_of[t].empty()) continue; // skip nonterminals
                        string act = "r" + to_string(it.prod);
                        if (ACTION_LR0[i].count(t) && ACTION_LR0[i][t] != act) {
                            ACTION_LR0[i][t] += "|" + act;
                        } else ACTION_LR0[i][t] = act;
                    }
                }
            }
        }
    }

    // Print LR(0) table
    cout << "----- LR(0) ACTION/GOTO table -----\n";
    cout << "States x Terminals (";
    // list terminals
    vector<int> terminals;
    for (int i=0;i<SYMBOLS;++i) if (nt_prods_of[i].empty()) terminals.push_back(i);
    for (auto t: terminals) cout << idx2sym[t] << (t==terminals.back()?"":", ");
    cout << ")\n\n";
    for (int i=0;i<STATES;++i) {
        cout << "State " << i << ":\n";
        cout << " ACTION:\n";
        for (auto t: terminals) {
            cout << "  on '" << idx2sym[t] << "' -> ";
            if (ACTION_LR0[i].count(t)) cout << ACTION_LR0[i][t];
            else cout << "-";
            cout << "\n";
        }
        cout << " GOTO:\n";
        for (int j=0;j<SYMBOLS;++j) {
            if (!nt_prods_of[j].empty()) {
                cout << "  " << idx2sym[j] << " -> ";
                if (GOTO_LR0[i].count(j)) cout << GOTO_LR0[i][j];
                else cout << "-";
                cout << "\n";
            }
        }
        cout << "\n";
    }

    // Report LR(0) conflicts (presence of '|' in actions)
    bool lr0_conflict=false;
    for (int i=0;i<STATES;++i) for (auto &kv: ACTION_LR0[i]) if (kv.second.find('|')!=string::npos) lr0_conflict=true;
    cout << (lr0_conflict ? "LR(0) parsing table has conflicts (grammar not LR(0)).\n\n"
                         : "LR(0) parsing table has no conflicts (grammar is LR(0)).\n\n");

    // ---------- Compute FIRST and FOLLOW for SLR ----------
    vector<unordered_set<int>> FIRST;
    compute_FIRST(prods, idx2sym, nt_prods_of, FIRST);
    // We'll treat dollar ($) as a terminal; ensure it's in FIRST too
    FIRST[dollar].insert(dollar);

    // Print FIRST (for nonterminals mainly)
    cout << "FIRST sets (terminals included):\n";
    for (int i=0;i<SYMBOLS;++i) {
        cout << idx2sym[i] << " : { ";
        for (int t: FIRST[i]) cout << idx2sym[t] << " ";
        cout << "}\n";
    }
    cout << "\n";

    vector<unordered_set<int>> FOLLOW;
    compute_FOLLOW(prods, idx2sym, nt_prods_of, S_p, FOLLOW, FIRST);

    cout << "FOLLOW sets:\n";
    for (int i=0;i<SYMBOLS;++i) {
        if (nt_prods_of[i].empty()) continue;
        cout << idx2sym[i] << " : { ";
        for (int t: FOLLOW[i]) cout << idx2sym[t] << " ";
        cout << "}\n";
    }
    cout << "\n";

    // Build SLR(1) table (use FOLLOW for reduce placement)
    vector<unordered_map<int,string>> ACTION_SLR(STATES);
    vector<unordered_map<int,int>> GOTO_SLR(STATES);

    for (int i=0;i<STATES;++i) {
        for (auto &it : C[i]) {
            const Production &p = prods[it.prod];
            if (it.dot < (int)p.rhs.size()) {
                int aSym = p.rhs[it.dot];
                if (nt_prods_of[aSym].empty()) {
                    if (goto_trans.count({i,aSym})) {
                        int tstate = goto_trans[{i,aSym}];
                        string act = "s" + to_string(tstate);
                        if (ACTION_SLR[i].count(aSym) && ACTION_SLR[i][aSym] != act) {
                            ACTION_SLR[i][aSym] += "|" + act;
                        } else ACTION_SLR[i][aSym] = act;
                    }
                } else {
                    if (goto_trans.count({i,aSym})) {
                        int tstate = goto_trans[{i,aSym}];
                        GOTO_SLR[i][aSym] = tstate;
                    }
                }
            } else {
                if (it.prod == 0) {
                    ACTION_SLR[i][dollar] = "acc";
                } else {
                    // reduce by production it.prod, but only on terminals in FOLLOW(lhs)
                    int A_lhs = p.lhs;
                    for (int t : FOLLOW[A_lhs]) {
                        string act = "r" + to_string(it.prod);
                        if (ACTION_SLR[i].count(t) && ACTION_SLR[i][t] != act) {
                            ACTION_SLR[i][t] += "|" + act;
                        } else ACTION_SLR[i][t] = act;
                    }
                }
            }
        }
    }

    // Print SLR table
    cout << "----- SLR(1) ACTION/GOTO table -----\n";
    for (int i=0;i<STATES;++i) {
        cout << "State " << i << ":\n";
        cout << " ACTION:\n";
        for (auto t: terminals) {
            cout << "  on '" << idx2sym[t] << "' -> ";
            if (ACTION_SLR[i].count(t)) cout << ACTION_SLR[i][t];
            else cout << "-";
            cout << "\n";
        }
        cout << " GOTO:\n";
        for (int j=0;j<SYMBOLS;++j) {
            if (!nt_prods_of[j].empty()) {
                cout << "  " << idx2sym[j] << " -> ";
                if (GOTO_SLR[i].count(j)) cout << GOTO_SLR[i][j];
                else cout << "-";
                cout << "\n";
            }
        }
        cout << "\n";
    }

    // Report SLR conflicts
    bool slr_conflict=false;
    for (int i=0;i<STATES;++i) for (auto &kv: ACTION_SLR[i]) if (kv.second.find('|')!=string::npos) slr_conflict=true;
    cout << (slr_conflict ? "SLR(1) parsing table has conflicts (grammar not SLR(1)).\n\n"
                         : "SLR(1) parsing table has no conflicts (grammar is SLR(1)).\n\n");

    // ---------- Demo parsing using SLR(1) ----------
    // parse a sample input: "aab" -> tokens: a a b $
    cout << "Parsing demo (using SLR(1) table):\n";
    string input = "aab";
    cout << " Input string: \"" << input << "\" (tokens: a a b $ )\n";

    // tokenize into symbol indices
    vector<int> tokens;
    for (char ch : input) {
        if (ch=='a') tokens.push_back(a);
        else if (ch=='b') tokens.push_back(b);
        else continue;
    }
    tokens.push_back(dollar);

    // parse stack: state stack and symbol stack not needed separately; use state stack and symbol stack for clarity
    vector<int> stateStack; stateStack.push_back(0);
    vector<int> symStack; symStack.push_back(-1); // sentinel

    int ip = 0;
    bool accept=false, error=false;
    while (!accept && !error) {
        int s = stateStack.back();
        int look = tokens[ip];
        string action = ACTION_SLR[s].count(look) ? ACTION_SLR[s][look] : "";
        cout << " State=" << s << ", lookahead=" << idx2sym[look] << " => action: " << (action==""? "(err)":action) << "\n";
        if (action=="") { error=true; break; }
        if (action=="acc") { accept=true; break; }
        if (action[0]=='s') {
            // shift
            int t = stoi(action.substr(1));
            cout << "  shift -> push symbol " << idx2sym[look] << ", state " << t << "\n";
            symStack.push_back(look);
            stateStack.push_back(t);
            ip++;
        } else if (action[0]=='r') {
            int prodIdx = stoi(action.substr(1));
            const Production &rp = prods[prodIdx];
            cout << "  reduce by [" << prodIdx << "] " << idx2sym[rp.lhs] << " -> ";
            for (int x: rp.rhs) cout << idx2sym[x] << " ";
            cout << "\n";
            // pop |rhs| symbols
            for (int k=0;k<(int)rp.rhs.size();++k) {
                if (!symStack.empty()) symStack.pop_back();
                if (!stateStack.empty()) stateStack.pop_back();
            }
            // push LHS symbol and goto
            int curState = stateStack.back();
            symStack.push_back(rp.lhs);
            if (GOTO_SLR[curState].count(rp.lhs)==0) {
                cout << "  ERROR: no goto after reduce\n";
                error=true; break;
            }
            int gotoState = GOTO_SLR[curState][rp.lhs];
            cout << "  goto state " << gotoState << "\n";
            stateStack.push_back(gotoState);
        } else {
            // conflict or unknown combined action
            cout << "  ERROR: unhandled action '" << action << "'\n";
            error=true; break;
        }
    }

    if (accept) cout << "Input accepted by SLR(1) parser.\n";
    else cout << "Parsing failed / error.\n";

    return 0;
}
