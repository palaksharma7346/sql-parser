#include <bits/stdc++.h>
using namespace std;

struct Action {
    string type;  // "s" = shift, "r" = reduce, "acc" = accept
    int state;
    int prod;
};

map<pair<int,string>, Action> actionLALR;
map<pair<int,string>, int> goTo;

// Grammar
// r1: S → S a
// r2: S → b

void initTables() {
    // ACTION TABLE (LALR(1))
    actionLALR[{0, "a"}] = {"", -1, -1};
    actionLALR[{0, "b"}] = {"s", 3, -1};

    actionLALR[{1, "a"}] = {"s", 2, -1};
    actionLALR[{1, "$"}] = {"acc", -1, -1};

    actionLALR[{2, "a"}] = {"s", 2, -1};
    actionLALR[{2, "$"}] = {"r", -1, 1}; // r1: S → S a

    actionLALR[{3, "a"}] = {"r", -1, 2}; // r2: S → b
    actionLALR[{3, "$"}] = {"r", -1, 2};

    actionLALR[{4, "a"}] = {"s", 2, -1};
    actionLALR[{4, "$"}] = {"r", -1, 1}; // S → S a

    // GOTO TABLE
    goTo[{0, "S"}] = 1;
    goTo[{1, "S"}] = -1;
    goTo[{2, "S"}] = 4;
    goTo[{3, "S"}] = -1;
    goTo[{4, "S"}] = 4;
}

bool parseLALR(string input) {
    input += "$";
    stack<int> st;
    st.push(0);
    int idx = 0;

    while (true) {
        int state = st.top();
        string symbol(1, input[idx]);
        
        if (actionLALR.count({state, symbol}) == 0) {
            return false;
        }

        Action act = actionLALR[{state, symbol}];

        if (act.type == "s") {
            st.push(act.state);
            idx++;
        }
        else if (act.type == "r") {
            if (act.prod == 1) { // S → S a
                st.pop(); st.pop();
                int newState = goTo[{st.top(), "S"}];
                st.push(newState);
            } 
            else if (act.prod == 2) { // S → b
                st.pop();
                int newState = goTo[{st.top(), "S"}];
                st.push(newState);
            }
        }
        else if (act.type == "acc") {
            return true;
        } 
        else return false;
    }
}

void printTable() {
    cout << "\nLALR(1) Parsing Table:\n";
    cout << " State   a      b      $      S\n";
    for (int i = 0; i < 5; i++) {
        cout << "   " << i << "   ";
        for (string term : {"a","b","$"}) {
            if (actionLALR.count({i,term})) {
                Action act = actionLALR[{i,term}];
                if (act.type == "s") cout << "s" << act.state << "    ";
                else if (act.type == "r") cout << "r" << act.prod << "    ";
                else cout << "acc   ";
            } else cout << "     ";
        }
        if (goTo.count({i,"S"})) cout << goTo[{i,"S"}];
        cout << "\n";
    }
}

int main() {
    initTables();
    printTable();

    string s;
    cout << "\nEnter string (ex: bba): ";
    cin >> s;

    cout << "\n--- LALR(1) Parsing ---\n";
    if (parseLALR(s)) cout << "Accepted\n";
    else cout << "Rejected\n";

    return 0;
}
