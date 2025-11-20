#include <iostream>
#include <map>
#include <vector>
#include <stack>
#include <iomanip>
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

map<int, map<char, string>> actionTable = {
    {0, {{'a', "s2"}, {'b', "s3"}}},
    {1, {{'$', "acc"}}},
    {2, {{'a', "s2"}, {'b', "s3"}}},
    {3, {{'$', "r2"}}},
    {4, {{'$', "r1"}}}
};

map<int, map<char, int>> gotoTable = {
    {0, {{'S', 1}}},
    {2, {{'S', 4}}}
};

void printParsingTable() {
    vector<char> terminals = {'a', 'b', '$'};
    vector<char> nonTerminals = {'S'};

    cout << "\nSLR(1) Parsing Table:\n";
    cout << setw(6) << "State";
    for (char t : terminals) cout << setw(6) << t;
    for (char nt : nonTerminals) cout << setw(6) << nt;
    cout << endl;

    for (int state = 0; state <= 4; state++) {
        cout << setw(6) << state;
        for (char t : terminals) {
            cout << setw(6) << actionTable[state][t];
        }
        for (char nt : nonTerminals) {
            if (gotoTable[state].count(nt))
                cout << setw(6) << gotoTable[state][nt];
            else
                cout << setw(6) << "";
        }
        cout << endl;
    }
}

void parseString(string input) {
    input += "$";
    stack<int> st;
    st.push(0);
    int i = 0;

    cout << "\nParsing steps:\n";
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

        if (action[0] == 's') {
            int nextState = stoi(action.substr(1));
            st.push(symbol);
            st.push(nextState);
            i++;
        } else if (action[0] == 'r') {
            int prodIndex = stoi(action.substr(1));
            string rhs = grammar[prodIndex].rhs;
            for (int j = 0; j < 2 * rhs.size(); j++) st.pop();
            int topState = st.top();
            st.push(grammar[prodIndex].lhs[0]);
            st.push(gotoTable[topState][grammar[prodIndex].lhs[0]]);
            cout << "Reduce using " << grammar[prodIndex].lhs << " → " << rhs << endl;
        } else if (action == "acc") {
            cout << "Accepted!\n";
            break;
        } else {
            cout << "Error!\n";
            break;
        }
    }
}

int main() {
    printParsingTable();
    string input;
    cout << "\nEnter string to parse (e.g., aab): ";
    cin >> input;
    parseString(input);
    return 0;
}