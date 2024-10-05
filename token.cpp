#include "token.h"

#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <sstream>

namespace {
std::map<char, char> k_parenthesis_map{{'(', ')'}, {'[', ']'}, {'{', '}'}};

std::set<std::string> k_functions{
    "sin",  "cos",  "tan",  "sec",  "csc",  "cot", "sinh",
    "cosh", "tanh", "sech", "csch", "coth", "ln",
};

Token *get_next_token(std::string const &expression, int &i) {
    char character = expression[i];

    if (k_parenthesis_map.contains(character)) {
        int count = 1;
        char const original = character;

        character = expression[++i];

        std::size_t const start = i;

        while (i < expression.size() && count) {
            if (character == original)
                ++count;

            else if (character == k_parenthesis_map[original])
                --count;

            character = expression[++i];
        }

        if (count)
            throw std::invalid_argument("Expression is not valid!");

        std::size_t const end = i - start - 1;

        auto *result = new Expression(tokenise(expression.substr(start, end)));

        if (result->tokens().size() == 1) {
            Token *token = copy(result->tokens()[0]);

            delete result;

            return token;
        }

        return result;
    }

    for (int const offset : {4, 3, 2})
        if (i + offset < expression.size())
            if (std::string const &fn = expression.substr(i, offset);
                k_functions.contains(fn)) {
                i += offset;

                return new Function(fn, nullptr, get_next_token(expression, i),
                                    nullptr);
            }

    if (character == 'e' && i < expression.size() - 1 &&
        expression[i + 1] == '^') {
        i += 2;

        return new Function("e^", nullptr, get_next_token(expression, i),
                            nullptr);
    }

    if (auto *op = new Operation(character); op->operation)
        return op;
    else
        delete op;

    if ('0' <= character && character <= '9') {
        std::string number;

        while (i < expression.size() && '0' < character && character < '9') {
            number.push_back(character);

            character = expression[++i];
        }

        if (i < expression.size()) {
            if (character != '.') {
                --i;

                return new Constant(std::stoull(number));
            }

            do {
                number.push_back(character);

                character = expression[++i];
            } while (i < expression.size() && '0' <= character &&
                     character <= '9');

            if (i < expression.size())
                --i;

            return new Constant(std::stold(number));
        }

        return new Constant(std::stoull(number));
    }

    if (('A' <= character && character <= 'Z') ||
        ('a' <= character && character <= 'z'))
        return new Variable(character);

    return nullptr;
}
} // namespace

Constant::Constant(long double const value) : value(value) {}

Constant::operator std::string() const { return std::to_string(this->value); }

Variable::Variable(char const var) : var(var) {}

Variable::operator std::string() const { return {this->var}; }

Operation::Operation(char const operation) {
    switch (operation) {
    case '+':
        this->operation = op::add;
        break;

    case '-':
        this->operation = op::sub;
        break;

    case '*':
        this->operation = op::mul;
        break;

    case '/':
        this->operation = op::div;
        break;

    case '^':
        this->operation = op::pow;
        break;

    default:
        break;
    }
}

Operation::operator std::string() const {
    if (!this->operation)
        return "";

    switch (*this->operation) {
    case op::add:
        return "+";
    case op::sub:
        return "-";
    case op::mul:
        return "*";
    case op::div:
        return "/";
    case op::pow:
        return "^";
    }

    return "";
}

Function::Function(std::string function, Constant *coefficient,
                   Token *parameter, Token *power)
    : function(std::move(function)), coefficient(coefficient),
      parameter(parameter), power(power) {}

Function::Function(Function const &function) {
    this->function = function.function;
    this->coefficient = dynamic_cast<Constant *>(copy(function.coefficient));
    this->parameter = copy(function.parameter);
    this->power = copy(function.power);
}

Function::operator std::string() const {
    std::stringstream result;

    if (this->coefficient->value != 1)
        result << std::to_string(this->coefficient->value);

    result << this->function << '('
           << static_cast<std::string>(*this->parameter) << ')';

    if (this->power)
        if (auto const *power = dynamic_cast<Constant *>(this->power);
            power && power->value != 1)
            result << '^' << static_cast<std::string>(*this->power);

    return result.str();
}

Expression::Expression(Term const &term) {
    if (term.coefficient)
        this->add_token(term.coefficient);

    if (term.base)
        this->add_token(term.base);

    if (term.power) {
        this->add_token(new Operation('^'));
        this->add_token(term.power);
    }
}

Expression::Expression(Expression const &expression) {
    for (Token *token : expression.tokens_)
        this->add_token(copy(token));
}

Expression::~Expression() noexcept {
    for (Token *&token : this->tokens_) {
        delete token;

        token = nullptr;
    }
}

Expression::operator std::string() const {
    std::stringstream result;

    if (this->tokens_.size() > 1) {
        result << '(';

        for (Token const *token : this->tokens_)
            result << static_cast<std::string>(*token);

        result << ')';
    } else {
        result << static_cast<std::string>(*this->tokens_[0]);
    }

    return result.str();
}

std::vector<Token *> &Expression::tokens() { return this->tokens_; }

std::vector<Token *> const &Expression::tokens() const { return this->tokens_; }

void Expression::add_token(Token *token) { this->tokens_.push_back(token); }

Expression tokenise(std::string expression) {
    Expression result{};

    auto e = std::ranges::remove(expression, ' ');

    expression.erase(e.begin(), e.end());

    for (int i = 0; i < expression.size(); ++i) {
        Token *token = get_next_token(expression, i);

        if (!token)
            continue;

        if (auto *function = dynamic_cast<Function *>(token)) {
            if (std::vector<Token *> &tokens = result.tokens(); !tokens.empty())
                if (auto *last = dynamic_cast<Constant *>(tokens.back())) {
                    function->coefficient = last;
                    tokens.pop_back();
                } else if (tokens.size() == 2) {
                    if (auto const *operation =
                            dynamic_cast<Operation *>(tokens.back());
                        operation &&
                        operation->operation == Operation::op::mul) {
                        auto *constant =
                            dynamic_cast<Constant *>(*(tokens.rbegin() + 1));

                        if (constant) {
                            function->coefficient = constant;
                            tokens.pop_back();
                            tokens.pop_back();
                        } else {
                            function->coefficient = new Constant(1);
                        }
                    } else {
                        function->coefficient = new Constant(1);
                    }
                } else {
                    function->coefficient = new Constant(1);
                }

            else
                function->coefficient = new Constant(1);

            if (i < expression.size()) {
                if (Operation operation{expression[i + 1]};
                    operation.operation) {
                    if (operation.operation == Operation::op::pow) {
                        i += 2;

                        function->power = get_next_token(expression, i);
                    } else {
                        function->power = new Constant(1);
                    }
                } else {
                    function->power = new Constant(1);
                }
            } else {
                function->power = new Constant(1);
            }
        }

        result.add_token(token);
    }

    return result;
}

std::ostream &operator<<(std::ostream &os, Token const &token) {
    os << static_cast<std::string>(token);

    return os;
}

Token *copy(Token *token) {
    if (auto const *constant = dynamic_cast<Constant *>(token))
        return new Constant(constant->value);
    if (auto const *variable = dynamic_cast<Variable *>(token))
        return new Variable(variable->var);
    if (auto const *operation = dynamic_cast<Operation *>(token))
        return new Operation(static_cast<std::string>(*operation)[0]);
    if (auto const *expression = dynamic_cast<Expression *>(token))
        return new Expression(*expression);
    if (auto const *function = dynamic_cast<Function *>(token))
        return new Function(*function);

    return nullptr;
}
