#include <map>
#include <string>
#include <iostream>
#include <vector>
#include <queue>

constexpr char EPSILON = '\0';

class FSA
{
private:
    size_t initialState;
    size_t finalState;
    std::map<size_t, std::map<char, std::vector<size_t>>> transitions;

    size_t nextState;
    void copyTransitionsWithOffset(size_t offset, const FSA &copyFrom);
    void unionWith(const FSA &other);
    void concatenateWith(const FSA &other);
    void kleene();

public:
    FSA();
    FSA(char symbol);
    FSA(const FSA &other);
    ~FSA();

    void print();

    static FSA unionExpression(const FSA &left, const FSA &right);
    static FSA concatenationExpression(const FSA &left, const FSA &right);
    static FSA kleeneExpression(const FSA &left);
};

FSA::FSA() : initialState(0), finalState(1), nextState(1)
{
}

FSA::FSA(char symbol) : initialState(0), nextState(0)
{
    size_t currentState = nextState++;
    transitions[currentState][symbol].push_back(nextState);
    finalState = nextState++;
}

FSA::FSA(const FSA &other) : initialState(other.initialState), finalState(other.finalState), nextState(other.nextState)
{
    copyTransitionsWithOffset(0, other);
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
                if (symbol == EPSILON)
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
    std::cout << "\t" << initialState << "((" << initialState << " init))\n";
    std::cout << "\t" << finalState << "(((" << finalState << ")))\n";
}

void FSA::copyTransitionsWithOffset(size_t offset, const FSA &other)
{
    std::map<size_t, size_t> visited;
    std::queue<std::pair<size_t, size_t>> fsaQueue;

    visited[other.initialState] = offset;
    fsaQueue.push(std::make_pair(other.initialState, offset));

    while (!fsaQueue.empty())
    {
        auto &[currentOtherState, currentState] = fsaQueue.front();
        std::cerr << "visiting " << currentOtherState << std::endl;
        std::cerr << "offset = " << currentState << std::endl;
        fsaQueue.pop();

        if (other.transitions.find(currentOtherState) != other.transitions.end())
        {
            for (const auto &[symbol, toStates] : other.transitions.at(currentOtherState))
            {
                for (const auto &toState : toStates)
                {
                    std::cerr << "setting transition " << currentState << ' ' << symbol << ' ' << ++offset << std::endl;
                    if (visited.find(toState) != visited.end())
                    {
                        std::cerr << toState << " already vsited. reusing " << visited[toState] << std::endl;
                        transitions[currentState][symbol].push_back(visited[toState]);
                        // fsaQueue.push(std::make_pair(toState, visited[toState]));
                    }
                    else
                    {
                        transitions[currentState][symbol].push_back(offset);
                        visited[toState] = offset;
                        fsaQueue.push(std::make_pair(toState, offset));
                    }
                }
            }
        }
    }

    nextState = offset + 1;
}

FSA FSA::unionExpression(const FSA &left, const FSA &right)
{
    FSA resultFSA(left);
    std::cerr << "rfa ns " << resultFSA.nextState << std::endl;

    resultFSA.unionWith(right);
    return resultFSA;
}

FSA FSA::concatenationExpression(const FSA &left, const FSA &right)
{
    FSA resultFSA(left);

    resultFSA.concatenateWith(right);
    return resultFSA;
}

FSA FSA::kleeneExpression(const FSA &left)
{
    FSA resultFSA(left);

    resultFSA.kleene();
    return resultFSA;
}

FSA FSA::parseExpression(const std::string &expression)
{
    FSA resultFSA;

    std::cout << expression << '\n';
    return resultFSA;
}

void FSA::unionWith(const FSA &other)
{
    std::cerr << "newInitialState " << nextState << std::endl;

    size_t newInitialState = nextState++;

    transitions[newInitialState][EPSILON].push_back(initialState);
    transitions[newInitialState][EPSILON].push_back(nextState);
    initialState = newInitialState;

    copyTransitionsWithOffset(nextState, other);
    std::cerr << "other" << other.finalState << '\n';
    std::cerr << "nextState= " << nextState << std::endl;

    transitions[finalState][EPSILON].push_back(nextState);
    transitions[nextState - 1][EPSILON].push_back(nextState);

    finalState = nextState++;
}

void FSA::concatenateWith(const FSA &other)
{
    std::cerr << "concatenating with " << other.initialState << ' ' << other.finalState << std::endl;

    copyTransitionsWithOffset(finalState, other);
    finalState = finalState + other.finalState;
    nextState = finalState + 1;
}

void FSA::kleene()
{
    transitions[finalState][EPSILON].push_back(initialState);

    transitions[nextState][EPSILON].push_back(initialState);
    initialState = nextState++;

    transitions[finalState][EPSILON].push_back(nextState);
    finalState = nextState++;

    transitions[initialState][EPSILON].push_back(finalState);
}