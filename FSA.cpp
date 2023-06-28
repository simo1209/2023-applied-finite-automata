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
    FSA();
    FSA(char symbol);
    ~FSA();

    void print();

    void copyTransitionsWithOffset(size_t offset, const FSA &copyFrom);
    static FSA unionExpression(const FSA &left, const FSA &right);
    static FSA concatenationExpression(const FSA &left, const FSA &right);
};

FSA::FSA() : initialState(0), finalState(1), nextState(1)
{
}

FSA::FSA(char symbol) : initialState(0), nextState(0)
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
        if (fromState != finalState && fromState != initialState)
        {
            std::cout << "\t" << fromState << "((" << fromState << "))\n";
        }
    }
    // std::cout << "\t" << initialState << "((" << initialState << " initial ))\n";
    std::cout << "\t" << initialState << "((" << initialState << "))\n";
    std::cout << "\t" << finalState << "(((" << finalState << ")))\n";
}

void FSA::copyTransitionsWithOffset(size_t offset, const FSA &other)
{
    for (const auto &[fromState, symbolToStates] : other.transitions)
    {
        for (const auto &[symbol, toStates] : symbolToStates)
        {
            for (const auto &toState : toStates)
            {
                transitions[fromState + offset][symbol].push_back(toState + offset);
            }
        }
    }
    nextState = other.finalState + offset + 1;
}

FSA FSA::unionExpression(const FSA &left, const FSA &right)
{
    FSA resultFSA;
    resultFSA.nextState = 1;

    std::cerr << "resultFSA nextState = " << resultFSA.nextState << '\n';

    resultFSA.transitions[0]['\0'].push_back(resultFSA.nextState);
    size_t leftOffset = resultFSA.nextState;
    resultFSA.copyTransitionsWithOffset(resultFSA.nextState, left);

    std::cerr << "resultFSA nextState = " << resultFSA.nextState << '\n';

    resultFSA.transitions[0]['\0'].push_back(resultFSA.nextState);
    size_t rightOffset = resultFSA.nextState;
    resultFSA.copyTransitionsWithOffset(resultFSA.nextState, right);

    resultFSA.finalState = ++resultFSA.nextState;
    resultFSA.transitions[left.finalState + leftOffset]['\0'].push_back(resultFSA.finalState);
    resultFSA.transitions[right.finalState + rightOffset]['\0'].push_back(resultFSA.finalState);

    return resultFSA;
}

FSA FSA::concatenationExpression(const FSA &left, const FSA &right)
{
    FSA resultFSA;
    resultFSA.nextState = 0;

    std::cerr << "resultFSA nextState = " << resultFSA.nextState << '\n';
    resultFSA.copyTransitionsWithOffset(resultFSA.nextState, left);
    resultFSA.nextState--;

    std::cerr << "resultFSA nextState = " << resultFSA.nextState << '\n';
    resultFSA.copyTransitionsWithOffset(resultFSA.nextState, right);
    resultFSA.nextState--;

    resultFSA.finalState = resultFSA.nextState++;

    return resultFSA;
}