#include "token.h"

#include <algorithm>
#include <map>
#include <numbers>
#include <ranges>
#include <regex>
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

mlp::Term mlp::Token::operator^(std::double_t exponent) const & {
    return {1, OwnedToken(this->clone()), std::make_unique<Constant>(exponent)};
}

mlp::Term mlp::Token::operator^(Token const &exponent) const & {
    return {1, OwnedToken(this->clone()), OwnedToken(exponent.clone())};
}

mlp::Term mlp::Token::operator^(OwnedToken &&exponent) const & {
    return {1, OwnedToken(this->clone()), std::move(exponent)};
}

mlp::Term mlp::Token::operator^(std::double_t exponent) && {
    return {1, OwnedToken(this), std::make_unique<Constant>(exponent)};
}

mlp::Term mlp::Token::operator^(Token const &exponent) && {
    return {1, OwnedToken(this), OwnedToken(exponent.clone())};
}

mlp::Term mlp::Token::operator^(OwnedToken &&exponent) && {
    return {1, OwnedToken(this), std::move(exponent)};
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

    Operation op{Operation::add};

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
                op.operation == Operation::add ? Sign::pos : Sign::neg;

            if (!numerator.empty()) {
                Terms terms{};

                for (OwnedToken &t : numerator)
                    terms *= std::move(t);

                for (OwnedToken &t : denominator)
                    terms /= std::move(t);

                result.add_token(sign, terms.simplified());

                numerator.clear();
                denominator.clear();
            }

            else if (sign == s)
                result.add_token(Sign::pos, std::make_unique<Constant>(1));

            else
                result.add_token(Sign::neg, std::make_unique<Constant>(1));

            op = operation;

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
            power = std::make_unique<Term>(
                std::move(*p.release()) ^ std::move(power)
            );

        power = power->simplified();

        if (!last)
            numerator.back() = std::make_unique<Term>(
                std::move(*numerator.back().release()) ^ std::move(power)
            );
        else
            denominator.back() = std::make_unique<Term>(
                std::move(*denominator.back().release()) ^ std::move(power)
            );
    }

    Terms terms{};

    for (OwnedToken &t : numerator)
        terms *= std::move(t);

    for (OwnedToken &t : denominator)
        terms /= std::move(t);

    result.add_token(s, terms.simplified());

    return result.simplified();
}

namespace mlp {
std::ostream &operator<<(std::ostream &os, Token const &token) {
    os << static_cast<std::string>(token);

    return os;
}

std::ostream &operator<<(std::ostream &os, Sign const sign) {
    os << to_string(sign);

    return os;
}
} // namespace mlp

std::string to_string(mlp::Sign const sign) {
    return sign == mlp::Sign::pos ? "+" : "-";
}