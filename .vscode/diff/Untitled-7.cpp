#include <iostream>
#include <map>
#include <vector>
#include <stack>
#include <iomanip>
#include <set>  
using namespace std;

struct Production {
    string lhs;
    string rhs;
};

vector<Production> grammar = {
    {"S'", "S"},   // Production 0
    {"S", "aS"},   // Production 1
    {"S", "b"}     // Production 2
};

// FOLLOW(S) = { $ } for SLR(1)
set<char> followS = {'$'};

map<int, map<char, string>> lr0ActionTable = {
    {0, {{'a', "s2"}, {'b', "s3"}}},
    {1, {{'$', "acc"}}},
    {2, {{'a', "s2"}, {'b', "s3"}, {'$', "r1"}}},
    {3, {{'a', ""}, {'b', ""}, {'$', "r2"}}},
    {4, {{'a', "s2"}, {'b', "s3"}, {'$', ""}}}
};

map<int, map<char, string>> slrActionTable;

map<int, map<char, int>> gotoTable = {
    {0, {{'S', 1}}},
    {2, {{'S', 4}}},
    {4, {{'S', 4}}}
};

void buildSLRTable() {
    slrActionTable = lr0ActionTable;

    // Remove invalid reductions using FOLLOW(S)
    for (int st : {2, 3, 4}) {
        for (char t : {'a', 'b'}) {
            if (!followS.count(t)) slrActionTable[st][t] = "";
        }
    }
}

void printTable(map<int, map<char, string>>& actionTable, string type) {
    vector<char> terminals = {'a', 'b', '$'};
    vector<char> nonTerminals = {'S'};

    cout << "\n" << type << " Parsing Table:\n";
    cout << setw(6) << "State";
    for (char t : terminals) cout << setw(6) << t;
    for (char nt : nonTerminals) cout << setw(6) << nt;
    cout << endl;

    for (int state = 0; state <= 4; state++) {
        cout << setw(6) << state;
        for (char t : terminals) cout << setw(6) << actionTable[state][t];
        for (char nt : nonTerminals)
            cout << setw(6) << (gotoTable[state].count(nt) ? gotoTable[state][nt] : -1);
        cout << endl;
    }
}

void parseString(string input, map<int, map<char, string>>& actionTable) {
    input += "$";
    stack<int> st;
    st.push(0);
    int i = 0;

    cout << "\nParsing Steps:\n";
    while (true) {
        int state = st.top();
        char symbol = input[i];
        string action = actionTable[state][symbol];

        cout << "Stack: ";
        stack<int> temp = st;
        vector<int> display;
        while (!temp.empty()) {
            display.push_back(temp.top());
            temp.pop();
        }
        for (int j = display.size() - 1; j >= 0; j--) cout << display[j] << " ";
        cout << " | Input: " << input.substr(i) << " | Action: " << action << endl;

        if (action == "acc") {
            cout << "Accepted ✅\n";
            break;
        }
        if (action == "") {
            cout << "Syntax Error ❌\n";
            break;
        }

        if (action[0] == 's') {
            int nextState = stoi(action.substr(1));
            st.push(symbol);
            st.push(nextState);
            i++;
        } 
        else if (action[0] == 'r') {
            int prodIndex = stoi(action.substr(1));
            string rhs = grammar[prodIndex].rhs;

            for (int j = 0; j < rhs.size() * 2; j++)
                st.pop();

            int topState = st.top();
            char lhs = grammar[prodIndex].lhs[0];

            st.push(lhs);
            st.push(gotoTable[topState][lhs]);

            cout << "Reduced: " << grammar[prodIndex].lhs << " → " << rhs << endl;
        }
    }
}

int main() {
    buildSLRTable();

    printTable(lr0ActionTable, "LR(0)");
    printTable(slrActionTable, "SLR(1)");

    string input;
    cout << "\nEnter input string (ex: aab): ";
    cin >> input;

    cout << "\n--- LR(0) Parsing ---";
    parseString(input, lr0ActionTable);

    cout << "\n--- SLR(1) Parsing ---";
    parseString(input, slrActionTable);

    return 0;
}
