#include <unordered_map>
#include <string>
#include <iostream>
#include <vector>
#include <queue>
#include <stack>
#include <set>

constexpr char EPSILON = '\0';

class NDA
{
private:
    size_t initialState;
    size_t finalState;
    std::unordered_map<size_t, std::unordered_map<char, std::vector<size_t>>> transitions;

    static bool isSpecial(char ch);
    static bool isOperator(char ch);
    static int precedence(char op);
    static void process_operator(std::stack<NDA *> &automatas, char op);

    size_t nextState;
    void copyTransitionsWithOffset(size_t offset, const NDA &copyFrom, std::unordered_map<size_t, size_t> &visited);
    void copyTransitionsWithOffset(size_t offset, const NDA &copyFrom);

    void unionWith(const NDA &other);
    void concatenateWith(const NDA &other);
    void kleene();

public:
    NDA();
    NDA(char symbol);
    NDA(const NDA &other);
    ~NDA();

    void print() const;

    static NDA *parseExpression(const std::string &expression);

    friend class DFA;
};

NDA::NDA() : initialState(0), finalState(1), nextState(1)
{
}

NDA::NDA(char symbol) : initialState(0), nextState(0)
{
    size_t currentState = nextState++;
    transitions[currentState][symbol].push_back(nextState);
    finalState = nextState++;
}

NDA::NDA(const NDA &other) : initialState(other.initialState), finalState(other.finalState), nextState(other.nextState)
{
    copyTransitionsWithOffset(0, other);
}

NDA::~NDA()
{
    std::cerr << "destroying " << initialState << " " << finalState << '\n';
}

void NDA::print() const
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

void NDA::copyTransitionsWithOffset(size_t offset, const NDA &other)
{
    std::unordered_map<size_t, size_t> visited;
    copyTransitionsWithOffset(offset, other, visited);
}

void NDA::copyTransitionsWithOffset(size_t offset, const NDA &other, std::unordered_map<size_t, size_t> &visited)
{
    std::queue<std::pair<size_t, size_t>> NDAQueue;

    visited[other.initialState] = offset;
    NDAQueue.push(std::make_pair(other.initialState, offset));

    while (!NDAQueue.empty())
    {
        auto &[currentOtherState, currentState] = NDAQueue.front();
        std::cerr << "visiting " << currentOtherState << std::endl;
        std::cerr << "offset = " << currentState << std::endl;
        NDAQueue.pop();

        if (other.transitions.find(currentOtherState) != other.transitions.end())
        {
            for (const auto &[symbol, toStates] : other.transitions.at(currentOtherState))
            {
                for (const auto &toState : toStates)
                {
                    offset++;
                    while (transitions.find(offset) != transitions.end() || offset == finalState || offset == initialState)
                    {
                        offset++;
                    }
                    std::cerr << "setting transition " << currentState << '(' << currentOtherState << ')' << ' ' << symbol << ' ' << offset << '(' << toState << ')' << std::endl;

                    if (visited.find(toState) != visited.end())
                    {
                        std::cerr << toState << " already vsited. reusing " << visited[toState] << std::endl;
                        transitions[currentState][symbol].push_back(visited[toState]);
                        // NDAQueue.push(std::make_pair(toState, visited[toState]));
                    }
                    else
                    {
                        transitions[currentState][symbol].push_back(offset);
                        visited[toState] = offset;
                        NDAQueue.push(std::make_pair(toState, offset));
                    }
                }
            }
        }
    }

    nextState = offset + 1;
}

bool NDA::isOperator(char ch)
{
    return ch == '*' || ch == '&' || ch == '|' || ch == '^';
}

bool NDA::isSpecial(char ch)
{
    return isOperator(ch) || ch == '(' || ch == ')';
}

int NDA::precedence(char op)
{
    switch (op)
    {
    case '*':
        return 3;
    case '&':
        return 2;
    case '|':
    case '^':
        return 1;
    default:
        return -1;
    }
}

void NDA::process_operator(std::stack<NDA *> &automatas, char op)
{
    NDA *a = automatas.top();
    automatas.pop();

    if (op == '*')
    {
        a->kleene();
        automatas.push(a);
    }
    else
    {
        NDA *b = automatas.top();
        automatas.pop();
        switch (op)
        {
        case '&':
            b->concatenateWith(*a);
            automatas.push(b);
            break;
        case '|':
            b->unionWith(*a);
            automatas.push(b);
            break;
        case '^':
            break;
        }
        delete a;
    }
}

NDA *NDA::parseExpression(const std::string &expression)
{
    std::stack<NDA *> automatas;
    std::stack<char> operators;

    bool expect_operator = false;

    for (char ch : expression)
    {
        if (isspace(ch))
        {
            continue;
        }

        if (ch == '(')
        {
            if (expect_operator)
            {
                while (!operators.empty() && precedence('&') <= precedence(operators.top()))
                {
                    process_operator(automatas, operators.top());
                    operators.pop();
                }
                operators.push('&');
            }
            operators.push(ch);
            expect_operator = false;
        }
        else if (ch == ')')
        {
            while (operators.top() != '(')
            {
                process_operator(automatas, operators.top());
                operators.pop();
            }
            operators.pop();
            expect_operator = true;
        }
        else if (ch == '*')
        {
            if (!expect_operator)
            {
                throw std::runtime_error("Unexpected character: " + std::string(1, ch));
            }
            process_operator(automatas, ch);
        }
        else if (isOperator(ch))
        {
            while (!operators.empty() && precedence(ch) <= precedence(operators.top()))
            {
                process_operator(automatas, operators.top());
                operators.pop();
            }
            operators.push(ch);
            expect_operator = false;
        }
        else
        {
            if (expect_operator)
            {
                while (!operators.empty() && precedence('&') <= precedence(operators.top()))
                {
                    process_operator(automatas, operators.top());
                    operators.pop();
                }
                operators.push('&');
            }
            automatas.push(new NDA(ch));
            expect_operator = true;
        }
    }

    while (!operators.empty())
    {
        process_operator(automatas, operators.top());
        operators.pop();
    }
    // std::cerr << "automatas left: " << automatas.size() << '\n';
    while (automatas.size() != 1)
    {
        process_operator(automatas, '&');
    }

    return automatas.top();
}

