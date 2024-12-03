#include "../include/expression.h"

#include "../include/function.h"
#include "../include/term.h"
#include "../include/terms.h"
#include "../include/variable.h"

#include <algorithm>
#include <map>
#include <ranges>
#include <sstream>

mlp::Expression::Expression(Expression const &expression) {
    *this = expression;
}

mlp::Expression &mlp::Expression::operator=(Expression const &expression) {
    this->tokens.clear();

    for (auto const &[operation, token] : expression.tokens)
        this->add_token(operation, token);

    return *this;
}

void mlp::Expression::add_token(Sign const sign, Token const &token) {
    if (sign == Sign::pos)
        *this += token;

    else
        *this -= token;
}

mlp::Expression::operator std::string() const {
    std::stringstream result;

    if (this->tokens.size() == 1) {
        result << this->tokens[0].first << to_string(this->tokens[0].second);
    } else {
        result << '(';

        for (auto const &[operation, token] : this->tokens)
            result << operation << to_string(token);

        result << ')';
    }

    return result.str();
}

mlp::Expression mlp::Expression::operator-() const {
    Expression expression;

    for (auto const &[sign, token] : this->tokens)
        if (sign == Sign::pos)
            expression -= token;
        else
            expression += token;

    return expression;
}

mlp::Expression &mlp::Expression::operator+=(Expression const &rhs) {
    for (auto &[sign, t] : rhs.tokens)
        if (sign == Sign::pos)
            *this += t;
        else
            *this -= t;

    return *this;
}

mlp::Expression &mlp::Expression::operator+=(Constant const rhs) {
    if (rhs == 0)
        return *this;

    if (rhs < 0)
        return *this -= -rhs;

    for (std::size_t i = 0; i < this->tokens.size(); ++i) {
        auto &[sign, token] = this->tokens[i];

        if (!std::holds_alternative<Constant>(token))
            continue;

        auto &constant = std::get<Constant>(token);

        if (sign == Sign::neg) {
            constant = -constant;
            sign = Sign::pos;
        }

        constant += rhs;

        if (constant == 0)
            this->tokens.erase(this->tokens.begin() + i);

        else if (constant < 0) {
            constant = -constant;
            sign = Sign::neg;
        }

        return *this;
    }

    this->tokens.emplace_back(Sign::pos, rhs);

    return *this;
}

mlp::Expression &mlp::Expression::operator+=(Variable const &rhs) {
    if (rhs.coefficient == 0)
        return *this;

    if (rhs.coefficient < 0)
        return *this -= -rhs;

    for (std::size_t i = 0; i < this->tokens.size(); ++i) {
        auto &[sign, token] = this->tokens[i];

        if (std::holds_alternative<Variable>(token)) {
            auto &variable = std::get<Variable>(token);

            if (variable != rhs)
                continue;

            if (sign == Sign::neg) {
                variable.coefficient = -variable.coefficient;
                sign = Sign::pos;
            }

            token = variable + rhs;

            if (std::holds_alternative<Variable>(token)) {
                if (variable.coefficient == 0)
                    this->tokens.erase(this->tokens.begin() + i);

                else if (variable.coefficient < 0) {
                    variable.coefficient = -variable.coefficient;
                    sign = sign == Sign::pos ? Sign::neg : Sign::pos;
                }
            }

            return *this;
        }

        if (std::holds_alternative<Term>(token)) {
            auto &term = std::get<Term>(token);

            if (!std::holds_alternative<Variable>(*term.base) ||
                std::get<Variable>(*term.base) != rhs ||
                !std::holds_alternative<Constant>(*term.power) ||
                std::get<Constant>(*term.power) != 1)
                continue;

            if (sign == Sign::neg) {
                term.coefficient = -term.coefficient;
                sign = Sign::pos;
            }

            term.coefficient += rhs.coefficient;

            if (term.coefficient == 0)
                this->tokens.erase(this->tokens.begin() + i);

            if (term.coefficient < 0) {
                sign = sign == Sign::pos ? Sign::neg : Sign::pos;
                term.coefficient = -term.coefficient;
            }

            if (term.coefficient == 1)
                token = std::move(*term.base);

            return *this;
        }
    }

    this->tokens.emplace_back(Sign::pos, rhs);

    return *this;
}

mlp::Expression &mlp::Expression::operator+=(Function const &rhs) {
    this->tokens.emplace_back(Sign::pos, rhs);

    return *this;
}

