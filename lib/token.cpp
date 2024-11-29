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

std::variant<mlp::Operation, std::optional<mlp::Token>>
get_next_token(std::string const &expression, std::size_t &i) {
    if (i >= expression.size())
        return std::optional<mlp::Token>{};

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
            return mlp::Constant(std::stod(number));

        if (character != '.') {
            --i;

            return mlp::Constant(std::stod(number));
        }

        do {
            number.push_back(character);

            character = expression[++i];
        } while (i < expression.size() && '0' <= character && character <= '9');

        if (i < expression.size())
            --i;

        return mlp::Constant(std::stod(number));
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

        return std::move(mlp::tokenise(expression.substr(start, end)));
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

        auto &parameter = std::get<std::optional<mlp::Token>>(token);

        if (!parameter)
            throw std::runtime_error("Expression is not valid!");

        return mlp::Function(
            fn, std::make_unique<mlp::Token>(std::move(*parameter))
        );
    }

    if (character == 'e')
        return mlp::Constant(std::numbers::e);

    if (('A' <= character && character <= 'Z') ||
        ('a' <= character && character <= 'z'))
        return mlp::Variable(character);

    throw std::runtime_error("Expression is not valid!");
}
} // namespace

bool mlp::is_dependent_on(Token const &token, Variable const &variable) {
    return std::visit(
        [&variable](auto &&var) -> bool {
            return is_dependent_on(var, variable);
        },
        token
    );
}

bool mlp::is_linear_of(Token const &token, Variable const &variable) {
    return std::visit(
        [&variable](auto &&var) -> bool { return is_linear_of(var, variable); },
        token
    );
}

mlp::Token
mlp::evaluate(Token const &token, std::map<Variable, Token> const &values) {
    return std::visit(
        [&values](auto &&val) -> Token { return evaluate(val, values); }, token
    );
}

mlp::Token mlp::simplified(Token const &token) {
    return std::visit(
        [](auto &&var) -> Token { return simplified(var); }, token
    );
}

mlp::Token mlp::derivative(
    Token const &token, Variable const &variable, std::uint32_t const order,
    std::map<Variable, Token> const &values
) {
    return evaluate(derivative(token, variable, order), values);
}

mlp::Token mlp::derivative(
    Token const &token, Variable const &variable, std::uint32_t const order
) {
    return std::visit(
        [&variable, &order](auto &&val) -> Token {
            return derivative(val, variable, order);
        },
        token
    );
}

mlp::Token mlp::integral(Token const &token, Variable const &variable) {
    return std::visit(
        [&variable](auto &&val) -> Token { return integral(val, variable); },
        token
    );
}

mlp::Token mlp::integral(
    Token const &token, Variable const &variable, Token const &from,
    Token const &to
) {
    auto const &integral = mlp::integral(token, variable);

    return simplified(
        evaluate(integral, {{variable, to}}) -
        evaluate(integral, {{variable, from}})
    );
}

mlp::Token mlp::tokenise(std::string expression) {
    Expression result{};

    auto e = std::ranges::remove(expression, ' ');

    expression.erase(e.begin(), e.end());

    expression = std::regex_replace(
        expression, std::regex("π"), std::to_string(std::numbers::pi)
    );

    auto s = Sign::pos;
    std::vector<Token> numerator;
    std::vector<Token> denominator;
    bool last = false;

    for (std::size_t i = 0; i < expression.size(); ++i) {
        std::size_t const copy = i;

        std::variant<Operation, std::optional<Token>> token =
            get_next_token(expression, i);

        if (!std::holds_alternative<Operation>(token)) {
            auto &term = std::get<std::optional<Token>>(token);

            if (!term)
                continue;

            numerator.push_back(std::move(*term));

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

        std::variant<Operation, std::optional<Token>> next =
            get_next_token(expression, ++i);

        if (std::holds_alternative<Operation>(next))
            throw std::runtime_error("Expression is not valid!");

        auto &next_term = std::get<std::optional<Token>>(next);

        if (!next_term)
            throw std::runtime_error("Expression is not valid!");

        if (operation.operation == Operation::add ||
            operation.operation == Operation::sub) {
            Sign const sign =
                operation.operation == Operation::add ? Sign::pos : Sign::neg;

            if (!numerator.empty()) {
                Terms terms{};

                for (Token &t : numerator)
                    terms *= std::move(t);

                for (Token &t : denominator)
                    terms /= std::move(t);

                result.add_token(s, simplified(terms));

                numerator.clear();
                denominator.clear();
            }

            else if (sign == s)
                result += Constant(1);

            else
                result -= Constant(1);

            s = sign;
            numerator.push_back(std::move(*next_term));

            continue;
        }

        if (operation.operation == Operation::mul) {
            numerator.push_back(std::move(*next_term));
            last = false;

            continue;
        }

        if (operation.operation == Operation::div) {
            denominator.push_back(std::move(*next_term));
            last = true;

            continue;
        }

        std::vector<Token> powers;

        while (true) {
            powers.push_back(std::move(*next_term));

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

        Token power = Constant(1);

        for (Token &p : powers | std::views::reverse)
            power = p ^ power;

        power = std::move(simplified(power));

        if (!last)
            numerator.back() = numerator.back() ^ power;
        else
            denominator.back() = denominator.back() ^ power;
    }

    Terms terms{};

    for (Token &t : numerator)
        terms *= std::move(t);

    for (Token &t : denominator)
        terms /= std::move(t);

    result.add_token(s, simplified(terms));

    return simplified(result);
}

namespace mlp {
Term operator-(Token const &token) {
    return {
        -1, std::make_unique<Token>(token), std::make_unique<Token>(Constant(1))
    };
}

Term operator*(std::double_t const lhs, Token const &rhs) {
    return lhs * (rhs ^ 1);
}

Term operator*(Token const &lhs, std::double_t const rhs) { return rhs * lhs; }

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

Term operator^(Token const &lhs, std::double_t const rhs) {
    return {
        1, std::make_unique<Token>(lhs), std::make_unique<Token>(Constant(rhs))
    };
}

Term operator^(Token const &lhs, Token const &rhs) {
    return {1, std::make_unique<Token>(lhs), std::make_unique<Token>(rhs)};
}
} // namespace mlp

std::string to_string(mlp::Token const &token) {
    return std::visit(
        [](auto &&var) -> std::string { return static_cast<std::string>(var); },
        token
    );
}

std::string to_string(mlp::Sign const sign) {
    return sign == mlp::Sign::pos ? "+" : "-";
}

std::ostream &mlp::operator<<(std::ostream &os, Token const &token) {
    os << to_string(token);

    return os;
}