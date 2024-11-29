#include "../include/expression.h"

#include "../include/constant.h"
#include "../include/function.h"
#include "../include/term.h"
#include "../include/terms.h"
#include "../include/variable.h"

#include <map>
#include <ranges>
#include <sstream>

mlp::Expression::Expression(Expression const &expression) {
    *this = expression;
}

mlp::Expression &mlp::Expression::operator=(Expression const &expression) {
    this->tokens.clear();

    for (auto const &[operation, token] : expression.tokens)
        this->add_token(operation, *token);

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
        result << this->tokens[0].first << to_string(*this->tokens[0].second);
    } else {
        result << '(';

        for (auto const &[operation, token] : this->tokens)
            result << operation << to_string(*token);

        result << ')';
    }

    return result.str();
}

mlp::Expression mlp::Expression::operator-() const {
    Expression expression;

    for (auto const &[sign, token] : this->tokens)
        if (sign == Sign::pos)
            expression -= *token;
        else
            expression += *token;

    return expression;
}

mlp::Expression &mlp::Expression::operator+=(Token token) {
    if (std::holds_alternative<Expression>(token)) {
        for (auto &[sign, t] : std::get<Expression>(token).tokens)
            if (sign == Sign::pos)
                *this += *t;
            else
                *this -= *t;

        return *this;
    }

    if (std::holds_alternative<Constant>(token)) {
        auto const &constant = std::get<Constant>(token);

        if (constant == 0)
            return *this;

        for (std::size_t i = 0; i < this->tokens.size(); ++i) {
            auto &[sign, t] = this->tokens[i];

            if (!std::holds_alternative<Constant>(*t))
                continue;

            auto &c = std::get<Constant>(*t);

            if (sign == Sign::neg) {
                c = -c;
                sign = Sign::pos;
            }

            c += constant;

            if (c == 0)
                this->tokens.erase(this->tokens.begin() + i);

            else if (c < 0) {
                c = -c;
                sign = Sign::neg;
            }

            return *this;
        }
    }

    if (std::holds_alternative<Variable>(token)) {
        auto const &variable = std::get<Variable>(token);

        for (std::size_t i = 0; i < this->tokens.size(); ++i) {
            auto &[sign, t] = this->tokens[i];

            if (std::holds_alternative<Variable>(*t)) {
                if (auto const &v = std::get<Variable>(*t); v != variable)
                    continue;

                if (sign == Sign::neg) {
                    this->tokens.erase(this->tokens.begin() + i);
                } else {
                    *t = 2 * variable;
                }

                return *this;
            }

            if (std::holds_alternative<Term>(*t)) {
                auto &term = std::get<Term>(*t);

                if (!std::holds_alternative<Variable>(*term.base) ||
                    std::get<Variable>(*term.base) != variable ||
                    !std::holds_alternative<Constant>(*term.power) ||
                    std::get<Constant>(*term.power) != 1)
                    continue;

                ++term.coefficient;

                if (term.coefficient == 0)
                    this->tokens.erase(this->tokens.begin() + i);

                if (term.coefficient < 0) {
                    sign = sign == Sign::pos ? Sign::neg : Sign::pos;
                    term.coefficient = -term.coefficient;
                }

                if (term.coefficient == 1)
                    *t = std::move(*term.base);

                return *this;
            }
        }

        this->tokens.emplace_back(Sign::pos, new Token(token));

        return *this;
    }

    if (std::holds_alternative<Term>(token)) {
        auto &term = std::get<Term>(token);

        if (term.coefficient == 0)
            return *this;

        if (term.coefficient < 0) {
            term.coefficient = -term.coefficient;

            return *this -= term;
        }

        if (!std::holds_alternative<Variable>(*term.base) ||
            !std::holds_alternative<Constant>(*term.power)) {
            this->tokens.emplace_back(Sign::pos, new Token(token));

            return *this;
        }

        auto const &variable = std::get<Variable>(*term.base);

        for (std::size_t i = 0; i < this->tokens.size(); ++i) {
            auto &[sign, t] = this->tokens[i];

            if (std::holds_alternative<Variable>(*t)) {
                if (auto const &v = std::get<Variable>(*t); v != variable)
                    continue;

                Term temp = (sign == Sign::pos ? 1 + term.coefficient
                                               : 1 - term.coefficient) *
                            variable;

                if (temp.coefficient == 0)
                    this->tokens.erase(this->tokens.begin() + i);

                if (temp.coefficient < 0) {
                    sign = sign == Sign::pos ? Sign::neg : Sign::pos;
                    temp.coefficient = -temp.coefficient;
                }

                if (temp.coefficient == 1)
                    *t = std::move(*temp.base);

                else
                    *t = std::move(temp);

                return *this;
            }

            if (std::holds_alternative<Term>(*t)) {
                auto &term_ = std::get<Term>(*t);

                if (!std::holds_alternative<Variable>(*term_.base) ||
                    std::get<Variable>(*term_.base) != variable ||
                    !std::holds_alternative<Constant>(*term_.power) ||
                    std::get<Constant>(*term_.power) != 1)
                    continue;

                term_.coefficient += term.coefficient;

                if (term_.coefficient == 0)
                    this->tokens.erase(this->tokens.begin() + i);

                if (term_.coefficient < 0) {
                    sign = sign == Sign::pos ? Sign::neg : Sign::pos;
                    term_.coefficient = -term_.coefficient;
                }

                if (term_.coefficient == 1)
                    *t = std::move(*term_.base);

                return *this;
            }
        }
    }

    if (std::holds_alternative<Terms>(token)) {
        auto &terms = std::get<Terms>(token);

        if (terms.coefficient == 0)
            return *this;

        if (terms.coefficient < 0) {
            terms.coefficient = -terms.coefficient;

            return *this -= terms;
        }
    }

    this->tokens.emplace_back(Sign::pos, new Token(token));

    return *this;
}

