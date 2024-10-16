#include "token.h"
#include "simplify.h"

#include <algorithm>
#include <map>
#include <ranges>
#include <set>
#include <sstream>
#include <utility>

namespace {
std::map<char, char> const k_parenthesis_map{
    {'(', ')'}, {'[', ']'}, {'{', '}'}
};

std::set<std::string> k_functions{
    "sin",   "cos",   "tan",   "sec",  "csc",   "cot",   "sinh",
    "cosh",  "tanh",  "sech",  "csch", "coth",  "asin",  "acos",
    "atan",  "asec",  "acsc",  "acot", "asinh", "acosh", "atanh",
    "asech", "acsch", "acoth", "ln",
};

mlp::OwnedToken get_next_token(std::string const &expression, int &i) {
    if (i >= expression.size())
        return nullptr;

    char character = expression[i];

    if (auto op = mlp::Operation::from_char(character))
        return op;

    if ('0' <= character && character <= '9') {
        std::string number;

        while (i < expression.size() && '0' <= character && character < '9') {
            number.push_back(character);

            character = expression[++i];
        }

        if (i < expression.size()) {
            if (character != '.') {
                --i;

                return std::make_unique<mlp::Constant>(std::stoull(number));
            }

            do {
                number.push_back(character);

                character = expression[++i];
            } while (i < expression.size() && '0' <= character &&
                     character <= '9');

            if (i < expression.size())
                --i;

            return std::make_unique<mlp::Constant>(std::stold(number));
        }

        return std::make_unique<mlp::Constant>(std::stoull(number));
    }

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

        return mlp::tokenise(expression.substr(start, end));
    }

    for (int const offset : {5, 4, 3, 2}) {
        if (i + offset >= expression.size())
            continue;

        std::string const &fn = expression.substr(i, offset);

        if (!k_functions.contains(fn))
            continue;

        i += offset;

        mlp::OwnedToken parameter = get_next_token(expression, i);

        if (!parameter)
            throw std::runtime_error("Expression is not valid!");

        return std::make_unique<mlp::Function>(fn, std::move(parameter));
    }

    if (('A' <= character && character <= 'Z') ||
        ('a' <= character && character <= 'z'))
        return std::make_unique<mlp::Variable>(character);

    throw std::runtime_error("Expression is not valid!");
}
} // namespace

mlp::Constant::Constant(double const value) : value(value) {}

mlp::OwnedToken mlp::Constant::clone() const {
    return std::make_unique<Constant>(this->value);
}

mlp::Constant::operator std::string() const {
    return std::to_string(this->value);
}

bool mlp::Constant::operator==(Constant const &constant) const {
    return this->value == constant.value;
}

mlp::Variable::Variable(char const var) : var(var) {}

mlp::OwnedToken mlp::Variable::clone() const {
    return std::make_unique<Variable>(this->var);
}

mlp::Variable::operator std::string() const { return {this->var}; }

bool mlp::Variable::operator==(Variable const &variable) const {
    return this->var == variable.var;
}

mlp::Operation::Operation(op const operation) : operation(operation) {}

mlp::OwnedToken mlp::Operation::clone() const {
    return std::make_unique<Operation>(this->operation);
}

std::unique_ptr<mlp::Operation>
mlp::Operation::from_char(char const operation) {
    switch (operation) {
    case '+':
        return std::make_unique<Operation>(add);

    case '-':
        return std::make_unique<Operation>(sub);

    case '*':
        return std::make_unique<Operation>(mul);

    case '/':
        return std::make_unique<Operation>(div);

    case '^':
        return std::make_unique<Operation>(pow);

    default:
        return nullptr;
    }
}

