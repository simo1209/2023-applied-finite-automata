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
#include <list>

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

    void complement();
    void intersect(const FSA &other);
    void difference(const FSA &other);

    void determinize();
    void minimize();

    std::unordered_set<size_t> epsilonClosure(const std::unordered_set<size_t> &startStates) const;
    void mergeStates(std::unordered_map<size_t, size_t> &partition);

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

    size_t numberOfTransitions = 0;
    for (const auto &[fromState, symbolToStates] : transitions)
    {
        for (const auto &[symbol, toStates] : symbolToStates)
        {
            for (const auto &toState : toStates)
            {
                ++numberOfTransitions;
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

    std::cerr << "Number of states: " << states.size() << '\n';
    std::cerr << "Number of transitions: " << numberOfTransitions << '\n';
}

void FSA::copyTransitionsWithOffset(size_t offset, const FSA &other)
{
    std::unordered_map<size_t, size_t> visited;
    copyTransitionsWithOffset(offset, other, visited);
}

void FSA::copyTransitionsWithOffset(size_t offset, const FSA &other, std::unordered_map<size_t, size_t> &visited)
{
    // auto start = std::chrono::high_resolution_clock::now();

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
    // auto end = std::chrono::high_resolution_clock::now();
    // std::cerr << "copying transitions took: " << end.time_since_epoch().count() - start.time_since_epoch().count() << " nanoseconds\n";
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
        a->determinize();
        automatas.push(a);
    }
    else if (op == '^')
    {
        a->reverse();
        automatas.push(a);
    }
    else if ( op == '~' )
    {
        a->complement();
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
            b->determinize();
            automatas.push(b);
            delete a;

            break;
        case '-':
            b->difference(*a);
            b->determinize();
            automatas.push(b);
            break;
        }
    }
}

