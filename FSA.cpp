#include <map>
#include <string>
#include <iostream>

class FSA
{
private:
    size_t finalState;
    std::map<size_t, std::map<char, size_t>> transitions;

    size_t nextState;

public:
    FSA(char symbol);
    ~FSA();

    void print();
};

FSA::FSA(char symbol = '\0') : nextState(0)
{
    size_t currentState = nextState++;
    transitions[currentState][symbol] = nextState;
    finalState = nextState;
}

FSA::~FSA()
{
}

void FSA::print()
{
    std::cout << "flowchart LR\n";
    for (const auto &[q, ar] : transitions)
    {
        for (const auto &[a, r] : ar)
        {
            if (a == '\0')
            {
                std::cout << "\t" << q << "-- ε -->" << r << "\n";
            }
            else
            {
                std::cout << "\t" << q << "-- " << a << " -->" << r << "\n";
            }
        }
        std::cout << "\t" << q << "((" << q << "))\n";
    }
    std::cout << "\t" << finalState << "(((" << finalState << ")))\n";
}