mlp::Operation::operator std::string() const {
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

mlp::Function::Function(std::string function, OwnedToken &&parameter)
    : function(std::move(function)), parameter(std::move(parameter)) {}

mlp::OwnedToken mlp::Function::clone() const {
    return std::make_unique<Function>(this->function, this->parameter->clone());
}

mlp::Function::operator std::string() const {
    std::stringstream result;

    result << this->function << '('
           << static_cast<std::string>(*this->parameter) << ')';

    return result.str();
}

mlp::Term::Term(double const coefficient, OwnedToken &&base, OwnedToken &&power)
    : coefficient(coefficient), base(std::move(base)), power(std::move(power)) {
}

mlp::Term::Term(OwnedToken &&base, OwnedToken &&power)
    : base(std::move(base)), power(std::move(power)) {}

mlp::OwnedToken mlp::Term::clone() const {
    return std::make_unique<Term>(
        this->coefficient.value, this->base->clone(), this->power->clone()
    );
}

mlp::Term::operator std::string() const {
    std::stringstream result;

    if (this->coefficient.value != 1) {
        if (this->coefficient.value == -1)
            result << "-";

        else
            result << this->coefficient;
    }

    result << '(';

    result << *this->base;

    if (this->power) {
        if (typeid(*this->power) == typeid(Constant)) {
            if (dynamic_cast<Constant &>(*this->power).value != 1) {
                result << '^';
                result << *this->power;
            }
        } else {
            result << '^';
            result << *this->power;
        }
    }

    result << ')';

    return result.str();
}

mlp::OwnedToken mlp::Terms::clone() const {
    auto terms = std::make_unique<Terms>();
    terms->coefficient = this->coefficient;

    for (auto const &term : this->terms)
        terms->add_term(term->clone());

    return terms;
}

void mlp::Terms::add_term(OwnedToken &&token) {
    auto const &term_type = typeid(*token);

    if (term_type == typeid(Constant)) {
        this->coefficient.value *= dynamic_cast<Constant &>(*token).value;

        return;
    }

    if (term_type == typeid(Term)) {
        auto &term = dynamic_cast<Term &>(*token);
        this->coefficient.value *= term.coefficient.value;
        term.coefficient.value = 1;

        this->terms.push_back(std::move(token));

        return;
    }

    if (term_type == typeid(Terms)) {
        auto &terms = dynamic_cast<Terms &>(*token);
        this->coefficient.value *= terms.coefficient.value;
        terms.coefficient.value = 1;

        for (OwnedToken &term : terms.terms)
            this->add_term(std::move(term));

        return;
    }

    this->terms.push_back(std::move(token));
}

mlp::Terms::operator std::string() const {
    std::stringstream result;

    if (this->coefficient.value != 1) {
        if (this->coefficient.value == -1)
            result << "-";

        else
            result << this->coefficient;
    }

    result << '(';

    for (int i = 0; i < this->terms.size(); ++i) {
        result << static_cast<std::string>(*this->terms[i]);

        if (i != this->terms.size() - 1)
            result << '*';
    }

    result << ')';

    return result.str();
}

mlp::OwnedToken mlp::Expression::clone() const {
    auto expression = std::make_unique<Expression>();

    for (auto const &token : this->tokens)
        expression->add_token(token->clone());

    return expression;
}

void mlp::Expression::add_token(OwnedToken &&token) {
    this->tokens.push_back(std::move(token));
}

mlp::OwnedToken mlp::Expression::pop_token() {
    OwnedToken token = std::move(this->tokens.back());

    this->tokens.pop_back();

    return token;
}

mlp::Expression::operator std::string() const {
    std::stringstream result;

    if (this->tokens.size() == 1) {
        result << static_cast<std::string>(*this->tokens[0]);
    } else {
        result << '(';

        for (OwnedToken const &token : this->tokens)
            result << static_cast<std::string>(*token);

        result << ')';
    }

    return result.str();
}

mlp::OwnedToken mlp::tokenise(std::string expression) {
    Expression result{};

    auto e = std::ranges::remove(expression, ' ');

    expression.erase(e.begin(), e.end());

    auto terms = std::make_unique<Terms>();

    for (int i = 0; i < expression.size(); ++i) {
        std::int32_t const copy = i;

        OwnedToken token = get_next_token(expression, i);

        if (!token)
            continue;

        if (typeid(*token) == typeid(Operation)) {
            auto &operation = dynamic_cast<Operation &>(*token);

            if (operation.operation == Operation::add ||
                operation.operation == Operation::sub) {
                if (!terms->terms.empty())
                    result.add_token(std::move(terms));

                else if (copy != 0)
                    result.add_token(
                        std::make_unique<Constant>(terms->coefficient)
                    );

                terms = std::make_unique<Terms>();
                result.add_token(std::move(token));

                continue;
            }

            OwnedToken next = get_next_token(expression, ++i);

            if (!next)
                throw std::runtime_error("Expression is not valid!");

            if (operation.operation == Operation::mul) {
                if (typeid(*next) == typeid(Constant)) {
                    terms->coefficient.value *=
                        dynamic_cast<Constant &>(*next).value;
                } else {
                    terms->add_term(std::move(next));
                }

                continue;
            }

            if (operation.operation == Operation::div) {
                if (typeid(*next) == typeid(Constant)) {
                    terms->coefficient.value /=
                        dynamic_cast<Constant &>(*next).value;
                } else {
                    terms->add_term(
                        std::make_unique<Term>(
                            1, std::move(next), std::make_unique<Constant>(-1)
                        )
                    );
                }

                continue;
            }

            std::vector<OwnedToken> powers;

            while (true) {
                if (typeid(*next) == typeid(Term)) {
                    powers.push_back(std::move(next));
                } else {
                    powers.push_back(
                        std::make_unique<Term>(
                            1, std::move(next), std::make_unique<Constant>(1)
                        )
                    );
                }

                next = get_next_token(expression, ++i);

                if (!next)
                    break;

                if (typeid(*next) == typeid(Operation)) {
                    if (i == expression.size() - 1)
                        throw std::runtime_error("Expression is not valid!");

                    operation = dynamic_cast<Operation &>(*next);

                    if (operation.operation == Operation::pow)
                        continue;

                    break;
                }
            }

            OwnedToken power = std::make_unique<Constant>(1);

            for (OwnedToken &p : powers | std::views::reverse) {
                power =
                    std::make_unique<Term>(1, std::move(p), std::move(power));
            }

            terms->terms.back() = std::make_unique<Term>(
                1, std::move(terms->terms.back()), std::move(power)
            );

            if (next) {
                --i;
            }

            continue;
        }

        if (typeid(*token) == typeid(Constant)) {
            terms->terms.push_back(std::move(token));
        } else {
            terms->add_term(std::move(token));
        }
    }

    if (terms->terms.empty())
        throw std::runtime_error("Expression is not valid!");

    result.add_token(std::move(terms));

    return simplified(result);
}

namespace mlp {
std::ostream &operator<<(std::ostream &os, Token const &token) {
    os << static_cast<std::string>(token);

    return os;
}

bool operator<(Variable const &lhs, Variable const &rhs) {
    return lhs.var < rhs.var;
}
} // namespace mlp