FSA *FSA::parseExpression(const std::string &expression)
{
    auto timeStart = std::chrono::high_resolution_clock::now();

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
        else if (ch == '*' || ch == '^' || ch == '~')
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

    auto timeEnd = std::chrono::high_resolution_clock::now();
    std::cerr << "nda build took: " << timeEnd.time_since_epoch().count() - timeStart.time_since_epoch().count() << " nanoseconds\n";

    automatas.top()->print();

    automatas.top()->determinize();
    automatas.top()->minimize();
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
    // auto start = std::chrono::high_resolution_clock::now();

    size_t newInitialState = nextState++;
    states.insert(newInitialState);

    transitions[newInitialState][EPSILON].insert(initialState);
    transitions[newInitialState][EPSILON].insert(nextState);
    initialState = newInitialState;

    copyTransitionsWithOffset(nextState, other);

    if (finalStates.size() != 1)
    {
        // std::cerr << "undefined behav union" << '\n';
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

    // auto end = std::chrono::high_resolution_clock::now();
    // std::cerr << "union took: " << end.time_since_epoch().count() - start.time_since_epoch().count() << " nanoseconds\n";
}

void FSA::concatenateWith(const FSA &other)
{
    // auto start = std::chrono::high_resolution_clock::now();

    std::unordered_map<size_t, size_t> visited;
    if (finalStates.size() != 1)
    {
        // std::cerr << "undefined behav concat1" << '\n';
    }
    for (const auto &finalState : finalStates)
    {
        copyTransitionsWithOffset(finalState, other, visited);
    }

    finalStates = {};
    if (other.finalStates.size() != 1)
    {
        // std::cerr << "undefined behav concat2" << '\n';
    }
    for (const auto &otherFinalState : other.finalStates)
    {
        finalStates.insert(visited[otherFinalState]);
        nextState = visited[otherFinalState] + other.nextState;
    }

    // auto end = std::chrono::high_resolution_clock::now();
    // std::cerr << "concatenation took: " << end.time_since_epoch().count() - start.time_since_epoch().count() << " nanoseconds\n";
}

void FSA::kleene()
{
    if (finalStates.size() != 1)
    {
        // std::cerr << "undefined behav union" << '\n';
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

void FSA::complement()
{
    std::unordered_set<size_t> newFinalStates;

    for ( const auto &state : states )
    {
        if ( ! finalStates.count(state) )
        {
            newFinalStates.insert(state);
        }
    }

    finalStates = newFinalStates;
}

void FSA::intersect( const FSA& other )
{
    other.print();
}

void FSA::difference(const FSA& other)
{
    FSA c(other);
    c.complement();
    intersect(c);
}

std::unordered_set<size_t> FSA::epsilonClosure(const std::unordered_set<size_t> &states) const
// std::set<size_t> FSA::epsilonClosure(size_t state, std::unordered_map<size_t, std::unordered_set<size_t>> &epsilon_transitions)
{

    // auto timeStart = std::chrono::high_resolution_clock::now();

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

    // auto timeEnd = std::chrono::high_resolution_clock::now();
    // std::cerr << "epsilon closure took: " << timeEnd.time_since_epoch().count() - timeStart.time_since_epoch().count() << " nanoseconds\n";

    // epsilon_transitions[state] = closure;

    return closure;
}

std::string setToString(std::set<size_t> &s)
{
    std::string res = "";
    for (const auto &state : s)
    {
        res += std::to_string(state) + ",";
    }
    return res;
}

void FSA::determinize()
{
    auto timeStart = std::chrono::high_resolution_clock::now();
    // Use the power-set construction to create a deterministic FSA
    FSA dFSA;
    dFSA.initialState = 0;

    // std::unordered_map<std::unordered_set<size_t>, std::unordered_set<size_t>, StateSetHash, StateSetEqual> epsilonCache;

    std::queue<std::unordered_set<size_t>> unmarkedStates;
    auto initialClosure = epsilonClosure({initialState});
    // epsilonCache[{initialState}] = initialClosure;

    unmarkedStates.push(initialClosure);

    std::unordered_map<std::unordered_set<size_t>, size_t, StateSetHash, StateSetEqual> stateMapping;

    size_t stateID = 0;
    stateMapping[initialClosure] = stateID++;

    // StateSetEqual sse;

    while (!unmarkedStates.empty())
    {
        // std::cerr << "queue size: " << unmarkedStates.size() << '\n';
        // std::cerr << "cache size: " << epsilonCache.size() << '\n';
        // std::cerr << "map size: " << stateMapping.size() << '\n';

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
                        // if (epsilonCache.count({next}))
                        // {
                        // auto eClosure = epsilonCache[{next}];
                        // newState.insert(eClosure.begin(), eClosure.end());
                        // }
                        // else
                        // {
                        auto eClosure = epsilonClosure({next});
                        // epsilonCache[{next}] = eClosure;
                        newState.insert(eClosure.begin(), eClosure.end());
                        // }
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

            // if (!sse(currentState, newState))
            // {
            //     unmarkedStates.push(newState);
            // }

            dFSA.transitions[stateMapping[currentState]][ch] = {stateMapping[newState]};
        }
    }

    dFSA.states = std::unordered_set<size_t>(dFSA.finalStates.begin(), dFSA.finalStates.end());
    size_t transitions_count = 0;
    for (const auto &[fromState, symbolToStates] : dFSA.transitions)
    {
        dFSA.states.insert(fromState);
        for (const auto &[symbol, toStates] : symbolToStates)
        {
            for (const auto &toState : toStates)
            {
                dFSA.states.insert(toState);
                ++transitions_count;
            }
        }
    }

    this->initialState = dFSA.initialState;
    this->states = dFSA.states;
    this->finalStates = dFSA.finalStates;
    this->transitions = dFSA.transitions;

    auto timeEnd = std::chrono::high_resolution_clock::now();
    std::cerr << "det took: " << timeEnd.time_since_epoch().count() - timeStart.time_since_epoch().count() << " nanoseconds\n";
    std::cerr << "states count:" << states.size() << '\n';
    std::cerr << "transitions count:" << transitions_count << '\n';
}
/*
void FSA::minimize()
{
    auto timeStart = std::chrono::high_resolution_clock::now();

    std::unordered_set<size_t> nonFinalStates;
    std::set_difference(states.begin(), states.end(), finalStates.begin(), finalStates.end(),
                        std::inserter(nonFinalStates, nonFinalStates.begin()));

    // Initial partitions
    std::vector<std::unordered_set<size_t>> partitions = {finalStates, nonFinalStates};

    // The list of partitions to be checked
    std::list<std::unordered_set<size_t>> toCheck(partitions.begin(), partitions.end());

    while (!toCheck.empty())
    {
        // Pick and remove a partition from the list
        std::unordered_set<size_t> partition = toCheck.front();
        toCheck.pop_front();

        // Iterate over the set of input symbols
        for (char symbol = 'a'; symbol <= 'z'; ++symbol)
        {
            // States that transition to the current partition with the current symbol
            std::unordered_set<size_t> relatedStates;

            for (const auto &state : partition)
            {
                for (const auto &transition : transitions[state][symbol])
                {
                    relatedStates.insert(transition);
                }
            }

            // Iterate over all partitions
            for (auto it = partitions.begin(); it != partitions.end();)
            {
                // Split partitions in two: states that transition to the current partition and those that don't
                std::unordered_set<size_t> intersection, difference;
                std::set_intersection(it->begin(), it->end(), relatedStates.begin(), relatedStates.end(),
                                      std::inserter(intersection, intersection.begin()));
                std::set_difference(it->begin(), it->end(), relatedStates.begin(), relatedStates.end(),
                                    std::inserter(difference, difference.begin()));

                // If one of the sets is empty, don't split the partition
                if (intersection.empty() || difference.empty())
                {
                    ++it;
                    continue;
                }

                // Replace the original partition with the intersection and add the difference as a new partition
                *it = intersection;
                it = partitions.insert(it, difference);

                // If the original partition is in the list to be checked, replace it with the intersection
                if (std::find(toCheck.begin(), toCheck.end(), *it) != toCheck.end())
                {
                    toCheck.remove(*it);
                    toCheck.push_back(*it);
                    toCheck.push_back(intersection);
                }
                // Otherwise, add the smaller partition to the list to be checked
                else
                {
                    if (intersection.size() <= difference.size())
                    {
                        toCheck.push_back(intersection);
                    }
                    else
                    {
                        toCheck.push_back(difference);
                    }
                }
            }
        }
    }

    std::unordered_set<size_t> newStates;
    std::unordered_map<std::unordered_set<size_t> *, size_t> partitionIndices;
    size_t index = 0;
    for (auto &partition : partitions)
    {
        newStates.insert(index);
        partitionIndices[&partition] = index++;
    }

    std::unordered_map<size_t, std::unordered_map<char, std::unordered_set<size_t>>> newTransitions;
    std::unordered_set<size_t> newFinalStates;

    for (auto &partition : partitions)
    {
        size_t partitionIndex = partitionIndices[&partition];

        // Update final states
        std::unordered_set<size_t> intersection;
        std::set_intersection(partition.begin(), partition.end(), finalStates.begin(), finalStates.end(),
                              std::inserter(intersection, intersection.begin()));
        if (!intersection.empty())
        {
            newFinalStates.insert(partitionIndex);
        }

        // Update transitions
        size_t representativeState = *partition.begin(); // choose a representative state from each partition
        for (const auto &transition : transitions[representativeState])
        {
            char symbol = transition.first;
            size_t destinationState = *transition.second.begin(); // assuming there is only one transition for each pair of state and symbol

            // Find the partition of the destination state
            auto destinationPartition = std::find_if(partitions.begin(), partitions.end(),
                                                     [destinationState](const auto &partition)
                                                     {
                                                         return partition.find(destinationState) != partition.end();
                                                     });
            if (destinationPartition != partitions.end())
            {
                size_t destinationPartitionIndex = partitionIndices[&*destinationPartition];
                newTransitions[partitionIndex][symbol] = {destinationPartitionIndex};
            }
        }
    }

    // Update the DFA

    this->states = newStates;
    this->finalStates = newFinalStates;
    this->transitions = newTransitions;

    // this->initialState = newInitialState;
    // this->states = newStates;
    // this->finalStates = newFinalStates;
    // this->transitions = newTransitions;

    auto timeEnd = std::chrono::high_resolution_clock::now();
    std::cerr << "minimize took: " << timeEnd.time_since_epoch().count() - timeStart.time_since_epoch().count() << " nanoseconds\n";
}

*/

/*
void FSA::minimize()
{
    auto timeStart = std::chrono::high_resolution_clock::now();

    std::unordered_map<size_t, std::unordered_map<char, std::unordered_set<size_t>>> inverseTransitions;
    for ( const auto &[fromState, symbolToStates] : transitions )
    {
        for ( const auto &[symbol, toStates] : symbolToStates )
        {
            for ( const auto &toState : toStates)
            {
                inverseTransitions[toState][symbol].insert(fromState);
            }
        }
    }

    std::unordered_map<size_t, size_t> partition;

    std::vector<std::unordered_set<size_t>> P(states.size());

    for (auto &state : states)
    {
        if (finalStates.count(state) > 0)
        {
            P[0].insert(state);
            partition[state] = 0;
        }
        else
        {
            P[1].insert(state);
            partition[state] = 1;
        }
    }

    std::vector<std::unordered_set<size_t>> W = {P[0], P[1]};

    while (!W.empty())
    {
        std::unordered_set<size_t> A = W.back();
        W.pop_back();

        for (char a = 'a'; a <= 'z'; a++)
        {
            std::unordered_map<size_t, std::unordered_set<size_t>> connected;
            for (auto q : A)
            {
                for (auto state : states)
                {
                    if (transitions[state][a].count(q))
                    {
                        connected[partition[state]].insert(state);
                    }
                }
            }

            for (auto &[r, statesConnected] : connected)
            {
                if (statesConnected.size() < P[r].size())
                {
                    size_t j = P.size();
                    P.push_back({});
                    for (auto state : statesConnected)
                    {
                        P[r].erase(state);
                        P[j].insert(state);
                        partition[state] = j;
                    }

                    auto it = std::find(W.begin(), W.end(), P[r]);
                    if (it != W.end())
                    {
                        W.push_back(P[j]);
                    }
                    else
                    {
                        if (P[r].size() <= P[j].size())
                        {
                            W.push_back(P[r]);
                        }
                        else
                        {
                            W.push_back(P[j]);
                        }
                    }
                }
            }
        }
    }

    // Now merge states in the same partition and update the transitions
    mergeStates(partition);

    auto timeEnd = std::chrono::high_resolution_clock::now();
    std::cerr << "minimization took: " << timeEnd.time_since_epoch().count() - timeStart.time_since_epoch().count() << " nanoseconds\n";
}
*/

void FSA::minimize()
{
    std::unordered_map<size_t, std::unordered_map<char, std::unordered_set<size_t>>> inverseTransitions;
    for (const auto &[fromState, symbolToStates] : transitions)
    {
        for (const auto &[symbol, toStates] : symbolToStates)
        {
            for (const auto &toState : toStates)
            {
                inverseTransitions[toState][symbol].insert(fromState);
            }
        }
    }

    std::unordered_map<size_t, size_t> partition;
    std::vector<std::unordered_set<size_t>> P(states.size());

    for (auto &state : states)
    {
        if (finalStates.count(state) > 0)
        {
            P[0].insert(state);
            partition[state] = 0;
        }
        else
        {
            P[1].insert(state);
            partition[state] = 1;
        }
    }

    std::vector<std::unordered_set<size_t>> W = {P[0], P[1]};

    while (!W.empty())
    {
        std::unordered_set<size_t> A = W.back();
        W.pop_back();

        for (char a = 'a'; a <= 'z'; a++)
        {
            std::unordered_map<size_t, std::unordered_set<size_t>> connected;

            for (auto q : A)
            {
                // Use the inverse transition function to improve efficiency
                if (inverseTransitions[q].count(a))
                {
                    for (auto state : inverseTransitions[q][a])
                    {
                        connected[partition[state]].insert(state);
                    }
                }
            }

            for (auto &[r, statesConnected] : connected)
            {
                if (statesConnected.size() < P[r].size())
                {
                    size_t j = P.size();
                    P.push_back({});
                    for (auto state : statesConnected)
                    {
                        P[r].erase(state);
                        P[j].insert(state);
                        partition[state] = j;
                    }

                    auto it = std::find(W.begin(), W.end(), P[r]);
                    if (it != W.end())
                    {
                        W.push_back(P[j]);
                    }
                    else
                    {
                        if (P[r].size() <= P[j].size())
                        {
                            W.push_back(P[r]);
                        }
                        else
                        {
                            W.push_back(P[j]);
                        }
                    }
                }
            }
        }
    }

    // Now merge states in the same partition and update the transitions
    mergeStates(partition);
}

void FSA::mergeStates(std::unordered_map<size_t, size_t> &partition)
{
    std::unordered_map<size_t, size_t> representative;
    std::unordered_set<size_t> newFinalStates;
    std::unordered_map<size_t, std::unordered_map<char, std::unordered_set<size_t>>> newTransitions;

    for (const auto &[state, part] : partition)
    {
        if (representative.find(part) == representative.end())
        {
            representative[part] = state;
        }
        if (finalStates.find(state) != finalStates.end())
        {
            newFinalStates.insert(representative[part]);
        }
    }

    for (const auto &[state, part] : partition)
    {
        for (const auto &[a, dest] : transitions[state])
        {
            for (const auto &toState : dest)
            {
                newTransitions[representative[part]][a].insert(representative[partition[toState]]);
            }
        }
    }

    // Update FSA states, finalStates, and transitions
    states.clear();
    for (const auto &[_, state] : representative)
    {
        states.insert(state);
    }

    finalStates = newFinalStates;
    transitions = newTransitions;
    initialState = representative[partition[initialState]];
}