#include "../include/token.h"

#include "../include/constant.h"
#include "../include/expression.h"
#include "../include/function.h"
#include "../include/term.h"
#include "../include/terms.h"
#include "../include/variable.h"

#include <algorithm>
#include <map>
#include <numbers>
#include <ranges>
#include <regex>
#include <set>
#include <sstream>
#include <utility>

namespace mlp {
struct Operation final {
    enum op { add, sub, mul, div, pow } operation;

    explicit Operation(op operation);

    Operation(Operation const &) = default;

    static std::optional<Operation> from_char(char operation);

    explicit operator std::string() const;

    bool operator==(Operation const &) const = default;
};
} // namespace mlp

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
get_next_token(std::string const &expression, std::size_t &i) {
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
        std::size_t count = 1;
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

    for (std::size_t const offset : {5, 4, 3, 2}) {
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

    if (character == 'e')
        return std::make_unique<mlp::Constant>(std::numbers::e);

    if (('A' <= character && character <= 'Z') ||
        ('a' <= character && character <= 'z'))
        return std::make_unique<mlp::Variable>(character);

    throw std::runtime_error("Expression is not valid!");
}
} // namespace

mlp::Term mlp::Token::operator-() const {
    return {-1, OwnedToken(this->clone()), std::make_unique<Constant>(1)};
}

bool mlp::is_dependent_on(token const &token, Variable const &variable) {
    return std::visit(
        [&variable](auto &&var) -> bool {
            return is_dependent_on(var, variable);
        },
        token
    );
}

bool mlp::is_linear_of(token const &token, Variable const &variable) {
    return std::visit(
        [&variable](auto &&var) -> bool { return is_linear_of(var, variable); },
        token
    );
}

mlp::token mlp::evaluate(
    token const &token, std::map<Variable, SharedToken> const &values
) {
    return std::visit(
        [&values](auto &&val) -> mlp::token { return evaluate(val, values); },
        token
    );
}

mlp::token mlp::simplified(Token const &token) {
    return simplified(to_variant(token));
}

mlp::token mlp::simplified(token const &token) {
    return std::visit(
        [](auto &&var) -> mlp::token { return simplified(var); }, token
    );
}

mlp::token mlp::derivative(
    token const &token, Variable const &variable, std::uint32_t const order,
    std::map<Variable, SharedToken> const &values
) {
    return evaluate(derivative(token, variable, order), values);
}

mlp::token mlp::derivative(
    token const &token, Variable const &variable, std::uint32_t const order
) {
    return std::visit(
        [&variable, &order](auto &&val) -> mlp::token {
            return derivative(val, variable, order);
        },
        token
    );
}

mlp::token mlp::integral(Token const &token, Variable const &variable) {
    return integral(to_variant(token), variable);
}

mlp::token mlp::integral(token const &token, Variable const &variable) {
    return std::visit(
        [&variable](auto &&val) -> mlp::token {
            return integral(val, variable);
        },
        token
    );
}

mlp::token mlp::integral(
    Token const &token, Variable const &variable, SharedToken const &from,
    SharedToken const &to
) {
    return integral(to_variant(token), variable, from, to);
}

mlp::token mlp::integral(
    token const &token, Variable const &variable, SharedToken const &from,
    SharedToken const &to
) {
    auto const &integral = mlp::integral(token, variable);

    return simplified(
        from_variant(evaluate(integral, {{variable, to}})) -
        from_variant(evaluate(integral, {{variable, from}}))
    );
}

