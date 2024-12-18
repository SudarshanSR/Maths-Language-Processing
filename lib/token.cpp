#include "../include/token.h"

#include "../include/expression.h"
#include "../include/function.h"
#include "../include/term.h"
#include "../include/terms.h"
#include "../include/variable.h"

#include <algorithm>
#include <istream>
#include <map>
#include <numbers>
#include <ranges>
#include <regex>
#include <set>
#include <utility>

namespace {
enum class Operation { add, sub, mul, div, pow };

std::map<char, Operation> const k_op_map{
    {'+', Operation::add}, {'-', Operation::sub}, {'*', Operation::mul},
    {'/', Operation::div}, {'^', Operation::pow},
};

std::map<char, char> const k_parenthesis_map{
    {'(', ')'}, {'[', ']'}, {'{', '}'}
};

std::variant<Operation, std::optional<mlp::Token>>
get_next_token(std::string const &expression, std::size_t &i) {
    if (i >= expression.size())
        return std::optional<mlp::Token>{};

    char character = expression[i];

    if (k_op_map.contains(character))
        return k_op_map.at(character);

    if (isdigit(character)) {
        std::string number;

        while (i < expression.size() && isdigit(character)) {
            number.push_back(character);

            character = expression[++i];
        }

        if (i >= expression.size())
            return std::stod(number);

        if (character != '.') {
            --i;

            return std::stod(number);
        }

        do {
            number.push_back(character);

            character = expression[++i];
        } while (i < expression.size() && isdigit(character));

        if (i < expression.size())
            --i;

        return std::stod(number);
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

    if (isalpha(character)) {
        std::string func;

        std::size_t const copy = i;

        while (i < expression.size() && isalpha(expression[i]))
            func += expression[i++];

        if (mlp::Function::is_defined(func)) {
            auto token = get_next_token(expression, i);

            if (std::holds_alternative<Operation>(token))
                throw std::runtime_error("Expression is not valid!");

            auto &parameter = std::get<std::optional<mlp::Token>>(token);

            if (!parameter)
                throw std::runtime_error("Expression is not valid!");

            return mlp::Function(func, {std::move(*parameter)});
        }

        i = copy;

        if (character == 'e')
            return std::numbers::e;

        return mlp::Variable(character);
    }

    throw std::runtime_error("Expression is not valid!");
}
} // namespace

bool mlp::is_dependent_on(Token const &token, Variable variable) {
    return std::visit(
        [&variable](auto &&var) -> bool {
            return is_dependent_on(var, variable);
        },
        token
    );
}

bool mlp::is_linear_of(Token const &token, Variable variable) {
    return std::visit(
        [&variable](auto &&var) -> bool { return is_linear_of(var, variable); },
        token
    );
}

mlp::Token
mlp::evaluate(Token const &token, std::map<Variable, Token> const &values) {
    return std::visit(
        [&values](auto &&var) -> Token { return evaluate(var, values); }, token
    );
}

mlp::Token mlp::simplified(Token const &token) {
    return std::visit(
        [](auto &&var) -> Token { return simplified(var); }, token
    );
}

mlp::Token mlp::derivative(
    Token const &token, Variable const variable, std::uint32_t const order,
    std::map<Variable, Token> const &values
) {
    return evaluate(derivative(token, variable, order), values);
}

mlp::Token mlp::derivative(
    Token const &token, Variable variable, std::uint32_t const order
) {
    return std::visit(
        [&variable, &order](auto &&var) -> Token {
            return derivative(var, variable, order);
        },
        token
    );
}

mlp::Token mlp::integral(Token const &token, Variable variable) {
    return std::visit(
        [&variable](auto &&var) -> Token { return integral(var, variable); },
        token
    );
}

mlp::Token mlp::integral(
    Token const &token, Variable variable, Token const &from, Token const &to
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
    Token *last = nullptr;

    for (std::size_t i = 0; i < expression.size(); ++i) {
        std::size_t const copy = i;

        std::variant<Operation, std::optional<Token>> token =
            get_next_token(expression, i);

        if (!std::holds_alternative<Operation>(token)) {
            auto &term = std::get<std::optional<Token>>(token);

            if (!term)
                continue;

            numerator.push_back(std::move(*term));
            last = &numerator.back();

            continue;
        }

        auto &operation = std::get<Operation>(token);

        if (copy == 0) {
            switch (operation) {
            case Operation::mul:
            case Operation::div:
            case Operation::pow:
                throw std::runtime_error("Expression is not valid!");
            case Operation::sub:
                s = Sign::neg;
            case Operation::add:
                break;
            }

            continue;
        }

        std::variant<Operation, std::optional<Token>> next =
            get_next_token(expression, ++i);

        if (std::holds_alternative<Operation>(next))
            throw std::runtime_error("Expression is not valid!");

        auto &next_term = std::get<std::optional<Token>>(next);

        if (!next_term)
            throw std::runtime_error("Expression is not valid!");

        if (operation == Operation::add || operation == Operation::sub) {
            Sign const sign =
                operation == Operation::add ? Sign::pos : Sign::neg;

            if (last) {
                Terms terms{};

                for (Token const &t : numerator)
                    terms *= t;

                for (Token const &t : denominator)
                    terms /= t;

                result.add_token(s, simplified(terms));

                numerator.clear();
                denominator.clear();
            }

            else if (sign == s)
                result += 1;

            else
                result -= 1;

            s = sign;
            numerator.push_back(std::move(*next_term));
            last = &numerator.back();

            continue;
        }

        if (!last)
            throw std::runtime_error("Expression is not valid!");

        if (operation == Operation::mul) {
            numerator.push_back(std::move(*next_term));
            last = &numerator.back();

            continue;
        }

        if (operation == Operation::div) {
            denominator.push_back(std::move(*next_term));
            last = &denominator.back();

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

                if (operation == Operation::pow)
                    continue;

                --i;

                break;
            }

            if (!next_term)
                break;
        }

        Token power = 1.0;

        for (Token &p : powers | std::views::reverse)
            power = pow(p, power);

        power = std::move(simplified(power));

        *last = pow(*last, power);
    }

