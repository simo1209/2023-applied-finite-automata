#include <map>
#include <string>
#include <iostream>
#include <vector>

class FSA
{
private:
    size_t finalState;
    std::map<size_t, std::map<char, std::vector<size_t>>> transitions;

    size_t nextState;

public:
    FSA(char symbol);
    ~FSA();

    void print();
};

FSA::FSA(char symbol = '\0') : nextState(0)
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
    for (const auto &[q, ars] : transitions)
    {
        for (const auto &[a, rs] : ars)
        {
            for(const auto &r : rs)
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
        }
        std::cout << "\t" << q << "((" << q << "))\n";
    }
    std::cout << "\t" << finalState << "(((" << finalState << ")))\n";
}