mlp::OwnedToken mlp::tokenise(std::string expression) {
    Expression result{};

    auto e = std::ranges::remove(expression, ' ');

    expression.erase(e.begin(), e.end());

    expression = std::regex_replace(
        expression, std::regex("π"), std::to_string(std::numbers::pi)
    );

    auto s = Sign::pos;
    std::vector<OwnedToken> numerator;
    std::vector<OwnedToken> denominator;
    bool last = false;

    for (std::size_t i = 0; i < expression.size(); ++i) {
        std::size_t const copy = i;

        auto token = get_next_token(expression, i);

        if (!std::holds_alternative<Operation>(token)) {
            auto &term = std::get<OwnedToken>(token);

            if (!term)
                continue;

            numerator.push_back(std::move(term));

            continue;
        }

        auto &operation = std::get<Operation>(token);

        if (copy == 0) {
            if (operation.operation == Operation::mul ||
                operation.operation == Operation::div ||
                operation.operation == Operation::pow)
                throw std::runtime_error("Expression is not valid!");

            if (operation.operation == Operation::sub)
                s = Sign::neg;

            continue;
        }

        auto next = get_next_token(expression, ++i);

        if (std::holds_alternative<Operation>(next))
            throw std::runtime_error("Expression is not valid!");

        auto &next_term = std::get<OwnedToken>(next);

        if (!next_term)
            throw std::runtime_error("Expression is not valid!");

        if (operation.operation == Operation::add ||
            operation.operation == Operation::sub) {
            Sign const sign =
                operation.operation == Operation::add ? Sign::pos : Sign::neg;

            if (!numerator.empty()) {
                Terms terms{};

                for (OwnedToken &t : numerator)
                    terms *= std::move(t);

                for (OwnedToken &t : denominator)
                    terms /= std::move(t);

                result.add_token(s, from_variant(simplified(terms)));

                numerator.clear();
                denominator.clear();
            }

            else if (sign == s)
                result += Constant(1);

            else
                result -= Constant(1);

            s = sign;
            numerator.push_back(std::move(next_term));

            continue;
        }

        if (operation.operation == Operation::mul) {
            numerator.push_back(std::move(next_term));
            last = false;

            continue;
        }

        if (operation.operation == Operation::div) {
            denominator.push_back(std::move(next_term));
            last = true;

            continue;
        }

        std::vector<OwnedToken> powers;

        while (true) {
            powers.push_back(std::move(next_term));

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

        for (OwnedToken &p : powers | std::views::reverse)
            power = std::make_unique<Term>(std::move(*p) ^ std::move(*power));

        power = OwnedToken(from_variant(simplified(*power)).move());

        if (!last)
            numerator.back() = std::make_unique<Term>(
                std::move(*numerator.back()) ^ std::move(*power)
            );
        else
            denominator.back() = std::make_unique<Term>(
                std::move(*denominator.back()) ^ std::move(*power)
            );
    }

    Terms terms{};

    for (OwnedToken &t : numerator)
        terms *= std::move(t);

    for (OwnedToken &t : denominator)
        terms /= std::move(t);

    result.add_token(s, from_variant(simplified(terms)));

    return OwnedToken(from_variant(simplified(result)).move());
}

namespace mlp {
Term operator*(std::double_t const lhs, Token const &rhs) {
    return lhs * (rhs ^ 1);
}

Term operator*(Token const &lhs, std::double_t const rhs) { return rhs * lhs; }

Term operator*(std::double_t const lhs, Token &&rhs) {
    return lhs * (std::move(rhs) ^ 1);
}

Term operator*(Token &&lhs, std::double_t const rhs) {
    return rhs * std::move(lhs);
}

Expression operator+(Token const &lhs, Token const &rhs) {
    Expression result;
    result += lhs;
    result += rhs;

    return result;
}

Expression operator-(Token const &lhs, Token const &rhs) {
    Expression result;
    result += lhs;
    result -= rhs;

    return result;
}

Expression operator*(std::double_t const lhs, Expression rhs) {
    return std::move(rhs *= Constant(lhs));
}

Terms operator*(Token const &lhs, Token const &rhs) {
    Terms result;
    result *= lhs;
    result *= rhs;

    return result;
}

Expression operator*(Token const &lhs, Expression rhs) {
    return std::move(rhs *= lhs);
}

Terms operator/(Token const &lhs, Token const &rhs) {
    Terms result;
    result *= lhs;
    result /= rhs;

    return result;
}

std::ostream &operator<<(std::ostream &os, Sign const sign) {
    os << to_string(sign);

    return os;
}

token to_variant(Token const &token) {
    if (typeid(token) == typeid(Constant))
        return dynamic_cast<Constant const &>(token);

    if (typeid(token) == typeid(Variable))
        return dynamic_cast<Variable const &>(token);

    if (typeid(token) == typeid(Function))
        return dynamic_cast<Function const &>(token);

    if (typeid(token) == typeid(Term))
        return dynamic_cast<Term const &>(token);

    if (typeid(token) == typeid(Terms))
        return dynamic_cast<Terms const &>(token);

    if (typeid(token) == typeid(Expression))
        return dynamic_cast<Expression const &>(token);

    throw std::runtime_error("Invalid token");
}

Token const &from_variant(token const &token) {
    return std::visit(
        [](auto const &var) -> Token const & { return var; }, token
    );
}

Token &&from_variant(token &&token) {
    return std::visit(
        [](auto &&var) -> Token && { return std::move(var); }, token
    );
}

Term operator^(Token const &lhs, std::double_t const rhs) {
    return {1, OwnedToken(lhs.clone()), std::make_unique<Constant>(rhs)};
}

Term operator^(Token const &lhs, Token const &rhs) {
    return {1, OwnedToken(lhs.clone()), OwnedToken(rhs.clone())};
}

Term operator^(Token const &lhs, Token &&rhs) {
    return {1, OwnedToken(lhs.clone()), OwnedToken(std::move(rhs).move())};
}

Term operator^(Token &&lhs, std::double_t const rhs) {
    return {
        1, OwnedToken(std::move(lhs).move()), std::make_unique<Constant>(rhs)
    };
}

Term operator^(Token &&lhs, Token const &rhs) {
    return {1, OwnedToken(std::move(lhs).move()), OwnedToken(rhs.clone())};
}

Term operator^(Token &&lhs, Token &&rhs) {
    return {
        1, OwnedToken(std::move(lhs).move()), OwnedToken(std::move(rhs).move())
    };
}
} // namespace mlp

std::string to_string(mlp::Sign const sign) {
    return sign == mlp::Sign::pos ? "+" : "-";
}