mlp::Expression &mlp::Expression::operator+=(Term const &rhs) {
    if (rhs.coefficient == 0)
        return *this;

    if (rhs.coefficient < 0)
        return *this -= -rhs;

    if (!std::holds_alternative<Variable>(*rhs.base) ||
        !std::holds_alternative<Constant>(*rhs.power)) {
        this->tokens.emplace_back(Sign::pos, rhs);

        return *this;
    }

    auto const &variable = std::get<Variable>(*rhs.base);

    for (std::size_t i = 0; i < this->tokens.size(); ++i) {
        auto &[sign, token] = this->tokens[i];

        if (std::holds_alternative<Variable>(token)) {
            auto &v = std::get<Variable>(token);

            if (v != variable)
                continue;

            if (sign == Sign::neg) {
                v.coefficient = -v.coefficient;
                sign = Sign::pos;
            }

            v.coefficient += rhs.coefficient;

            if (v.coefficient == 0)
                this->tokens.erase(this->tokens.begin() + i);

            if (v.coefficient < 0) {
                v.coefficient = -v.coefficient;
                sign = sign == Sign::pos ? Sign::neg : Sign::pos;
            }

            return *this;
        }

        if (std::holds_alternative<Term>(token)) {
            auto &term = std::get<Term>(token);

            if (!std::holds_alternative<Variable>(*term.base) ||
                std::get<Variable>(*term.base) != variable ||
                !std::holds_alternative<Constant>(*term.power) ||
                std::get<Constant>(*term.power) != 1)
                continue;

            term.coefficient += rhs.coefficient;

            if (term.coefficient == 0)
                this->tokens.erase(this->tokens.begin() + i);

            if (term.coefficient < 0) {
                sign = sign == Sign::pos ? Sign::neg : Sign::pos;
                term.coefficient = -term.coefficient;
            }

            if (term.coefficient == 1)
                token = std::move(*term.base);

            return *this;
        }
    }

    this->tokens.emplace_back(Sign::pos, rhs);

    return *this;
}

mlp::Expression &mlp::Expression::operator+=(Terms const &rhs) {
    if (rhs.coefficient == 0)
        return *this;

    if (rhs.coefficient < 0)
        return *this -= -rhs;

    this->tokens.emplace_back(Sign::pos, rhs);

    return *this;
}

mlp::Expression &mlp::Expression::operator+=(Token const &token) {
    return std::visit(
        [this](auto &&var) -> Expression & { return *this += var; }, token
    );
}

mlp::Expression &mlp::Expression::operator-=(Expression const &rhs) {
    for (auto &[sign, t] : rhs.tokens)
        if (sign == Sign::pos)
            *this -= t;
        else
            *this += t;

    return *this;
}

mlp::Expression &mlp::Expression::operator-=(Constant const rhs) {
    if (rhs == 0)
        return *this;

    if (rhs < 0)
        return *this += -rhs;

    for (std::size_t i = 0; i < this->tokens.size(); ++i) {
        auto &[sign, t] = this->tokens[i];

        if (!std::holds_alternative<Constant>(t))
            continue;

        auto &constant = std::get<Constant>(t);

        if (sign == Sign::neg) {
            constant = -constant;
            sign = Sign::pos;
        }

        constant -= rhs;

        if (constant == 0)
            this->tokens.erase(this->tokens.begin() + i);

        else if (constant < 0) {
            constant = -constant;
            sign = Sign::neg;
        }

        return *this;
    }

    this->tokens.emplace_back(Sign::neg, rhs);

    return *this;
}

mlp::Expression &mlp::Expression::operator-=(Variable const &rhs) {
    if (rhs.coefficient == 0)
        return *this;

    if (rhs.coefficient < 0)
        return *this += -rhs;

    for (std::size_t i = 0; i < this->tokens.size(); ++i) {
        auto &[sign, token] = this->tokens[i];

        if (std::holds_alternative<Variable>(token)) {
            auto &variable = std::get<Variable>(token);

            if (variable != rhs)
                continue;

            if (sign == Sign::neg) {
                variable.coefficient = -variable.coefficient;
                sign = Sign::pos;
            }

            token = variable - rhs;

            if (std::holds_alternative<Variable>(token)) {
                if (variable.coefficient == 0)
                    this->tokens.erase(this->tokens.begin() + i);

                else if (variable.coefficient < 0) {
                    variable.coefficient = -variable.coefficient;
                    sign = sign == Sign::pos ? Sign::neg : Sign::pos;
                }
            }

            return *this;
        }

        if (std::holds_alternative<Term>(token)) {
            auto &term = std::get<Term>(token);

            if (!std::holds_alternative<Variable>(*term.base) ||
                std::get<Variable>(*term.base) != rhs ||
                !std::holds_alternative<Constant>(*term.power) ||
                std::get<Constant>(*term.power) != 1)
                continue;

            if (sign == Sign::neg) {
                term.coefficient = -term.coefficient;
                sign = Sign::pos;
            }

            term.coefficient -= rhs.coefficient;

            if (term.coefficient == 0)
                this->tokens.erase(this->tokens.begin() + i);

            if (term.coefficient < 0) {
                sign = sign == Sign::pos ? Sign::neg : Sign::pos;
                term.coefficient = -term.coefficient;
            }

            if (term.coefficient == 1)
                token = std::move(*term.base);

            return *this;
        }
    }

    this->tokens.emplace_back(Sign::neg, rhs);

    return *this;
}