    Terms terms{};

    for (Token const &t : numerator)
        terms *= t;

    for (Token const &t : denominator)
        terms /= t;

    result.add_token(s, simplified(terms));

    return simplified(result);
}

mlp::Token mlp::pow(Token const &lhs, Constant const rhs) {
    return std::visit(
        [rhs](auto &&var) -> Token { return pow(var, rhs); }, lhs
    );
}

mlp::Token mlp::pow(Constant lhs, Token rhs) {
    return std::visit(
        [lhs](auto &&var) -> Token { return pow(lhs, var); }, rhs
    );
}

mlp::Token mlp::pow(Token const &lhs, Token const &rhs) {
    return Term{1, lhs, rhs};
}

namespace mlp {
Token operator-(Token const &token) {
    return std::visit([](auto &&var) -> Token { return -var; }, token);
}

Token operator+(Token const &lhs, Token const &rhs) {
    return std::visit(
        [&rhs](auto &&outer) -> Token {
            return std::visit(
                [&outer](auto &&inner) -> Token { return outer + inner; }, rhs
            );
        },
        lhs
    );
}

Token operator-(Token const &lhs, Token const &rhs) {
    return std::visit(
        [&rhs](auto &&outer) -> Token {
            return std::visit(
                [&outer](auto &&inner) -> Token { return outer - inner; }, rhs
            );
        },
        lhs
    );
}

Token operator*(Token const &lhs, Token const &rhs) {
    return std::visit(
        [&rhs](auto &&outer) -> Token {
            return std::visit(
                [&outer](auto &&inner) -> Token { return outer * inner; }, rhs
            );
        },
        lhs
    );
}

Token operator*(Token const &lhs, Expression rhs) {
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

std::istream &operator>>(std::istream &input, Token &output) {
    std::string expression;
    input >> expression;
    output = tokenise(expression);

    return input;
}
} // namespace mlp

std::string to_string(mlp::Token const &token) {
    return std::visit(
        []<typename T>(T &&var) -> std::string {
            if constexpr (std::is_same_v<std::decay_t<T>, mlp::Constant>)
                return std::to_string(var);

            else
                return static_cast<std::string>(var);
        },
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