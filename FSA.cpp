#include <unordered_map>
#include <string>
#include <iostream>
#include <vector>
#include <queue>
#include <stack>
#include <set>
#include <unordered_set>
#include <chrono>
#include <bitset>
#include <algorithm>

constexpr char EPSILON = '\0';

struct StateSetHash
{
    std::size_t operator()(const std::unordered_set<size_t> &states) const
    {
        std::size_t sum = 0;
        for (const auto &state : states)
        {
            sum += state * state;
        }
        return sum;
    }
};

struct StateSetEqual
{
    bool operator()(const std::unordered_set<size_t> &lhs, const std::unordered_set<size_t> &rhs) const
    {
        if (lhs.size() != rhs.size())
        {
            return false;
        }

        for (const auto &state : lhs)
        {
            if (rhs.count(state) == 0)
            {
                return false;
            }
        }

        return true;
    }
};

class FSA
{
private:
    size_t initialState;
    std::unordered_set<size_t> states;
    std::unordered_set<size_t> finalStates;
    std::unordered_map<size_t, std::unordered_map<char, std::unordered_set<size_t>>> transitions;

    static bool isSpecial(char ch);
    static bool isOperator(char ch);
    static int precedence(char op);
    static void process_operator(std::stack<FSA *> &automatas, char op);

    size_t nextState;
    void copyTransitionsWithOffset(size_t offset, const FSA &copyFrom, std::unordered_map<size_t, size_t> &visited);
    void copyTransitionsWithOffset(size_t offset, const FSA &copyFrom);

    void unionWith(const FSA &other);
    void concatenateWith(const FSA &other);
    void kleene();
    void reverse();

    void determinize();

    std::unordered_set<size_t> epsilonClosure(const std::unordered_set<size_t> &startStates, std::unordered_map<std::unordered_set<size_t>, std::unordered_set<size_t>, StateSetHash, StateSetEqual> &epsilonCache) const;

public:
    FSA();
    FSA(char symbol);
    FSA(const FSA &other);
    ~FSA();

    void print() const;

    static FSA *parseExpression(const std::string &expression);
};

FSA::FSA() : initialState(0), states({0, 1}), finalStates({1}), nextState(2)
{
}

FSA::FSA(char symbol) : initialState(0), states({0, 1}), finalStates({1}), nextState(2)
{
    transitions[0][symbol].insert(1);
}

FSA::FSA(const FSA &other) : initialState(other.initialState), finalStates(other.finalStates), nextState(other.nextState)
{
    auto start = std::chrono::high_resolution_clock::now();

    copyTransitionsWithOffset(0, other);

    auto end = std::chrono::high_resolution_clock::now();
    std::cerr << "Copy FSA took: " << end.time_since_epoch().count() - start.time_since_epoch().count() << '\n';
}

FSA::~FSA()
{
    states.clear();
    finalStates.clear();
    transitions.clear();
}

void FSA::print() const
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
    }
    // std::cout << "\t" << initialState << "((" << initialState << " initial ))\n";
    for (const auto &state : states)
    {
        if (finalStates.count(state))
        {
            std::cout << "\t" << state << "(((" << state << ")))\n";
        }
        else if (state == initialState)
        {
            std::cout << "\t" << state << "((" << state << " init))\n";
        }
        else
        {
            std::cout << "\t" << state << "((" << state << " ))\n";
        }
    }
}

void FSA::copyTransitionsWithOffset(size_t offset, const FSA &other)
{
    std::unordered_map<size_t, size_t> visited;
    copyTransitionsWithOffset(offset, other, visited);
}