mlp::Expression &mlp::Expression::operator-=(Token token) {
    if (std::holds_alternative<Expression>(token)) {
        for (auto &[sign, t] : std::get<Expression>(token).tokens)
            if (sign == Sign::pos)
                *this -= *t;
            else
                *this += *t;

        return *this;
    }

    if (std::holds_alternative<Constant>(token)) {
        auto const &constant = std::get<Constant>(token);

        if (constant == 0)
            return *this;

        for (std::size_t i = 0; i < this->tokens.size(); ++i) {
            auto &[sign, t] = this->tokens[i];

            if (!std::holds_alternative<Constant>(*t))
                continue;

            auto &c = std::get<Constant>(*t);

            if (sign == Sign::neg) {
                c = -c;
                sign = Sign::pos;
            }

            c -= constant;

            if (c == 0)
                this->tokens.erase(this->tokens.begin() + i);

            else if (c < 0) {
                c = -c;
                sign = Sign::neg;
            }

            return *this;
        }
    }

    if (std::holds_alternative<Variable>(token)) {
        auto const &variable = std::get<Variable>(token);

        for (std::size_t i = 0; i < this->tokens.size(); ++i) {
            auto &[sign, t] = this->tokens[i];

            if (std::holds_alternative<Variable>(*t)) {
                if (auto const &v = std::get<Variable>(*t); v != variable)
                    continue;

                if (sign == Sign::pos) {
                    this->tokens.erase(this->tokens.begin() + i);
                } else {
                    *t = 2 * variable;
                }

                return *this;
            }

            if (std::holds_alternative<Term>(*t)) {
                auto &term = std::get<Term>(*t);

                if (!std::holds_alternative<Variable>(*term.base) ||
                    std::get<Variable>(*term.base) != variable ||
                    !std::holds_alternative<Constant>(*term.power) ||
                    std::get<Constant>(*term.power) != 1)
                    continue;

                --term.coefficient;

                if (term.coefficient == 0)
                    this->tokens.erase(this->tokens.begin() + i);

                if (term.coefficient < 0) {
                    sign = sign == Sign::pos ? Sign::neg : Sign::pos;
                    term.coefficient = -term.coefficient;
                }

                if (term.coefficient == 1)
                    *t = std::move(*term.base);

                return *this;
            }
        }

        this->tokens.emplace_back(Sign::neg, new Token(std::move(token)));

        return *this;
    }

    if (std::holds_alternative<Term>(token)) {
        auto &term = std::get<Term>(token);

        if (term.coefficient == 0)
            return *this;

        if (term.coefficient < 0) {
            term.coefficient = -term.coefficient;

            return *this += term;
        }

        if (!std::holds_alternative<Variable>(*term.base) ||
            !std::holds_alternative<Constant>(*term.power)) {
            this->tokens.emplace_back(Sign::neg, new Token(std::move(token)));

            return *this;
        }

        auto const &variable = std::get<Variable>(*term.base);

        for (std::size_t i = 0; i < this->tokens.size(); ++i) {
            auto &[sign, t] = this->tokens[i];

            if (std::holds_alternative<Variable>(*t)) {
                if (auto const &v = std::get<Variable>(*t); v != variable)
                    continue;

                Term temp = (sign == Sign::pos ? 1 - term.coefficient
                                               : 1 + term.coefficient) *
                            variable;

                if (temp.coefficient == 0)
                    this->tokens.erase(this->tokens.begin() + i);

                if (temp.coefficient < 0) {
                    sign = sign == Sign::pos ? Sign::neg : Sign::pos;
                    temp.coefficient = -temp.coefficient;
                }

                if (temp.coefficient == 1)
                    *t = std::move(*temp.base);

                else
                    *t = std::move(temp);

                return *this;
            }

            if (std::holds_alternative<Term>(*t)) {
                auto &term_ = std::get<Term>(*t);

                if (!std::holds_alternative<Variable>(*term_.base) ||
                    std::get<Variable>(*term_.base) != variable ||
                    !std::holds_alternative<Constant>(*term_.power) ||
                    std::get<Constant>(*term_.power) != 1)
                    continue;

                term_.coefficient -= term.coefficient;

                if (term_.coefficient == 0)
                    this->tokens.erase(this->tokens.begin() + i);

                if (term_.coefficient < 0) {
                    sign = sign == Sign::pos ? Sign::neg : Sign::pos;
                    term_.coefficient = -term_.coefficient;
                }

                if (term_.coefficient == 1)
                    *t = std::move(*term_.base);

                return *this;
            }
        }
    }

    if (std::holds_alternative<Terms>(token)) {
        auto &terms = std::get<Terms>(token);

        if (terms.coefficient == 0)
            return *this;

        if (terms.coefficient < 0) {
            terms.coefficient = -terms.coefficient;

            return *this += terms;
        }
    }

    this->tokens.emplace_back(Sign::neg, new Token(token));

    return *this;
}