mlp::Expression &mlp::Expression::operator-=(Function const &rhs) {
    this->tokens.emplace_back(Sign::neg, rhs);

    return *this;
}

mlp::Expression &mlp::Expression::operator-=(Term const &rhs) {
    if (rhs.coefficient == 0)
        return *this;

    if (rhs.coefficient < 0)
        return *this += -rhs;

    if (!std::holds_alternative<Variable>(*rhs.base) ||
        !std::holds_alternative<Constant>(*rhs.power)) {
        this->tokens.emplace_back(Sign::neg, rhs);

        return *this;
    }

    auto const &variable = std::get<Variable>(*rhs.base);

    for (std::size_t i = 0; i < this->tokens.size(); ++i) {
        auto &[sign, token] = this->tokens[i];

        if (std::holds_alternative<Variable>(token)) {
            auto &v = std::get<Variable>(token);

            if (v != variable)
                continue;

            if (sign == Sign::neg) {
                v.coefficient = -v.coefficient;
                sign = Sign::pos;
            }

            v.coefficient -= rhs.coefficient;

            if (v.coefficient == 0)
                this->tokens.erase(this->tokens.begin() + i);

            if (v.coefficient < 0) {
                v.coefficient = -v.coefficient;
                sign = sign == Sign::pos ? Sign::neg : Sign::pos;
            }

            return *this;
        }

        if (std::holds_alternative<Term>(token)) {
            auto &term = std::get<Term>(token);

            if (!std::holds_alternative<Variable>(*term.base) ||
                std::get<Variable>(*term.base) != variable ||
                !std::holds_alternative<Constant>(*term.power) ||
                std::get<Constant>(*term.power) != 1)
                continue;

            term.coefficient -= rhs.coefficient;

            if (term.coefficient == 0)
                this->tokens.erase(this->tokens.begin() + i);

            if (term.coefficient < 0) {
                sign = sign == Sign::pos ? Sign::neg : Sign::pos;
                term.coefficient = -term.coefficient;
            }

            if (term.coefficient == 1)
                token = std::move(*term.base);

            return *this;
        }
    }

    this->tokens.emplace_back(Sign::neg, rhs);

    return *this;
}

mlp::Expression &mlp::Expression::operator-=(Terms const &rhs) {
    if (rhs.coefficient == 0)
        return *this;

    if (rhs.coefficient < 0)
        return *this += -rhs;

    this->tokens.emplace_back(Sign::neg, rhs);

    return *this;
}

mlp::Expression &mlp::Expression::operator-=(Token const &token) {
    return std::visit(
        [this](auto &&var) -> Expression & { return *this -= var; }, token
    );
}

mlp::Expression &mlp::Expression::operator*=(Token const &token) {
    for (auto &[sign, t] : this->tokens) {
        Token temp = t * token;

        Constant *coefficient = nullptr;

        if (std::holds_alternative<Constant>(temp))
            coefficient = &std::get<Constant>(temp);

        else if (std::holds_alternative<Variable>(temp))
            coefficient = &std::get<Variable>(temp).coefficient;

        else if (std::holds_alternative<Term>(temp))
            coefficient = &std::get<Term>(temp).coefficient;

        else if (std::holds_alternative<Terms>(temp))
            coefficient = &std::get<Terms>(temp).coefficient;

        if (coefficient && *coefficient < 0) {
            *coefficient = -*coefficient;

            sign = sign == Sign::pos ? Sign::neg : Sign::pos;
        }

        t = simplified(temp);
    }

    return *this;
}

mlp::Expression &mlp::Expression::operator*=(Expression const &rhs) {
    Expression expression;

    for (auto &[sign, t] : this->tokens)
        expression.add_token(sign, rhs * t);

    return *this = std::move(expression);
}

mlp::Expression &mlp::Expression::operator/=(Token const &rhs) {
    for (auto &[sign, t] : this->tokens) {
        Terms temp = t / rhs;

        if (temp.coefficient < 0) {
            temp.coefficient = -temp.coefficient;

            sign = sign == Sign::pos ? Sign::neg : Sign::pos;
        }

        t = simplified(temp);
    }

    return *this;
}