void FSA::copyTransitionsWithOffset(size_t offset, const FSA &other, std::unordered_map<size_t, size_t> &visited)
{
    auto start = std::chrono::high_resolution_clock::now();
    std::queue<std::pair<size_t, size_t>> FSAQueue;

    visited[other.initialState] = offset;
    states.insert(offset);
    FSAQueue.push(std::make_pair(other.initialState, offset));

    while (!FSAQueue.empty())
    {
        auto &[currentOtherState, currentState] = FSAQueue.front();
        FSAQueue.pop();

        if (other.transitions.find(currentOtherState) != other.transitions.end())
        {
            for (const auto &[symbol, toStates] : other.transitions.at(currentOtherState))
            {
                for (const auto &toState : toStates)
                {
                    offset++;
                    while (transitions.find(offset) != transitions.end() || finalStates.count(offset) || offset == initialState)
                    {
                        offset++;
                    }

                    if (visited.find(toState) != visited.end())
                    {
                        transitions[currentState][symbol].insert(visited[toState]);
                        // FSAQueue.push(std::make_pair(toState, visited[toState]));
                    }
                    else
                    {
                        transitions[currentState][symbol].insert(offset);
                        visited[toState] = offset;
                        states.insert(offset);
                        FSAQueue.push(std::make_pair(toState, offset));
                    }
                }
            }
        }
    }

    nextState = offset + 1;
    auto end = std::chrono::high_resolution_clock::now();
    std::cerr << "copying transitions took: " << end.time_since_epoch().count() - start.time_since_epoch().count() << " nanoseconds\n";
}

bool FSA::isOperator(char ch)
{
    return ch == '*' || ch == '&' || ch == '|' || ch == '^';
}

bool FSA::isSpecial(char ch)
{
    return isOperator(ch) || ch == '(' || ch == ')';
}

int FSA::precedence(char op)
{
    switch (op)
    {
    case '*':
    case '^':
        return 3;
    case '&':
        return 2;
    case '|':
        return 1;
    default:
        return -1;
    }
}

void FSA::process_operator(std::stack<FSA *> &automatas, char op)
{
    FSA *a = automatas.top();
    automatas.pop();

    if (op == '*')
    {
        a->kleene();
        automatas.push(a);
    }
    else if (op == '^')
    {
        a->reverse();
        automatas.push(a);
    }
    else
    {
        FSA *b = automatas.top();
        automatas.pop();
        switch (op)
        {
        case '&':
            b->concatenateWith(*a);
            automatas.push(b);
            delete a;

            break;
        case '|':
            b->unionWith(*a);
            automatas.push(b);
            delete a;

            break;
        case '^':
            break;
        }
    }
}

FSA *FSA::parseExpression(const std::string &expression)
{
    std::stack<FSA *> automatas;
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
        else if (ch == '*' || ch == '^')
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
            automatas.push(new FSA(ch));
            expect_operator = true;
        }
    }

    while (!operators.empty())
    {
        process_operator(automatas, operators.top());
        operators.pop();
    }

    while (automatas.size() != 1)
    {
        process_operator(automatas, '&');
    }

    automatas.top()->determinize();
    return automatas.top();
}

