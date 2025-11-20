#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <stack>
#include <string>
using namespace std;

struct Production {
    string lhs;
    string rhs;
};

vector<Production> grammar = {
    {"S'", "S"},
    {"S", "aS"},
    {"S", "b"}
};

map<int, map<char, string>> actionTable;
map<int, map<char, int>> gotoTable;

void constructLR0Table() {
    // Placeholder: Construct canonical LR(0) items and fill actionTable/gotoTable
    // For simplicity, assume precomputed tables for the above grammar
    actionTable[0]['a'] = "s2";
    actionTable[0]['b'] = "s3";
    gotoTable[0]['S'] = 1;
    actionTable[1]['$'] = "acc";
    actionTable[2]['a'] = "s2";
    actionTable[2]['b'] = "s3";
    gotoTable[2]['S'] = 4;
    actionTable[3]['$'] = "r2";
    actionTable[4]['$'] = "r1";
}

void parseString(string input) {
    input += "$";
    stack<int> st;
    st.push(0);
    int i = 0;

    while (true) {
        int state = st.top();
        char symbol = input[i];
        string action = actionTable[state][symbol];

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
            cout << "Accepted!" << endl;
            break;
        } else {
            cout << "Error!" << endl;
            break;
        }
    }
}

int main() {
    constructLR0Table();
    string input = "aab";
    parseString(input);
    return 0;
}