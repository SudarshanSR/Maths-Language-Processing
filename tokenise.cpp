#include "tokenise.h"

#include <map>
#include <optional>

namespace {
std::map<char, char> k_parenthesis_map{{'(', ')'}, {'[', ']'}, {'{', '}'}};
}

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
        break;
    }

    return "";
}

Expression::Expression(Term const &term) {
    if (term.constant)
        this->add_token(term.constant);

    if (term.base)
        this->add_token(term.base);

    if (term.power) {
        this->add_token(new Operation('^'));
        this->add_token(term.power);
    }
}

Expression::Expression(Expression const &expression) {
    for (Token *token : expression.tokens_) {
        auto const &info = typeid(token);

        if (info == typeid(Constant))
            this->add_token(
                new Constant(dynamic_cast<Constant *>(token)->value));
        else if (info == typeid(Variable))
            this->add_token(new Variable(dynamic_cast<Variable *>(token)->var));
        else if (info == typeid(Operation))
            this->add_token(new Operation(static_cast<std::string>(
                *dynamic_cast<Operation *>(token))[0]));
        else if (info == typeid(Expression))
            this->add_token(new Expression(*dynamic_cast<Expression *>(token)));
    }
}

Expression::~Expression() noexcept {
    for (Token *&token : this->tokens_) {
        if (token)
            delete token;

        token = nullptr;
    }
}

Expression::operator std::string() const {
    std::string result;

    result += '(';

    for (Token const *token : this->tokens_)
        result += static_cast<std::string>(*token);

    result += ')';

    return result;
}

std::vector<Token *> const &Expression::tokens() const { return this->tokens_; }

void Expression::add_token(Token *token) { this->tokens_.push_back(token); }

Expression tokenise(std::string const &expression) {
    Expression result{};

    for (int i = 0; i < expression.size(); ++i) {
        char character = expression[i];

        if (character == ' ')
            continue;

        if (k_parenthesis_map.contains(character)) {
            int count = 1;
            char original = character;

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

            std::size_t const end = i - start;

            result.add_token(
                new Expression(tokenise(expression.substr(start, end))));

            continue;
        }

        if (auto *op = new Operation(character); op->operation) {
            result.add_token(op);

            continue;
        } else {
            delete op;
        }

        if ('0' <= character && character <= '9') {
            std::string number;

            while (i < expression.size() && '0' < character &&
                   character < '9') {
                number.push_back(character);

                character = expression[++i];
            }

            if (i < expression.size()) {
                if (character == '.') {
                    do {
                        number.push_back(character);

                        character = expression[++i];
                    } while (i < expression.size() && '0' <= character &&
                             character <= '9');

                    result.add_token(new Constant(std::stold(number)));

                    if (i < expression.size())
                        --i;
                } else {
                    result.add_token(new Constant(std::stoull(number)));

                    --i;
                }

            } else {
                result.add_token(new Constant(std::stoull(number)));
            }

            continue;
        }

        if (('A' <= character && character <= 'Z') ||
            ('a' <= character && character <= 'z')) {
            result.add_token(new Variable(character));

            continue;
        }
    }

    return result;
}

std::ostream &operator<<(std::ostream &os, Token const &token) {
    os << static_cast<std::string>(token);

    return os;
}