/*
bool FSA::accepts(const std::string &word)
{
    std::queue<std::pair<size_t, size_t>> nextStates;

    size_t wordCursor = 0;
    size_t wordSize = word.size();

    if (transitions[initialState].count('\0'))
    {
        for (const &toState : transitions[initialState]['\0'])
        {
            nextStates.push(std::make_pair(toState, initialState));
        }
    }

    while (wordCursor < wordSize && !nextStates.empty())
    {
        auto &[currentState, previousState] = nextStates.front();
        nextStates.pop();

        if (transitions[currentState].count('\0'))
        {
            for (const &toState : transitions[currentState]['\0'])
            {
                if (toState != previousState)
                {
                    nextStates.push(std::make_pair(toState, currentState));
                }
            }
        }
        if (transitions[currentState].count(word[wordCursor]))
        {
            for (const &toState : transitions[currentState][word[wordCursor]])
            {
                nextStates.push(std::make_pair(toState, currentState));
            }
        }
    }

    bool containsFinal = false;
    while (!nextStates.empty())
    {
        auto &[currentState, previousState] = nextStates.front();
        nextStates.pop();

        if ( previousState == finalState )
        {
            containsFinal = true;
            break;
        }
    }

    return wordCursor >= wordSize && containsFinal;
}
*/
void FSA::unionWith(const FSA &other)
{
    auto start = std::chrono::high_resolution_clock::now();
    size_t newInitialState = nextState++;
    states.insert(newInitialState);

    transitions[newInitialState][EPSILON].insert(initialState);
    transitions[newInitialState][EPSILON].insert(nextState);
    initialState = newInitialState;

    copyTransitionsWithOffset(nextState, other);

    if (finalStates.size() != 1)
    {
        std::cerr << "undefined behav union" << '\n';
    }

    for (const auto &finalState : finalStates)
    {
        transitions[finalState][EPSILON].insert(nextState);
        states.insert(finalState);
    }
    transitions[nextState - 1][EPSILON].insert(nextState);
    states.insert(nextState - 1);

    states.insert(nextState);
    finalStates = {nextState++};
    // finalState = nextState++;
    auto end = std::chrono::high_resolution_clock::now();
    std::cerr << "union took: " << end.time_since_epoch().count() - start.time_since_epoch().count() << " nanoseconds\n";
}