mlp::Expression &mlp::Expression::operator*=(Token const &token) {
    for (auto &[sign, t] : this->tokens) {
        Terms temp = *t * token;

        if (temp.coefficient < 0) {
            temp.coefficient = -temp.coefficient;

            sign = sign == Sign::pos ? Sign::neg : Sign::pos;
        }

        *t = simplified(temp);
    }

    return *this;
}

mlp::Expression &mlp::Expression::operator*=(Expression const &rhs) {
    Expression expression;

    for (auto &[sign, t] : this->tokens)
        expression.add_token(sign, rhs * *t);

    return *this = std::move(expression);
}

mlp::Expression mlp::operator+(Expression lhs, Token const &rhs) {
    return std::move(lhs += rhs);
}

mlp::Expression mlp::operator-(Expression lhs, Token const &rhs) {
    return std::move(lhs -= rhs);
}

mlp::Expression mlp::operator*(Expression lhs, Expression const &rhs) {
    return std::move(lhs *= rhs);
}

mlp::Expression mlp::operator*(Expression lhs, Token const &rhs) {
    return std::move(lhs *= rhs);
}

namespace mlp {
bool is_dependent_on(Expression const &token, Variable const &variable) {
    return std::ranges::any_of(
        token.tokens,
        [variable](std::pair<Sign, OwnedToken> const &term) -> bool {
            return is_dependent_on(*term.second, variable);
        }
    );
}

bool is_linear_of(Expression const &token, Variable const &variable) {
    if (!is_dependent_on(token, variable))
        return false;

    bool is_linear = false;

    for (OwnedToken const &t : token.tokens | std::views::values) {
        if (std::holds_alternative<Constant>(*t))
            continue;

        if (!is_linear_of(*t, variable))
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
        *term = evaluate(*term, values);

    return simplified(expression);
}

Token simplified(Expression const &token) {
    if (token.tokens.empty())
        return Constant(0);

    if (token.tokens.size() == 1) {
        Expression expression{token};

        if (expression.tokens.empty())
            return Constant(0);

        auto [sign, term] = std::move(expression.tokens.back());

        expression.tokens.pop_back();

        if (sign == Sign::pos)
            return simplified(*term);

        return simplified(-*term);
    }

    Expression expression{};

    std::vector<std::pair<Sign, OwnedToken>> const &tokens = expression.tokens;

    for (auto const &[sign, t] : token.tokens)
        expression.add_token(sign, simplified(*t));

    if (tokens.empty())
        return Constant(0);

    if (tokens.size() == 1) {
        if (expression.tokens.empty())
            return Constant(0);

        auto [sign, term] = std::move(expression.tokens.back());

        expression.tokens.pop_back();

        if (sign == Sign::pos)
            return *term;

        return simplified(-*term);
    }

    return expression;
}

Token derivative(
    Expression const &token, Variable const &variable, std::uint32_t const order
) {
    if (!order)
        return token;

    if (!is_dependent_on(token, variable))
        return Constant(0);

    Expression result{};

    for (auto const &[operation, token_] : token.tokens)
        result.add_token(operation, derivative(*token_, variable, 1));

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
        result.add_token(operation, integral(*term, variable));

    return simplified(result);
}
} // namespace mlp