void NDA::unionWith(const NDA &other)
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

void NDA::concatenateWith(const NDA &other)
{
    std::cerr << "concatenating with " << other.initialState << ' ' << other.finalState << std::endl;

    std::unordered_map<size_t, size_t> visited;
    copyTransitionsWithOffset(finalState, other, visited);
    finalState = visited[other.finalState];
    nextState = finalState + other.nextState;
}

void NDA::kleene()
{
    transitions[finalState][EPSILON].push_back(initialState);

    transitions[nextState][EPSILON].push_back(initialState);
    initialState = nextState++;

    transitions[finalState][EPSILON].push_back(nextState);
    finalState = nextState++;

    transitions[initialState][EPSILON].push_back(finalState);
}

class DFA
{
private:
    std::set<size_t> initialState;
    std::set<std::set<size_t>> finalStates;
    std::map<std::set<size_t>, std::unordered_map<char, std::set<size_t>>> transitions;

    void printStates(const std::set<size_t> &states) const;

    static std::set<size_t> epsilonClosure(const size_t &state, NDA &nda);

public:
    DFA();
    DFA(NDA &nda);
    ~DFA();

    void print();
};

DFA::DFA()
{
}

std::set<size_t> DFA::epsilonClosure(const size_t &state, NDA &nda)
{
    std::set<size_t> closure = {state};

    // If there are epsilon transitions from the state
    if (nda.transitions.count(state) && nda.transitions.at(state).count('\0'))
    {
        for (const auto &newState : nda.transitions.at(state).at('\0'))
        {
            // Get the epsilon-closure of the new state
            std::set<size_t> newClosure = epsilonClosure(newState, nda);

            // Merge the new closure with the existing one
            closure.insert(newClosure.begin(), newClosure.end());
        }
    }

    return closure;
}

DFA::DFA(NDA &nda)
{
    std::queue<std::set<size_t>> toProcess;
    std::set<std::set<size_t>> processed;
    
    initialState = epsilonClosure(nda.initialState, nda);
    toProcess.push(initialState);

    while (!toProcess.empty())
    {
        std::set<size_t> currentStates = toProcess.front();
        toProcess.pop();

        if (processed.count(currentStates))
        {
            continue;
        }

        processed.insert(currentStates);

        // Check if the current DFA state is an accepting state.
        if (currentStates.count(nda.finalState))
        {
            finalStates.insert(currentStates);
        }

        std::unordered_map<char, std::set<size_t>> toStatesByKey;
        for (const auto &state : currentStates)
        {
            if (nda.transitions.count(state))
            {
                for (const auto &[symbol, toStates] : nda.transitions.at(state))
                {
                    if (symbol)
                    {
                        for (const auto &toState : toStates)
                        {
                            std::set<size_t> newClosure = epsilonClosure(toState, nda);
                            toStatesByKey[symbol].insert(newClosure.begin(), newClosure.end());
                        }
                    }
                }
            }
        }
        for (const auto &[symbol, toStates] : toStatesByKey)
        {
            transitions[currentStates][symbol].insert(toStates.begin(), toStates.end());

            if (!processed.count(toStates))
            {
                toProcess.push(toStates);
            }
        }
    }
}

DFA::~DFA()
{
}

void DFA::printStates(const std::set<size_t> &states) const
{
    for (const auto &state : states)
    {
        std::cout << state << ' ';
    }
}

void DFA::print()
{
    std::cout << "flowchart LR\n";

    size_t indexCounter = 0;
    std::map<std::set<size_t>, size_t> statesIndecies;

    for (const auto &[fromStates, symbolToStates] : transitions)
    {
        if (statesIndecies.find(fromStates) == statesIndecies.end())
        {
            statesIndecies[fromStates] = indexCounter++;
        }

        for (const auto &[symbol, toStates] : symbolToStates)
        {
            if (statesIndecies.find(toStates) == statesIndecies.end())
            {
                statesIndecies[toStates] = indexCounter++;
            }

            // for (const auto &toState : toStates)
            // {
            if (symbol == EPSILON)
            {
                std::cout << "\t" << statesIndecies[fromStates] << "-- ε -->" << statesIndecies[toStates] << "\n";
            }
            else
            {
                std::cout << "\t" << statesIndecies[fromStates] << "-- " << symbol << " -->" << statesIndecies[toStates] << "\n";
            }
            // }
        }
        if (finalStates.count(fromStates) == 0 && fromStates != initialState)
        {
            std::cout << "\t" << statesIndecies[fromStates] << "((" << statesIndecies[fromStates] << "))\n";
        }
    }
    // std::cout << "\t" << initialState << "((" << initialState << " initial ))\n";
    std::cout << "\t" << statesIndecies[initialState] << "((";
    printStates(initialState);
    std::cout << " init))\n";

    for (const auto &finalState : finalStates)
    {
        std::cout << "\t" << statesIndecies[finalState] << "(((";
        printStates(finalState);
        std::cout << ")))\n";
    }
}