void FSA::concatenateWith(const FSA &other)
{
    auto start = std::chrono::high_resolution_clock::now();

    std::unordered_map<size_t, size_t> visited;
    if (finalStates.size() != 1)
    {
        std::cerr << "undefined behav concat1" << '\n';
    }
    for (const auto &finalState : finalStates)
    {
        copyTransitionsWithOffset(finalState, other, visited);
    }

    finalStates = {};
    if (other.finalStates.size() != 1)
    {
        std::cerr << "undefined behav concat2" << '\n';
    }
    for (const auto &otherFinalState : other.finalStates)
    {
        finalStates.insert(visited[otherFinalState]);
        nextState = visited[otherFinalState] + other.nextState;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::cerr << "concatenation took: " << end.time_since_epoch().count() - start.time_since_epoch().count() << " nanoseconds\n";
}

void FSA::kleene()
{
    if (finalStates.size() != 1)
    {
        std::cerr << "undefined behav union" << '\n';
    }
    for (const auto &finalState : finalStates)
    {
        transitions[finalState][EPSILON].insert(initialState);
    }

    states.insert(nextState);
    transitions[nextState][EPSILON].insert(initialState);
    initialState = nextState++;

    for (const auto &finalState : finalStates)
    {
        transitions[finalState][EPSILON].insert(nextState);
    }

    states.insert(nextState);
    finalStates = {nextState++};

    transitions[initialState][EPSILON].insert(finalStates.begin(), finalStates.end());
}

void FSA::reverse()
{
    auto timeStart = std::chrono::high_resolution_clock::now();

    std::unordered_map<size_t, std::unordered_map<char, std::unordered_set<size_t>>> newTransitions;
    for (const auto &[fromState, symbolToStates] : transitions)
    {
        for (const auto &[symbol, toStates] : symbolToStates)
        {
            for (const auto &toState : toStates)
            {
                newTransitions[toState][symbol].insert(fromState);
            }
        }
    }

    transitions = newTransitions;

    std::unordered_set<size_t> newFinalStates = {initialState};
    initialState = nextState++;
    states.insert(initialState);
    transitions[initialState][EPSILON].insert(finalStates.begin(), finalStates.end());
    finalStates = newFinalStates;

    auto timeEnd = std::chrono::high_resolution_clock::now();
    std::cerr << "reverse took: " << timeEnd.time_since_epoch().count() - timeStart.time_since_epoch().count() << " nanoseconds\n";
}

std::unordered_set<size_t> FSA::epsilonClosure(const std::unordered_set<size_t> &states, std::unordered_map<std::unordered_set<size_t>, std::unordered_set<size_t>, StateSetHash, StateSetEqual> &epsilonCache) const
// std::set<size_t> FSA::epsilonClosure(size_t state, std::unordered_map<size_t, std::unordered_set<size_t>> &epsilon_transitions)
{
    if (epsilonCache.count(states))
    {
        return epsilonCache[states];
    }

    auto timeStart = std::chrono::high_resolution_clock::now();

    std::unordered_set<size_t> closure = states;

    std::stack<size_t> stack;
    for (const auto &state : states)
    {
        stack.push(state);
    }

    while (!stack.empty())
    {
        size_t state = stack.top();
        stack.pop();

        // assuming epsilon transitions are represented by a special character, for example '#'
        if (transitions.count(state) && transitions.at(state).count('\0'))
        {
            for (const auto &newState : transitions.at(state).at('\0'))
            {
                if (closure.insert(newState).second)
                {
                    stack.push(newState);
                }
            }
        }
    }

    auto timeEnd = std::chrono::high_resolution_clock::now();
    std::cerr << "epsilon closure took: " << timeEnd.time_since_epoch().count() - timeStart.time_since_epoch().count() << " nanoseconds\n";

    // epsilon_transitions[state] = closure;

    epsilonCache[states] = closure;
    return closure;
}

void FSA::determinize()
{
    auto timeStart = std::chrono::high_resolution_clock::now();
    // Use the power-set construction to create a deterministic FSA
    FSA dFSA;
    dFSA.initialState = 0;

    std::unordered_map<std::unordered_set<size_t>, std::unordered_set<size_t>, StateSetHash, StateSetEqual> epsilonCache;

    std::queue<std::unordered_set<size_t>> unmarkedStates;
    auto initialClosure = epsilonClosure({initialState}, epsilonCache);
    unmarkedStates.push(initialClosure);

    std::unordered_map<std::unordered_set<size_t>, size_t, StateSetHash, StateSetEqual> stateMapping;
    size_t stateID = 0;
    stateMapping[initialClosure] = stateID++;

    while (!unmarkedStates.empty())
    {
        auto currentState = unmarkedStates.front();
        unmarkedStates.pop();

        if (std::any_of(currentState.begin(), currentState.end(), [&](size_t s)
                        { return finalStates.count(s); }))
        {
            dFSA.finalStates.insert(stateMapping[currentState]);
        }

        for (char ch = 'a'; ch <= 'z'; ch++)
        {
            std::unordered_set<size_t> newState;
            for (const auto &state : currentState)
            {
                if (transitions.count(state) && transitions.at(state).count(ch))
                {
                    for (const auto &next : transitions.at(state).at(ch))
                    {
                        auto eClosure = epsilonClosure({next}, epsilonCache);
                        newState.insert(eClosure.begin(), eClosure.end());
                    }
                }
            }

            if (newState.empty())
                continue;

            if (stateMapping.count(newState) == 0)
            {
                stateMapping[newState] = stateID++;
                unmarkedStates.push(newState);
            }

            dFSA.transitions[stateMapping[currentState]][ch] = {stateMapping[newState]};
        }
    }

    dFSA.states = std::unordered_set<size_t>(dFSA.finalStates.begin(), dFSA.finalStates.end());
    for (const auto &transition : dFSA.transitions)
    {
        dFSA.states.insert(transition.first);
        for (const auto &destination : transition.second)
            dFSA.states.insert(destination.second.begin(), destination.second.end());
    }

    this->initialState = dFSA.initialState;
    this->states = dFSA.states;
    this->finalStates = dFSA.finalStates;
    this->transitions = dFSA.transitions;

    auto timeEnd = std::chrono::high_resolution_clock::now();
    std::cerr << "det took: " << timeEnd.time_since_epoch().count() - timeStart.time_since_epoch().count() << " nanoseconds\n";
}
