#include "../include/expression.h"

#include "../include/constant.h"
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
    if (sign == Sign::pos)
        *this += std::move(token);

    else
        *this -= std::move(token);
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

mlp::Expression &mlp::Expression::operator+=(Token const &token) {
    return *this += std::move(*token.clone());
}

mlp::Expression &mlp::Expression::operator-=(Token const &token) {
    return *this -= std::move(*token.clone());
}

mlp::Expression &mlp::Expression::operator+=(OwnedToken &&token) {
    return *this += std::move(*token.release());
}

mlp::Expression &mlp::Expression::operator-=(OwnedToken &&token) {
    return *this -= std::move(*token.release());
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

                if (temp->coefficient < 0) {
                    sign = sign == Sign::pos ? Sign::neg : Sign::pos;

                    temp->coefficient = -temp->coefficient;
                }

                t = std::move(temp);

                if (term.coefficient == 0)
                    this->tokens.erase(this->tokens.begin() + i);

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

                if (temp->coefficient < 0) {
                    sign = sign == Sign::pos ? Sign::neg : Sign::pos;

                    temp->coefficient = -temp->coefficient;
                }

                t = std::move(temp);

                if (term.coefficient == 0)
                    this->tokens.erase(this->tokens.begin() + i);

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

bool mlp::Expression::is_dependent_on(Variable const &variable) const {
    return std::ranges::any_of(
        this->tokens,
        [variable](std::pair<Sign, OwnedToken> const &term) -> bool {
            return term.second->is_dependent_on(variable);
        }
    );
}

bool mlp::Expression::is_linear_of(Variable const &variable) const {
    if (!this->is_dependent_on(variable))
        return false;

    bool is_linear = false;

    for (OwnedToken const &token : this->tokens | std::views::values) {
        if (!token->is_linear_of(variable))
            continue;

        if (!token->is_linear_of(variable))
            return false;

        is_linear = true;
    }

    return is_linear;
}

mlp::OwnedToken
mlp::Expression::evaluate(std::map<Variable, SharedToken> const &values) const {
    Expression expression{*this};

    for (auto &term : expression.tokens | std::views::values)
        term = term->evaluate(values);

    return expression.simplified();
}

mlp::OwnedToken mlp::Expression::simplified() const {
    if (this->tokens.empty())
        return std::make_unique<Constant>(0);

    if (this->tokens.size() == 1) {
        Expression expression{*this};

        auto &&[sign, term] = expression.pop_token();

        if (sign == Sign::pos)
            return term->simplified();

        return (-(std::move(*term.release()) ^ 1)).simplified();
    }

    auto expression = std::make_unique<Expression>();

    std::vector<std::pair<Sign, OwnedToken>> const &tokens = expression->tokens;

    for (auto const &[sign, t] : this->tokens)
        expression->add_token(sign, t->simplified());

    if (tokens.empty())
        return std::make_unique<Constant>(0);

    if (tokens.size() == 1) {
        auto &&[sign, term] = expression->pop_token();

        if (sign == Sign::pos)
            return term->simplified();

        return (-(std::move(*term.release()) ^ 1)).simplified();
    }

    return expression;
}

mlp::OwnedToken mlp::Expression::derivative(
    Variable const &variable, std::uint32_t const order
) const {
    if (!order)
        return OwnedToken(this->clone());

    if (!this->is_dependent_on(variable))
        return std::make_unique<Constant>(0);

    Expression result{};

    for (auto const &[operation, token] : this->tokens)
        result.add_token(operation, token->derivative(variable, 1));

    auto derivative = result.simplified();

    if (order > 1)
        return derivative->derivative(variable, order - 1)->simplified();

    return derivative;
}

mlp::OwnedToken mlp::Expression::integral(Variable const &variable) {
    if (!this->is_dependent_on(variable)) {
        auto terms = std::make_unique<Terms>();
        *terms *= Variable(variable);
        *terms *= Expression(*this);

        return terms;
    }

    Expression result{};

    for (auto const &[operation, term] : this->tokens)
        result.add_token(operation, term->integral(variable));

    return result.simplified();
}

mlp::Expression mlp::operator+(Expression lhs, Token const &rhs) {
    return std::move(lhs += rhs);
}

mlp::Expression mlp::operator-(Expression lhs, Token const &rhs) {
    return std::move(lhs -= rhs);
}