bool mlp::Expression::operator==(Expression const &) const { return false; }

mlp::Expression mlp::operator+(Expression lhs, Token const &rhs) {
    return std::move(lhs += rhs);
}

mlp::Expression mlp::operator-(Expression lhs, Token const &rhs) {
    return std::move(lhs -= rhs);
}

mlp::Token mlp::operator*(Expression lhs, Constant const rhs) {
    if (rhs == 0)
        return 0.0;

    if (rhs == 1)
        return lhs;

    return lhs *= rhs;
}

mlp::Expression mlp::operator*(Expression lhs, Expression const &rhs) {
    return std::move(lhs *= rhs);
}

mlp::Expression mlp::operator*(Expression lhs, Token const &rhs) {
    return std::move(lhs *= rhs);
}

mlp::Token mlp::operator/(Expression lhs, Constant const rhs) {
    if (rhs == 0)
        throw std::domain_error{"Division by 0!"};

    if (rhs == 1)
        return lhs;

    return lhs /= rhs;
}

mlp::Token mlp::pow(Expression const &lhs, Constant const rhs) {
    if (rhs == 0)
        return 1.0;

    if (rhs == 1)
        return lhs;

    return Term{1, std::make_unique<Token>(lhs), std::make_unique<Token>(rhs)};
}

mlp::Token mlp::pow(Expression const &lhs, Token const &rhs) {
    return Term{1, std::make_unique<Token>(lhs), std::make_unique<Token>(rhs)};
}

namespace mlp {
bool is_dependent_on(Expression const &token, Variable const &variable) {
    return std::ranges::any_of(
        token.tokens,
        [variable](std::pair<Sign, Token> const &term) -> bool {
            return is_dependent_on(term.second, variable);
        }
    );
}

bool is_linear_of(Expression const &token, Variable const &variable) {
    if (!is_dependent_on(token, variable))
        return false;

    bool is_linear = false;

    for (Token const &t : token.tokens | std::views::values) {
        if (std::holds_alternative<Constant>(t))
            continue;

        if (!is_linear_of(t, variable))
            return false;

        is_linear = true;
    }

    return is_linear;
}

Token evaluate(
    Expression const &token, std::map<Variable, Token> const &values
) {
    Expression expression{token};

    for (auto &term : expression.tokens | std::views::values)
        term = evaluate(term, values);

    return simplified(expression);
}

Token simplified(Expression const &token) {
    if (token.tokens.empty())
        return 0.0;

    if (token.tokens.size() == 1) {
        Expression expression{token};

        if (expression.tokens.empty())
            return 0.0;

        auto [sign, term] = std::move(expression.tokens.back());

        expression.tokens.pop_back();

        if (sign == Sign::pos)
            return simplified(term);

        return simplified(-term);
    }

    Expression expression{};

    std::vector<std::pair<Sign, Token>> const &tokens = expression.tokens;

    for (auto const &[sign, t] : token.tokens)
        expression.add_token(sign, simplified(t));

    if (tokens.empty())
        return 0.0;

    if (tokens.size() == 1) {
        if (expression.tokens.empty())
            return 0.0;

        auto [sign, term] = std::move(expression.tokens.back());

        expression.tokens.pop_back();

        if (sign == Sign::pos)
            return term;

        return simplified(-term);
    }

    return expression;
}

Token derivative(
    Expression const &token, Variable const &variable, std::uint32_t const order
) {
    if (!order)
        return token;

    if (!is_dependent_on(token, variable))
        return 0.0;

    Expression result{};

    for (auto const &[operation, token_] : token.tokens)
        result.add_token(operation, derivative(token_, variable, 1));

    auto derivative = simplified(result);

    if (order > 1)
        return simplified(mlp::derivative(derivative, variable, order - 1));

    return derivative;
}

Token integral(Expression const &token, Variable const &variable) {
    if (!is_dependent_on(token, variable))
        return variable * token;

    Expression result{};

    for (auto const &[operation, term] : token.tokens)
        result.add_token(operation, integral(term, variable));

    return simplified(result);
}

Token pow(Constant const lhs, Expression const &rhs) {
    Terms terms;

    for (auto const &[sign, token] : rhs.tokens)
        if (sign == Sign::pos)
            terms *= pow(lhs, token);
        else
            terms /= pow(lhs, token);

    return terms;
}

Token pow(Token const &lhs, Expression const &rhs) {
    Terms terms;

    for (auto const &[sign, token] : rhs.tokens)
        if (sign == Sign::pos)
            terms *= pow(lhs, token);
        else
            terms /= pow(lhs, token);

    return terms;
}
} // namespace mlp
