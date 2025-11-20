#include <iostream>
#include <map>
#include <vector>
#include <stack>
#include <iomanip>
using namespace std;

// Grammar:
// 0: S' -> S
// 1: S  -> aS
// 2: S  -> b

vector<pair<string,string>> grammar = {
    {"S'","S"},   // 0
    {"S","aS"},   // 1
    {"S","b"}     // 2
};

// ACTION table: state -> (terminal -> action string)
// action string: "sN" (shift N), "rK" (reduce by production K), "acc" (accept), "" (error)
map<int, map<char, string>> actionTable;

// GOTO table: state -> (nonterminal -> state)
map<int, map<char, int>> gotoTable;

void initTables() {
    // For this grammar the LALR(1) table (after merging lookaheads) ends up identical
    // to the SLR/expected table for this simple grammar.

    // State 0
    actionTable[0]['a'] = "s2";
    actionTable[0]['b'] = "s3";
    gotoTable[0]['S'] = 1;

    // State 1
    actionTable[1]['$'] = "acc";

    // State 2
    actionTable[2]['a'] = "s2";
    actionTable[2]['b'] = "s3";
    actionTable[2]['$'] = "r1"; // reduce by S -> aS

    // State 3
    actionTable[3]['$'] = "r2"; // reduce by S -> b

    // State 4 (goto from 2 on S)
    actionTable[4]['a'] = "s2";
    actionTable[4]['b'] = "s3";
    actionTable[4]['$'] = "r1";
    gotoTable[4]['S'] = 4;

    // Note: gotoTable[2]['S'] = 4 (when we reduce in state 2 we push goto of top).
    gotoTable[2]['S'] = 4;
}

void printTable() {
    vector<char> terms = {'a','b','$'};
    vector<char> nterms = {'S'};

    cout << "\nLALR(1) Parsing Table:\n";
    cout << setw(6) << "State";
    for (char t : terms) cout << setw(6) << t;
    for (char nt : nterms) cout << setw(6) << nt;
    cout << '\n';

    for (int st = 0; st <= 4; ++st) {
        cout << setw(6) << st;    
        for (char t : terms) {
            string out = "";
            if (actionTable.count(st) && actionTable[st].count(t)) out = actionTable[st][t];
            cout << setw(6) << out;
        }
        for (char nt : nterms) {
            string out = "";
            if (gotoTable.count(st) && gotoTable[st].count(nt)) out = to_string(gotoTable[st][nt]);
            cout << setw(6) << out;
        }
        cout << '\n';
    }
}

void parseString(const string &inp, map<int, map<char, string>> &tbl) {
    string input = inp + "$";
    stack<int> st;
    st.push(0);
    int i = 0;

    while (true) {
        int state = st.top();
        char sym = input[i];
        string action = "";

        if (tbl.count(state) && tbl[state].count(sym)) action = tbl[state][sym];

        if (action == "acc") {
            cout << "Accepted\n";
            return;
        }
        if (action == "") {
            cout << "Syntax Error\n";
            return;
        }

        if (action[0] == 's') {
            int nxt = stoi(action.substr(1));
            // push symbol and state (we only store states on stack; using state-only stack is fine here)
            st.push(nxt);
            i++;
        } else if (action[0] == 'r') {
            int prod = stoi(action.substr(1));
            string rhs = grammar[prod].second;
            // Each shift pushes a state; to pop RHS length we pop one state per grammar symbol in rhs.
            int pops = rhs.size();
            for (int k = 0; k < pops; ++k) {
                if (!st.empty()) st.pop();
            }
            int topState = st.top();
            char lhs = grammar[prod].first[0]; // 'S' or "S'"
            // goto from topState on lhs
            if (gotoTable.count(topState) && gotoTable[topState].count(lhs)) {
                st.push(gotoTable[topState][lhs]);
            } else {
                // if goto missing, it's an error
                cout << "Syntax Error\n";
                return;
            }
        } else {
            cout << "Syntax Error\n";
            return;
        }
    }
}

int main() {
    initTables();
    printTable();

    string s;
    cout << "\nEnter string (ex: aab): ";
    if (!(cin >> s)) return 0;

    cout << "\n--- LALR(1) Parsing ---\n";
    parseString(s, actionTable);

    return 0;
}
