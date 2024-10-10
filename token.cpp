#include "token.h"
#include "simplify.h"

#include <algorithm>
#include <map>
#include <ranges>
#include <set>
#include <sstream>

namespace {
std::map<char, char> const k_parenthesis_map{
    {'(', ')'}, {'[', ']'}, {'{', '}'}};

std::set<std::string> k_functions{
    "sin",  "cos",  "tan",  "sec",  "csc",  "cot", "sinh",
    "cosh", "tanh", "sech", "csch", "coth", "ln",
};

std::shared_ptr<Token> get_next_token(std::string const &expression, int &i) {
    if (i >= expression.size())
        return nullptr;

    char character = expression[i];

    if (k_parenthesis_map.contains(character)) {
        int count = 1;
        char const original = character;

        character = expression[++i];

        std::size_t const start = i;

        while (i < expression.size() && count) {
            if (character == original)
                ++count;

            else if (character == k_parenthesis_map.at(original))
                --count;

            character = expression[++i];
        }

        --i;

        if (count)
            throw std::invalid_argument("Expression is not valid!");

        std::size_t const end = i - start;

        return tokenise(expression.substr(start, end));
    }

    for (int const offset : {4, 3, 2}) {
        if (i + offset >= expression.size())
            continue;

        std::string const &fn = expression.substr(i, offset);

        if (!k_functions.contains(fn))
            continue;

        i += offset;

        return std::make_shared<Function>(fn, get_next_token(expression, i));
    }

    if (character == 'e' && i < expression.size() - 1 &&
        expression[i + 1] == '^') {
        i += 2;

        return std::make_shared<Function>("e^", get_next_token(expression, i));
    }

    if (auto op = Operation::from_char(character))
        return op;

    if ('0' <= character && character <= '9') {
        std::string number;

        while (i < expression.size() && '0' < character && character < '9') {
            number.push_back(character);

            character = expression[++i];
        }

        if (i < expression.size()) {
            if (character != '.') {
                --i;

                return std::make_shared<Constant>(std::stoull(number));
            }

            do {
                number.push_back(character);

                character = expression[++i];
            } while (i < expression.size() && '0' <= character &&
                     character <= '9');

            if (i < expression.size())
                --i;

            return std::make_shared<Constant>(std::stold(number));
        }

        return std::make_shared<Constant>(std::stoull(number));
    }

    if (('A' <= character && character <= 'Z') ||
        ('a' <= character && character <= 'z'))
        return std::make_shared<Variable>(character);

    return nullptr;
}
} // namespace

Constant::Constant(long double const value) : value(value) {}

Constant::operator std::string() const { return std::to_string(this->value); }

bool Constant::operator==(Constant const &constant) const {
    return this->value == constant.value;
}

Variable::Variable(char const var) : var(var) {}

Variable::operator std::string() const { return {this->var}; }

bool Variable::operator==(Variable const &variable) const {
    return this->var == variable.var;
}

Operation::Operation(op const operation) : operation(operation) {}

std::shared_ptr<Operation> Operation::from_char(char const operation) {
    switch (operation) {
    case '+':
        return std::make_shared<Operation>(add);

    case '-':
        return std::make_shared<Operation>(sub);

    case '*':
        return std::make_shared<Operation>(mul);

    case '/':
        return std::make_shared<Operation>(div);

    case '^':
        return std::make_shared<Operation>(pow);

    default:
        return nullptr;
    }
}

Operation::operator std::string() const {
    switch (this->operation) {
    case add:
        return "+";
    case sub:
        return "-";
    case mul:
        return "*";
    case div:
        return "/";
    case pow:
        return "^";
    }

    return "";
}

Function::Function(std::string function,
                   std::shared_ptr<Token> const &parameter)
    : function(std::move(function)), parameter(parameter) {}

Function::operator std::string() const {
    std::stringstream result;

    result << this->function << '('
           << static_cast<std::string>(*this->parameter) << ')';

    return result.str();
}

Term::Term(long double const coefficient, std::shared_ptr<Token> const &base,
           std::shared_ptr<Token> const &power)
    : coefficient(coefficient), base(base), power(power) {}

Term::Term(std::shared_ptr<Token> const &base,
           std::shared_ptr<Token> const &power)
    : base(base), power(power) {}

Term::operator std::string() const {
    std::stringstream result;

    if (this->coefficient.value != 1) {
        if (this->coefficient.value == -1)
            result << "-";

        else
            result << this->coefficient;
    }

    result << *this->base;

    if (this->power) {
        if (typeid(*this->power) == typeid(Constant)) {
            if (std::dynamic_pointer_cast<Constant>(this->power)->value != 1) {
                result << '^';
                result << *this->power;
            }
        } else {
            result << '^';
            result << *this->power;
        }
    }

    return result.str();
}

