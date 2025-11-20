#include <iostream>
#include <cmath>
#include <string>

using namespace std;

double binaryToDecimal(const string& binary) {
    int point = binary.find('.');
    double decimal = 0.0;

    // If point is -1, it means no decimal point was found.
    int integerPart;
    if (point == -1) {
    integerPart = binary.length();  // No decimal point, so the whole string is the integer part
    } else {
    integerPart = point;  // The position of the decimal point is where the integer part ends
    }

    
    // Converting the integer part (before the decimal point)
    for (int i = 0; i < integerPart; ++i) {
        if (binary[integerPart - 1 - i] == '1') {
            decimal += pow(2, i);
        }
    }

    // Converting the fractional part (after the decimal point)
    if (point != -1) {
        for (int i = 1; i < binary.length() - point; ++i) {
            if (binary[point + i] == '1') {
                decimal += 1.0 / pow(2, i);
            }
        }
    }

    return decimal;
}

int main() {
    string binary;
    
    cout << "Enter a binary number (including fractional part): ";
    cin >> binary;

    double decimal = binaryToDecimal(binary);
    
    cout << "Decimal equivalent: " << decimal << endl;
    
    return 0;
}
