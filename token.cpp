#include "token.h"

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

std::variant<mlp::Operation, mlp::OwnedToken>
get_next_token(std::string const &expression, int &i) {
    if (i >= expression.size())
        return nullptr;

    char character = expression[i];

    if (auto op = mlp::Operation::from_char(character))
        return *op;

    if ('0' <= character && character <= '9') {
        std::string number;

        while (i < expression.size() && '0' <= character && character < '9') {
            number.push_back(character);

            character = expression[++i];
        }

        if (i >= expression.size())
            return std::make_unique<mlp::Constant>(std::stoull(number));

        if (character != '.') {
            --i;

            return std::make_unique<mlp::Constant>(std::stoull(number));
        }

        do {
            number.push_back(character);

            character = expression[++i];
        } while (i < expression.size() && '0' <= character && character <= '9');

        if (i < expression.size())
            --i;

        return std::make_unique<mlp::Constant>(std::stold(number));
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

        auto token = get_next_token(expression, i);

        if (std::holds_alternative<mlp::Operation>(token))
            throw std::runtime_error("Expression is not valid!");

        auto &parameter = std::get<mlp::OwnedToken>(token);

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

gsl::owner<mlp::Constant *> mlp::Constant::clone() const {
    return new Constant(this->value);
}

mlp::Constant::operator std::string() const {
    return std::to_string(this->value);
}

bool mlp::Constant::operator==(Constant const &constant) const {
    return this->value == constant.value;
}

mlp::Variable::Variable(char const var) : var(var) {}

gsl::owner<mlp::Variable *> mlp::Variable::clone() const {
    return new Variable(this->var);
}

mlp::Variable::operator std::string() const { return {this->var}; }

bool mlp::Variable::operator==(Variable const &variable) const {
    return this->var == variable.var;
}

mlp::Operation::Operation(op const operation) : operation(operation) {}

std::optional<mlp::Operation> mlp::Operation::from_char(char const operation) {
    switch (operation) {
    case '+':
        return Operation(add);

    case '-':
        return Operation(sub);

    case '*':
        return Operation(mul);

    case '/':
        return Operation(div);

    case '^':
        return Operation(pow);

    default:
        return {};
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

mlp::Function::Function(Function const &function)
    : function(function.function), parameter(function.parameter->clone()) {}

gsl::owner<mlp::Function *> mlp::Function::clone() const {
    return new Function(this->function, OwnedToken(this->parameter->clone()));
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

gsl::owner<mlp::Term *> mlp::Term::clone() const {
    return new Term(
        this->coefficient, OwnedToken(this->base->clone()),
        OwnedToken(this->power->clone())
    );
}

mlp::Term::operator std::string() const {
    std::stringstream result;

    if (this->coefficient != 1) {
        if (this->coefficient == -1)
            result << "-";

        else
            result << this->coefficient;
    }

    result << '(';

    result << *this->base;

    if (typeid(*this->power) != typeid(Constant) ||
        dynamic_cast<Constant &>(*this->power).value != 1) {
        result << '^';
        result << *this->power;
    }

    result << ')';

    return result.str();
}

gsl::owner<mlp::Terms *> mlp::Terms::clone() const {
    auto *terms = new Terms;
    terms->coefficient = this->coefficient;

    for (auto const &term : this->terms)
        terms->add_term(OwnedToken(term->clone()));

    return terms;
}

void mlp::Terms::add_term(OwnedToken &&token) {
    auto const &term_type = typeid(*token);

    if (term_type == typeid(Constant)) {
        this->coefficient *= dynamic_cast<Constant &>(*token).value;

        return;
    }

    if (term_type == typeid(Term)) {
        auto &term = dynamic_cast<Term &>(*token);
        this->coefficient *= term.coefficient;
        term.coefficient = 1;

        this->terms.push_back(std::move(token));

        return;
    }

    if (term_type == typeid(Terms)) {
        auto &terms = dynamic_cast<Terms &>(*token);
        this->coefficient *= terms.coefficient;
        terms.coefficient = 1;

        for (OwnedToken &term : terms.terms)
            this->add_term(std::move(term));

        return;
    }

    this->terms.push_back(std::move(token));
}

mlp::Terms::operator std::string() const {
    std::stringstream result;

    if (this->coefficient != 1) {
        if (this->coefficient == -1)
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

gsl::owner<mlp::Expression *> mlp::Expression::clone() const {
    auto *expression = new Expression();

    for (auto const &[operation, token] : this->tokens)
        expression->add_token(operation, OwnedToken(token->clone()));

    return expression;
}

void mlp::Expression::add_token(Sign const sign, OwnedToken &&token) {
    this->tokens.emplace_back(sign, std::move(token));
}

std::pair<mlp::Sign, mlp::OwnedToken> mlp::Expression::pop_token() {
    auto token = std::move(this->tokens.back());

    this->tokens.pop_back();

    return token;
}

bool mlp::Expression::empty() const { return this->tokens.empty(); }

mlp::Expression::operator std::string() const {
    std::stringstream result;

    if (this->tokens.size() == 1) {
        result << this->tokens[0].first
               << static_cast<std::string>(*this->tokens[0].second);
    } else {
        result << '(';

        for (auto const &[operation, token] : this->tokens)
            result << operation << static_cast<std::string>(*token);

        result << ')';
    }

    return result.str();
}

mlp::OwnedToken mlp::tokenise(std::string expression) {
    Expression result{};

    auto e = std::ranges::remove(expression, ' ');

    expression.erase(e.begin(), e.end());

    auto terms = std::make_unique<Terms>();

    Operation op{Operation::add};

    for (int i = 0; i < expression.size(); ++i) {
        std::int32_t const copy = i;

        auto token = get_next_token(expression, i);

        if (!std::holds_alternative<Operation>(token)) {
            auto &term = std::get<OwnedToken>(token);

            if (!term)
                continue;

            if (typeid(*term) == typeid(Constant))
                terms->terms.push_back(std::move(term));

            else
                terms->add_term(std::move(term));

            continue;
        }

        auto &operation = std::get<Operation>(token);

        auto next = get_next_token(expression, ++i);

        if (std::holds_alternative<Operation>(next))
            throw std::runtime_error("Expression is not valid!");

        auto &next_term = std::get<OwnedToken>(next);

        if (!next_term)
            throw std::runtime_error("Expression is not valid!");

        if (operation.operation == Operation::add ||
            operation.operation == Operation::sub) {
            Sign const sign =
                op.operation == Operation::add ? Sign::pos : Sign::neg;

            if (!terms->terms.empty())
                result.add_token(sign, std::move(terms));

            else if (copy != 0)
                result.add_token(
                    sign, std::make_unique<Constant>(terms->coefficient)
                );

            op = operation;

            terms = std::make_unique<Terms>();

            if (typeid(*next_term) == typeid(Constant))
                terms->terms.push_back(std::move(next_term));

            else
                terms->add_term(std::move(next_term));

            continue;
        }

        if (operation.operation == Operation::mul) {
            if (typeid(*next_term) == typeid(Constant))
                terms->coefficient *=
                    dynamic_cast<Constant &>(*next_term).value;
            else
                terms->add_term(std::move(next_term));

            continue;
        }

        if (operation.operation == Operation::div) {
            if (typeid(*next_term) == typeid(Constant))
                terms->coefficient /=
                    dynamic_cast<Constant &>(*next_term).value;

            else
                terms->add_term(std::make_unique<Term>(
                    1, std::move(next_term), std::make_unique<Constant>(-1)
                ));

            continue;
        }

        std::vector<OwnedToken> powers;

        while (true) {
            if (typeid(*next_term) == typeid(Term)) {
                powers.push_back(std::move(next_term));
            } else {
                powers.push_back(std::make_unique<Term>(
                    1, std::move(next_term), std::make_unique<Constant>(1)
                ));
            }

            next = get_next_token(expression, ++i);

            if (std::holds_alternative<Operation>(next)) {
                if (i == expression.size() - 1)
                    throw std::runtime_error("Expression is not valid!");

                operation = std::get<Operation>(next);

                if (operation.operation == Operation::pow)
                    continue;

                --i;

                break;
            }

            if (!next_term)
                break;
        }

        OwnedToken power = std::make_unique<Constant>(1);

        for (OwnedToken &p : powers | std::views::reverse) {
            power = std::make_unique<Term>(1, std::move(p), std::move(power));
        }

        power = power->simplified();

        terms->terms.back() = std::make_unique<Term>(
            1, std::move(terms->terms.back()), std::move(power)
        );
    }

    if (terms->terms.empty())
        throw std::runtime_error("Expression is not valid!");

    result.add_token(
        op.operation == Operation::add ? Sign::pos : Sign::neg, std::move(terms)
    );

    return result.simplified();
}

namespace mlp {
std::ostream &operator<<(std::ostream &os, Token const &token) {
    os << static_cast<std::string>(token);

    return os;
}

bool operator<(Variable const &lhs, Variable const &rhs) {
    return lhs.var < rhs.var;
}

std::ostream &operator<<(std::ostream &os, Sign const sign) {
    os << to_string(sign);

    return os;
}
} // namespace mlp

std::string to_string(mlp::Sign const sign) {
    return sign == mlp::Sign::pos ? "+" : "-";
}