Terms::operator std::string() const {
    std::stringstream result;

    if (this->coefficient.value != 1) {
        if (this->coefficient.value == -1)
            result << "-";

        else
            result << this->coefficient;
    }

    result << '(';

    for (auto const term :
         this->terms |
             std::views::transform([](std::shared_ptr<Token> const &token)
                                       -> std::vector<std::string> {
                 return {static_cast<std::string>(*token)};
             }) |
             std::views::join_with("*"))
        result << term;

    result << ')';

    return result.str();
}

void Terms::add_term(std::shared_ptr<Token> const &term) {
    auto const &term_type = typeid(*term);

    if (term_type == typeid(Constant)) {
        this->coefficient.value *=
            std::dynamic_pointer_cast<Constant>(term)->value;

        return;
    }

    if (term_type == typeid(Term)) {
        auto const t = std::dynamic_pointer_cast<Term>(term);
        this->coefficient.value *= t->coefficient.value;
        t->coefficient.value = 1;

        this->terms.push_back(t);

        return;
    }

    this->terms.push_back(term);
}

Expression::operator std::string() const {
    std::stringstream result;

    if (this->tokens.size() == 1) {
        result << static_cast<std::string>(*this->tokens[0]);
    } else {
        result << '(';

        for (std::shared_ptr<Token> const &token : this->tokens)
            result << static_cast<std::string>(*token);

        result << ')';
    }

    return result.str();
}

void Expression::add_token(std::shared_ptr<Token> const &token) {
    this->tokens.push_back(token);
}

std::shared_ptr<Token> Expression::pop_token() {
    std::shared_ptr<Token> token = this->tokens.back();

    this->tokens.pop_back();

    return token;
}

std::shared_ptr<Token> tokenise(std::string expression) {
    auto const result = std::make_shared<Expression>();

    auto e = std::ranges::remove(expression, ' ');

    expression.erase(e.begin(), e.end());

    auto terms = std::make_shared<Terms>();

    for (int i = 0; i < expression.size(); ++i) {
        std::shared_ptr<Token> token = get_next_token(expression, i);

        if (!token)
            continue;

        if (typeid(*token) == typeid(Operation)) {
            auto operation = std::dynamic_pointer_cast<Operation>(token);

            if (operation->operation == Operation::add ||
                operation->operation == Operation::sub) {
                result->add_token(terms);
                terms = std::make_shared<Terms>();
                result->add_token(operation);

                continue;
            }

            if (operation->operation == Operation::mul) {
                if (auto next = get_next_token(expression, ++i);
                    typeid(*next) == typeid(Constant)) {
                    terms->coefficient.value *=
                        std::dynamic_pointer_cast<Constant>(next)->value;
                } else {
                    terms->terms.push_back(next);
                }

                continue;
            }

            if (operation->operation == Operation::div) {
                if (auto next = get_next_token(expression, ++i);
                    typeid(*next) == typeid(Constant)) {
                    terms->coefficient.value /=
                        std::dynamic_pointer_cast<Constant>(next)->value;
                } else {
                    terms->terms.emplace_back(std::make_shared<Term>(
                        1, next, std::make_shared<Constant>(-1)));
                }

                continue;
            }

            std::vector<std::shared_ptr<Term>> powers;

            std::shared_ptr<Token> next;

            while (operation->operation == Operation::pow) {
                next = get_next_token(expression, ++i);

                if (!next)
                    break;

                if (typeid(*next) == typeid(Operation)) {
                    operation = std::dynamic_pointer_cast<Operation>(next);

                    continue;
                }

                if (typeid(*next) == typeid(Term)) {
                    powers.push_back(std::dynamic_pointer_cast<Term>(next));
                } else {
                    powers.push_back(std::make_shared<Term>(
                        1, next, std::make_shared<Constant>(1)));
                }
            }

            auto power = std::make_shared<Term>(
                1, std::make_shared<Constant>(1), nullptr);

            for (std::shared_ptr<Term> const &p :
                 powers | std::views::reverse) {
                power = std::make_shared<Term>(1, p, power);
            }

            terms->terms.back() =
                std::make_shared<Term>(1, terms->terms.back(), power);

            if (next) {
                --i;
            }

            continue;
        }

        if (typeid(*token) == typeid(Term)) {
            auto term = std::dynamic_pointer_cast<Term>(token);
            terms->coefficient.value *= term->coefficient.value;
            term->coefficient.value = 1;

            terms->terms.push_back(term);
        } else {
            terms->terms.push_back(token);
        }
    }

    if (!terms->terms.empty())
        result->add_token(terms);

    return simplify(result);
}

std::ostream &operator<<(std::ostream &os, Token const &token) {
    os << static_cast<std::string>(token);

    return os;
}