#include <iostream>

#include "FSA.cpp"

int main() {

    FSA emptyWord;

    FSA singletonA('a');
    // singletonA.print();

    FSA singletonB('b');
    // singletonB.print();

    FSA ab = FSA::unionExpression(singletonA, singletonB);
    // ab.print();

    FSA aab = FSA::concatenationExpression(singletonA, ab);
    aab.print();

    return 0;
}