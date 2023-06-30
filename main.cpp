#include <iostream>
#include <sstream>
#include <string>
#include <stack>
#include <map>

#include "FSA.cpp"

int main(int argc, char *argv[]) {

    if ( argc < 2 )
    {
        std::cerr << "Not enough arguments" << '\n';
        return 1;
    }

    std::string testExpression{argv[1]};

    std::cerr << "testing with: " << testExpression << '\n';

    NDA *test = NDA::parseExpression(testExpression);
    test->print();

    DFA *dTest = new DFA(*test);
    dTest->print();

    MinimizedDFA *mdTest = new MinimizedDFA(*dTest);
    mdTest->print();

    delete test;
    delete dTest;
    delete mdTest;

    return 0;
}
