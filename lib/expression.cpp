#include "../include/expression.h"

#include "../include/constant.h"
#include "../include/function.h"
#include "../include/term.h"
#include "../include/terms.h"
#include "../include/variable.h"

#include <ranges>
#include <sstream>

mlp::Expression::Expression(Expression const &expression) {
    for (auto const &[operation, token] : expression.tokens)
        this->add_token(operation, OwnedToken(token->clone()));
}

gsl::owner<mlp::Expression *> mlp::Expression::clone() const {
    return new Expression(*this);
}

gsl::owner<mlp::Expression *> mlp::Expression::move() && {
    return new Expression(std::move(*this));
}

void mlp::Expression::add_token(Sign const sign, OwnedToken &&token) {
    this->add_token(sign, std::move(*token));
}

void mlp::Expression::add_token(Sign const sign, Token const &token) {
    this->add_token(sign, OwnedToken(token.clone()));
}

void mlp::Expression::add_token(Sign const sign, Token &&token) {
    if (sign == Sign::pos)
        *this += std::move(token);

    else
        *this -= std::move(token);
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

mlp::Expression mlp::Expression::operator-() const {
    Expression expression;

    for (auto const &[sign, token] : this->tokens)
        if (sign == Sign::pos)
            expression -= *token;
        else
            expression += *token;

    return expression;
}

mlp::Expression &mlp::Expression::operator+=(Token const &token) {
    return *this += OwnedToken(token.clone());
}

mlp::Expression &mlp::Expression::operator-=(Token const &token) {
    return *this -= OwnedToken(token.clone());
}

mlp::Expression &mlp::Expression::operator+=(OwnedToken &&token) {
    return *this += std::move(*token);
}

mlp::Expression &mlp::Expression::operator-=(OwnedToken &&token) {
    return *this -= std::move(*token);
}

mlp::Expression &mlp::Expression::operator+=(Token &&token) {
    if (typeid(token) == typeid(Expression)) {
        for (auto &[sign, t] : dynamic_cast<Expression &>(token).tokens)
            if (sign == Sign::pos)
                *this += std::move(t);
            else
                *this -= std::move(t);

        return *this;
    }

    if (typeid(token) == typeid(Constant)) {
        auto const &constant = dynamic_cast<Constant const &>(token);

        if (constant == 0)
            return *this;

        for (std::size_t i = 0; i < this->tokens.size(); ++i) {
            auto &[sign, t] = this->tokens[i];

            if (typeid(*t) != typeid(Constant))
                continue;

            auto &c = dynamic_cast<Constant &>(*t);

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

    if (typeid(token) == typeid(Variable)) {
        auto const &variable = dynamic_cast<Variable const &>(token);

        for (std::size_t i = 0; i < this->tokens.size(); ++i) {
            auto &[sign, t] = this->tokens[i];
            if (typeid(*t) == typeid(Variable)) {
                if (auto const &v = dynamic_cast<Variable const &>(*t);
                    v != variable)
                    continue;

                if (sign == Sign::neg) {
                    this->tokens.erase(this->tokens.begin() + i);
                } else {
                    t = std::make_unique<Term>(2 * variable);
                }

                return *this;
            }

            if (typeid(*t) == typeid(Term)) {
                auto &term = dynamic_cast<Term &>(*t);

                if (typeid(*term.base) != typeid(Variable) ||
                    dynamic_cast<Variable const &>(*term.base) != variable ||
                    typeid(*term.power) != typeid(Constant) ||
                    dynamic_cast<Constant const &>(*term.power) != 1)
                    continue;

                ++term.coefficient;

                if (term.coefficient == 0)
                    this->tokens.erase(this->tokens.begin() + i);

                if (term.coefficient < 0) {
                    sign = sign == Sign::pos ? Sign::neg : Sign::pos;
                    term.coefficient = -term.coefficient;
                }

                if (term.coefficient == 1)
                    t = std::move(term.base);

                return *this;
            }
        }

        this->tokens.emplace_back(
            Sign::pos, OwnedToken(std::move(token).move())
        );

        return *this;
    }

    if (typeid(token) == typeid(Term)) {
        auto &term = dynamic_cast<Term &>(token);

        if (term.coefficient == 0)
            return *this;

        if (term.coefficient < 0) {
            term.coefficient = -term.coefficient;

            return *this -= std::move(token);
        }

        if (typeid(*term.base) != typeid(Variable) ||
            typeid(*term.power) != typeid(Constant) ||
            dynamic_cast<Constant const &>(*term.power) != 1) {
            this->tokens.emplace_back(
                Sign::pos, OwnedToken(std::move(token).move())
            );

            return *this;
        }

        auto const &variable = dynamic_cast<Variable const &>(*term.base);

        for (std::size_t i = 0; i < this->tokens.size(); ++i) {
            auto &[sign, t] = this->tokens[i];
            if (typeid(*t) == typeid(Variable)) {
                if (auto const &v = dynamic_cast<Variable const &>(*t);
                    v != variable)
                    continue;

                std::unique_ptr<Term> temp;

                if (sign == Sign::pos) {
                    temp = std::make_unique<Term>(
                        (1 + term.coefficient) * variable
                    );
                } else {
                    temp = std::make_unique<Term>(
                        (1 - term.coefficient) * variable
                    );
                }

                if (temp->coefficient == 0)
                    this->tokens.erase(this->tokens.begin() + i);

                if (temp->coefficient < 0) {
                    sign = sign == Sign::pos ? Sign::neg : Sign::pos;
                    temp->coefficient = -temp->coefficient;
                }

                if (temp->coefficient == 1)
                    t = std::move(temp->base);

                else
                    t = std::move(temp);

                return *this;
            }

            if (typeid(*t) == typeid(Term)) {
                auto &term_ = dynamic_cast<Term &>(*t);

                if (typeid(*term_.base) != typeid(Variable) ||
                    dynamic_cast<Variable const &>(*term_.base) != variable ||
                    typeid(*term_.power) != typeid(Constant) ||
                    dynamic_cast<Constant const &>(*term_.power) != 1)
                    continue;

                term_.coefficient += term.coefficient;

                if (term_.coefficient == 0)
                    this->tokens.erase(this->tokens.begin() + i);

                if (term_.coefficient < 0) {
                    sign = sign == Sign::pos ? Sign::neg : Sign::pos;
                    term_.coefficient = -term_.coefficient;
                }

                if (term_.coefficient == 1)
                    t = std::move(term_.base);

                return *this;
            }
        }
    }

    if (typeid(token) == typeid(Terms)) {
        auto &terms = dynamic_cast<Terms &>(token);

        if (terms.coefficient == 0)
            return *this;

        if (terms.coefficient < 0) {
            terms.coefficient = -terms.coefficient;

            return *this -= std::move(token);
        }
    }

    this->tokens.emplace_back(Sign::pos, OwnedToken(std::move(token).move()));

    return *this;
}

mlp::Expression &mlp::Expression::operator-=(Token &&token) {
    if (typeid(token) == typeid(Expression)) {
        for (auto &[sign, t] : dynamic_cast<Expression &>(token).tokens)
            if (sign == Sign::pos)
                *this -= std::move(t);
            else
                *this += std::move(t);

        return *this;
    }

    if (typeid(token) == typeid(Constant)) {
        auto const &constant = dynamic_cast<Constant const &>(token);

        if (constant == 0)
            return *this;

        for (std::size_t i = 0; i < this->tokens.size(); ++i) {
            auto &[sign, t] = this->tokens[i];

            if (typeid(*t) != typeid(Constant))
                continue;

            auto &c = dynamic_cast<Constant &>(*t);

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

    if (typeid(token) == typeid(Variable)) {
        auto const &variable = dynamic_cast<Variable const &>(token);

        for (std::size_t i = 0; i < this->tokens.size(); ++i) {
            auto &[sign, t] = this->tokens[i];

            if (typeid(*t) == typeid(Variable)) {
                if (auto const &v = dynamic_cast<Variable const &>(*t);
                    v != variable)
                    continue;

                if (sign == Sign::pos) {
                    this->tokens.erase(this->tokens.begin() + i);
                } else {
                    t = std::make_unique<Term>(2 * variable);
                }

                return *this;
            }

            if (typeid(*t) == typeid(Term)) {
                auto &term = dynamic_cast<Term &>(*t);

                if (typeid(*term.base) != typeid(Variable) ||
                    dynamic_cast<Variable const &>(*term.base) != variable ||
                    typeid(*term.power) != typeid(Constant) ||
                    dynamic_cast<Constant const &>(*term.power) != 1)
                    continue;

                --term.coefficient;

                if (term.coefficient == 0)
                    this->tokens.erase(this->tokens.begin() + i);

                if (term.coefficient < 0) {
                    sign = sign == Sign::pos ? Sign::neg : Sign::pos;
                    term.coefficient = -term.coefficient;
                }

                if (term.coefficient == 1)
                    t = std::move(term.base);

                return *this;
            }
        }

        this->tokens.emplace_back(
            Sign::neg, OwnedToken(std::move(token).move())
        );

        return *this;
    }

    if (typeid(token) == typeid(Term)) {
        auto &term = dynamic_cast<Term &>(token);

        if (term.coefficient == 0)
            return *this;

        if (term.coefficient < 0) {
            term.coefficient = -term.coefficient;

            return *this += std::move(token);
        }

        if (typeid(*term.base) != typeid(Variable) ||
            typeid(*term.power) != typeid(Constant) ||
            dynamic_cast<Constant const &>(*term.power) != 1) {
            this->tokens.emplace_back(
                Sign::neg, OwnedToken(std::move(token).move())
            );

            return *this;
        }

        auto const &variable = dynamic_cast<Variable const &>(*term.base);

        for (std::size_t i = 0; i < this->tokens.size(); ++i) {
            auto &[sign, t] = this->tokens[i];
            if (typeid(*t) == typeid(Variable)) {
                if (auto const &v = dynamic_cast<Variable const &>(*t);
                    v != variable)
                    continue;

                std::unique_ptr<Term> temp;

                if (sign == Sign::pos) {
                    temp = std::make_unique<Term>(
                        (1 - term.coefficient) * variable
                    );
                } else {
                    temp = std::make_unique<Term>(
                        (1 + term.coefficient) * variable
                    );
                }

                if (temp->coefficient == 0)
                    this->tokens.erase(this->tokens.begin() + i);

                if (temp->coefficient < 0) {
                    sign = sign == Sign::pos ? Sign::neg : Sign::pos;
                    temp->coefficient = -temp->coefficient;
                }

                if (temp->coefficient == 1)
                    t = std::move(temp->base);

                else
                    t = std::move(temp);

                return *this;
            }

            if (typeid(*t) == typeid(Term)) {
                auto &term_ = dynamic_cast<Term &>(*t);

                if (typeid(*term_.base) != typeid(Variable) ||
                    dynamic_cast<Variable const &>(*term_.base) != variable ||
                    typeid(*term_.power) != typeid(Constant) ||
                    dynamic_cast<Constant const &>(*term_.power) != 1)
                    continue;

                term_.coefficient -= term.coefficient;

                if (term_.coefficient == 0)
                    this->tokens.erase(this->tokens.begin() + i);

                if (term.coefficient < 0) {
                    sign = sign == Sign::pos ? Sign::neg : Sign::pos;
                    term.coefficient = -term.coefficient;
                }

                if (term.coefficient == 1)
                    t = std::move(term.base);

                return *this;
            }
        }
    }

    if (typeid(token) == typeid(Terms)) {
        auto &terms = dynamic_cast<Terms &>(token);

        if (terms.coefficient == 0)
            return *this;

        if (terms.coefficient < 0) {
            terms.coefficient = -terms.coefficient;

            return *this += std::move(token);
        }
    }

    this->tokens.emplace_back(Sign::neg, OwnedToken(std::move(token).move()));

    return *this;
}

mlp::Expression &mlp::Expression::operator*=(Token const &token) {
    for (auto &[sign, t] : this->tokens) {
        auto const temp = std::make_unique<Terms>(*t * token);

        if (temp->coefficient < 0) {
            temp->coefficient = -temp->coefficient;

            sign = sign == Sign::pos ? Sign::neg : Sign::pos;
        }

        t = OwnedToken(from_variant(simplified(*temp)).move());
    }

    return *this;
}

mlp::Expression &mlp::Expression::operator*=(Expression const &rhs) {
    Expression expression;

    for (auto &[sign, t] : this->tokens)
        expression.add_token(sign, rhs * *t);

    return *this = std::move(expression);
}

namespace mlp {
bool is_dependent_on(Expression const &token, Variable const &variable) {
    return std::ranges::any_of(
        token.tokens,
        [variable](std::pair<Sign, OwnedToken> const &term) -> bool {
            return is_dependent_on(to_variant(*term.second), variable);
        }
    );
}

bool is_linear_of(Expression const &token, Variable const &variable) {
    if (!is_dependent_on(token, variable))
        return false;

    bool is_linear = false;

    for (OwnedToken const &t : token.tokens | std::views::values) {
        if (typeid(*t) == typeid(Constant))
            continue;

        if (!is_linear_of(to_variant(*t), variable))
            return false;

        is_linear = true;
    }

    return is_linear;
}

token evaluate(
    Expression const &token, std::map<Variable, SharedToken> const &values
) {
    Expression expression{token};

    for (auto &term : expression.tokens | std::views::values)
        term =
            OwnedToken(from_variant(evaluate(to_variant(*term), values)).move()
            );

    return simplified(expression);
}

token simplified(Expression const &token) {
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

        return simplified(-std::move(*term));
    }

    Expression expression{};

    std::vector<std::pair<Sign, OwnedToken>> const &tokens = expression.tokens;

    for (auto const &[sign, t] : token.tokens)
        expression.add_token(
            sign, OwnedToken(from_variant(simplified(*t)).move())
        );

    if (tokens.empty())
        return Constant(0);

    if (tokens.size() == 1) {
        auto [sign, term] = std::move(expression.tokens.back());

        expression.tokens.pop_back();

        if (sign == Sign::pos)
            return to_variant(*term);

        return -*term;
    }

    return expression;
}

token derivative(
    Expression const &token, Variable const &variable, std::uint32_t const order
) {
    if (!order)
        return token;

    if (!is_dependent_on(token, variable))
        return Constant(0);

    Expression result{};

    for (auto const &[operation, token_] : token.tokens)
        result.add_token(
            operation,
            from_variant(derivative(to_variant(*token_), variable, 1))
        );

    auto derivative = simplified(result);

    if (order > 1)
        return simplified(mlp::derivative(derivative, variable, order - 1));

    return derivative;
}

token integral(Expression const &token, Variable const &variable) {
    if (!is_dependent_on(token, variable))
        return variable * token;

    Expression result{};

    for (auto const &[operation, term] : token.tokens)
        result.add_token(operation, from_variant(integral(*term, variable)));

    return simplified(result);
}
} // namespace mlp

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