#include <unordered_map>
#include <string>
#include <iostream>
#include <vector>
#include <queue>
#include <stack>

constexpr char EPSILON = '\0';

class FSA
{
private:
    size_t initialState;
    size_t finalState;
    std::unordered_map<size_t, std::unordered_map<char, std::vector<size_t>>> transitions;

    static bool isSpecial(char ch);
    static bool isOperator(char ch);
    static int precedence(char op);
    static void process_operator(std::stack<FSA*> &automatas, char op);

    size_t nextState;
    void copyTransitionsWithOffset(size_t offset, const FSA &copyFrom, std::unordered_map<size_t, size_t> &visited);
    void copyTransitionsWithOffset(size_t offset, const FSA &copyFrom);

    void unionWith(const FSA &other);
    void concatenateWith(const FSA &other);
    void kleene();

public:
    FSA();
    FSA(char symbol);
    FSA(const FSA &other);
    ~FSA();

    void print() const;

    static FSA *parseExpression(const std::string &expression);
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
    std::cerr << "destroying " << initialState << " " << finalState << '\n';
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
    std::unordered_map<size_t, size_t> visited;
    copyTransitionsWithOffset(offset, other, visited);
}

void FSA::copyTransitionsWithOffset(size_t offset, const FSA &other, std::unordered_map<size_t, size_t> &visited)
{
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
                    offset++;
                    while ( transitions.find(offset) != transitions.end() || offset == finalState || offset == initialState ){
                        offset++;
                    }
                    std::cerr << "setting transition " << currentState << '(' << currentOtherState << ')' << ' ' << symbol << ' ' << offset << '(' << toState << ')' << std::endl;

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

void FSA::process_operator(std::stack<FSA *> &automatas, char op)
{
    FSA *a = automatas.top();
    automatas.pop();

    if (op == '*')
    {
        a->kleene();
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

FSA *FSA::parseExpression(const std::string &expression)
{
    std::stack<FSA*> automatas;
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
        else if ( ch == '*' )
        {
            if (!expect_operator) {
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
    // std::cerr << "automatas left: " << automatas.size() << '\n';
    while ( automatas.size() != 1 )
    {
        process_operator(automatas, '&');
    }


    return automatas.top();
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

    std::unordered_map<size_t, size_t> visited;
    copyTransitionsWithOffset(finalState, other, visited);
    finalState = visited[other.finalState];
    nextState = finalState + other.nextState;
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