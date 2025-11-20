#include <bits/stdc++.h>
using namespace std;

struct Action {
    string type;
    int state;
    int prod;
};

map<pair<int,string>, Action> actionLR0, actionSLR;
map<pair<int,string>, int> goTo;

// Grammar:
// r1: E → E + T
// r2: E → T
// r3: T → id

void initTables() {

    // Goto table (same for LR0 and SLR)
    goTo[{0,"E"}] = 1;
    goTo[{0,"T"}] = 2;
    goTo[{3,"T"}] = 4;

    // LR(0) table
    actionLR0[{0,"id"}] = {"shift",5,0};
    actionLR0[{1,"+"}]  = {"shift",3,0};
    actionLR0[{1,"$"}]  = {"acc",0,0};
    actionLR0[{2,"+"}]  = {"reduce",0,3};
    actionLR0[{2,"$"}]  = {"reduce",0,3};
    actionLR0[{3,"id"}] = {"shift",5,0};
    actionLR0[{4,"+"}]  = {"reduce",0,2};
    actionLR0[{4,"$"}]  = {"reduce",0,2};
    actionLR0[{5,"+"}]  = {"reduce",0,1};
    actionLR0[{5,"$"}]  = {"reduce",0,1};

    // SLR(1) table (same as LR0 for this grammar)
    actionSLR = actionLR0;
}

// Production rules
vector<pair<string,string>> rules = {
    {"E","+T"},
    {"E","T"},
    {"T","id"}
};

void parse(string input, map<pair<int,string>, Action> &table, string title) {
    cout << "\n--- " << title << " Parsing ---\n";

    stack<int> st;
    st.push(0);
    int i = 0;

    while (true) {
        int state = st.top();
        string sym = "";
        if (isalpha(input[i])) sym = "id";
        else sym = string(1,input[i]);

        auto key = make_pair(state, sym);
        if (table.find(key) == table.end()) { 
            cout << "Error!\n"; return;
        }

        Action a = table[key];

        if (a.type == "shift") {
            cout << "Shift " << sym << "\n";
            st.push(a.state);
            i += (sym=="id"?2:1);
        }
        else if (a.type == "reduce") {
            int r = a.prod;
            string A = rules[r].first;
            string B = rules[r].second;
            int pop = (B=="id" ? 1 : B.length());
            while (pop--) st.pop();
            int ns = goTo[{st.top(), A}];
            st.push(ns);
            cout << "Reduce by rule: " << A << " → " << B << "\n";
        }
        else if (a.type == "acc") {
            cout << "\n✅ Accepted!\n";
            return;
        }
    }
}

int main() {
    initTables();

    cout << "Enter input string (example: id+id+id$): ";
    string input;
    cin >> input;

    parse(input, actionLR0, "LR(0)");
    parse(input, actionSLR, "SLR(1)");

    return 0;
}
