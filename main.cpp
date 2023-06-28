#include <iostream>

#include "FSA.cpp"

int main() {

    FSA singletonA('a');
    // singletonA.print();

    FSA singletonB('b');
    // singletonB.print();

    FSA ab = FSA::unionExpression(singletonA, singletonB);
    ab.print();

    return 0;
}