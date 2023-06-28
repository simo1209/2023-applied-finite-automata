#include <map>
#include <string>
#include <iostream>
#include <vector>

class FSA
{
private:
    size_t initialState;
    size_t finalState;
    std::map<size_t, std::map<char, std::vector<size_t>>> transitions;

    size_t nextState;

public:
    FSA(char symbol);
    ~FSA();

    void print();
};

FSA::FSA(char symbol = '\0') : nextState(0), initialState(0)
{
    size_t currentState = nextState++;
    transitions[currentState][symbol].push_back(nextState);
    finalState = nextState;
}

FSA::~FSA()
{
}

void FSA::print()
{
    std::cout << "flowchart LR\n";
    for (const auto &[fromState, symbolToStates] : transitions)
    {
        for (const auto &[symbol, toStates] : symbolToStates)
        {
            for (const auto &toState : toStates)
            {
                if (symbol == '\0')
                {
                    std::cout << "\t" << fromState << "-- ε -->" << toState << "\n";
                }
                else
                {
                    std::cout << "\t" << fromState << "-- " << symbol << " -->" << toState << "\n";
                }
            }
        }
        std::cout << "\t" << fromState << "((" << fromState << "))\n";
    }
    std::cout << "\t" << finalState << "(((" << finalState << ")))\n";
}
