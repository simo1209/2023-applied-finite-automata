#include <iostream>

#include "FSA.cpp"

int main() {

    FSA singleton('a');
    singleton.print();

    FSA epsilon;
    epsilon.print();

    return 0;
}