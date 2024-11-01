#include "../include/terms.h"

#include "../include/constant.h"
#include "../include/expression.h"
#include "../include/term.h"
#include "../include/variable.h"

#include <sstream>

mlp::Terms::operator std::string() const {
    std::stringstream result;

    if (this->coefficient != 1) {
        if (this->coefficient == -1)
            result << "-";

        else
            result << this->coefficient;
    }

    result << '(';

    for (std::size_t i = 0; i < this->terms.size(); ++i) {
        result << static_cast<std::string>(*this->terms[i]);

        if (i != this->terms.size() - 1)
            result << '*';
    }

    result << ')';

    return result.str();
}

mlp::Terms mlp::Terms::operator-() const {
    Terms terms{};
    terms.coefficient = -this->coefficient;

    for (OwnedToken const &token : this->terms)
        terms *= OwnedToken(token->clone());

    return terms;
}

mlp::Terms &mlp::Terms::operator*=(Token const &token) {
    return *this *= std::move(*token.clone());
}

mlp::Terms &mlp::Terms::operator/=(Token const &token) {
    return *this /= std::move(*token.clone());
}

mlp::Terms &mlp::Terms::operator*=(OwnedToken &&token) {
    return *this *= std::move(*token.release());
}

mlp::Terms &mlp::Terms::operator/=(OwnedToken &&token) {
    return *this /= std::move(*token.release());
}

mlp::Terms &mlp::Terms::operator*=(Token &&token) {
    if (typeid(token) == typeid(Constant)) {
        this->coefficient *= dynamic_cast<Constant const &>(token);

        return *this;
    }

    if (typeid(token) == typeid(Variable)) {
        auto const &variable = dynamic_cast<Variable const &>(token);

        for (OwnedToken &term : this->terms) {
            if (typeid(*term) == typeid(Variable)) {
                if (auto const &v = dynamic_cast<Variable const &>(*term);
                    v != variable)
                    continue;

                term = std::make_unique<Term>(variable ^ 2);

                return *this;
            }

            if (typeid(*term) == typeid(Term)) {
                auto &t = dynamic_cast<Term &>(*term);

                if (typeid(*t.base) != typeid(Variable))
                    continue;

                if (auto const &v = dynamic_cast<Variable const &>(*t.base);
                    variable != v)
                    continue;

                if (typeid(*t.power) != typeid(Expression)) {
                    auto power = std::make_unique<Expression>();
                    *power += std::move(t.power);
                    t.power = std::move(power);
                }

                dynamic_cast<Expression &>(*t.power) +=
                    std::make_unique<Constant>(1);

                t.power = t.power->simplified();

                return *this;
            }
        }

        this->terms.emplace_back(std::move(token).move());

        return *this;
    }

    if (typeid(token) == typeid(Term)) {
        auto &term = dynamic_cast<Term &>(token);

        if (term.coefficient != 1) {
            *this *= term.coefficient;
            term.coefficient = 1;
        }

        if (typeid(*term.base) != typeid(Variable)) {
            this->terms.emplace_back(std::move(token).move());

            return *this;
        }

        auto const &variable = dynamic_cast<Variable const &>(*term.base);

        if (typeid(*term.power) != typeid(Expression)) {
            auto power = std::make_unique<Expression>();
            *power += std::move(term.power);
            term.power = std::move(power);
        }

        auto &power = dynamic_cast<Expression &>(*term.power);

        for (OwnedToken &t : this->terms) {
            if (typeid(*t) == typeid(Variable)) {
                if (auto const &v = dynamic_cast<Variable const &>(*t);
                    v != variable)
                    continue;

                power += std::make_unique<Constant>(1);
                term.power = power.simplified();
                t = OwnedToken(std::move(token).move());

                return *this;
            }

            if (typeid(*t) == typeid(Term)) {
                auto &term1 = dynamic_cast<Term &>(*t);

                if (typeid(*term1.base) != typeid(Variable))
                    continue;

                if (auto const &v = dynamic_cast<Variable const &>(*term1.base);
                    variable != v)
                    continue;

                power += std::move(term1.power);
                term.power = power.simplified();
                t = OwnedToken(std::move(token).move());

                return *this;
            }
        }

        term.power = power.simplified();

        this->terms.emplace_back(std::move(token).move());

        return *this;
    }

    if (typeid(token) == typeid(Terms)) {
        auto &terms = dynamic_cast<Terms &>(token);

        if (terms.coefficient != 1) {
            *this *= terms.coefficient;
            terms.coefficient = 1;
        }

        for (OwnedToken &term : terms.terms)
            *this *= std::move(term);

        return *this;
    }

    this->terms.emplace_back(std::move(token).move());

    return *this;
}

mlp::Terms &mlp::Terms::operator/=(Token &&token) {
    if (typeid(token) == typeid(Constant)) {
        this->coefficient /= dynamic_cast<Constant const &>(token);

        return *this;
    }

    if (typeid(token) == typeid(Variable)) {
        auto const &variable = dynamic_cast<Variable const &>(token);

        for (std::size_t i = 0; i < this->terms.size(); ++i) {
            OwnedToken const &term = this->terms[i];

            if (typeid(*term) == typeid(Variable)) {
                if (auto const &v = dynamic_cast<Variable const &>(*term);
                    v != variable)
                    continue;

                this->terms.erase(this->terms.begin() + i);

                return *this;
            }

            if (typeid(*term) == typeid(Term)) {
                auto &t = dynamic_cast<Term &>(*term);

                if (typeid(*t.base) != typeid(Variable))
                    continue;

                if (auto const &v = dynamic_cast<Variable const &>(*t.base);
                    variable != v)
                    continue;

                if (typeid(*t.power) != typeid(Expression)) {
                    auto power = std::make_unique<Expression>();
                    *power += std::move(t.power);
                    t.power = std::move(power);
                }

                auto &power = dynamic_cast<Expression &>(*t.power);
                power -= std::make_unique<Constant>(1);

                t.power = t.power->simplified();

                return *this;
            }
        }

        this->terms.push_back(std::make_unique<Term>(std::move(token) ^ -1));

        return *this;
    }

    if (typeid(token) == typeid(Term)) {
        auto &term = dynamic_cast<Term &>(token);

        if (term.coefficient != 1) {
            *this /= term.coefficient;
            term.coefficient = 1;
        }

        if (typeid(*term.base) != typeid(Variable)) {
            this->terms.push_back(
                std::make_unique<Term>(std::move(token) ^ -1)
            );

            return *this;
        }

        auto const &variable = dynamic_cast<Variable const &>(*term.base);

        if (typeid(*term.power) != typeid(Expression)) {
            auto power = std::make_unique<Expression>();
            *power -= std::move(term.power);
            term.power = std::move(power);
        }

        auto &power = dynamic_cast<Expression &>(*term.power);

        for (OwnedToken &t : this->terms) {
            if (typeid(*t) == typeid(Variable)) {
                if (auto const &v = dynamic_cast<Variable const &>(*t);
                    v != variable)
                    continue;

                power += std::make_unique<Constant>(1);
                term.power = power.simplified();
                t = OwnedToken(std::move(token).move());

                return *this;
            }

            if (typeid(*t) == typeid(Term)) {
                auto &term1 = dynamic_cast<Term &>(*t);

                if (typeid(*term1.base) != typeid(Variable))
                    continue;

                if (auto const &v = dynamic_cast<Variable const &>(*term1.base);
                    variable != v)
                    continue;

                power += std::move(term1.power);
                term.power = power.simplified();
                t = OwnedToken(std::move(token).move());

                return *this;
            }
        }

        term.power = power.simplified();

        this->terms.push_back(std::make_unique<Term>(std::move(token) ^ -1));

        return *this;
    }

    if (typeid(token) == typeid(Terms)) {
        auto &terms = dynamic_cast<Terms &>(token);

        if (terms.coefficient != 1) {
            *this /= terms.coefficient;
            terms.coefficient = 1;
        }

        for (OwnedToken &term : terms.terms)
            *this /= std::move(term);

        return *this;
    }

    this->terms.push_back(std::make_unique<Term>(std::move(token) ^ -1));

    return *this;
}

mlp::Terms &mlp::Terms::operator*=(std::double_t const scalar) {
    this->coefficient *= scalar;

    return *this;
}

mlp::Terms &mlp::Terms::operator/=(std::double_t const scalar) {
    this->coefficient /= scalar;

    return *this;
}

mlp::Terms &mlp::Terms::operator*=(Constant const &rhs) {
    this->coefficient *= rhs;

    return *this;
}

mlp::Terms &mlp::Terms::operator/=(Constant const &rhs) {
    this->coefficient /= rhs;

    return *this;
}

bool mlp::Terms::is_dependent_on(Variable const &variable) const {
    return std::ranges::any_of(
        this->terms,
        [variable](OwnedToken const &term) -> bool {
            return term->is_dependent_on(variable);
        }
    );
}

bool mlp::Terms::is_linear_of(Variable const &variable) const {
    if (!this->is_dependent_on(variable))
        return false;

    bool is_linear = false;

    for (OwnedToken const &token : this->terms) {
        if (!token->is_linear_of(variable))
            continue;

        if (is_linear)
            return false;

        is_linear = true;
    }

    return is_linear;
}

mlp::OwnedToken
mlp::Terms::evaluate(std::map<Variable, SharedToken> const &values) const {
    Terms terms{*this};

    for (OwnedToken &term : terms.terms)
        term = term->evaluate(values);

    return terms.simplified();
}

mlp::OwnedToken mlp::Terms::simplified() const {
    if (this->coefficient == 0)
        return std::make_unique<Constant>(0);

    if (this->terms.empty())
        return std::make_unique<Constant>(this->coefficient);

    if (this->terms.size() == 1) {
        auto term = OwnedToken(this->terms[0]->clone());

        if (typeid(*term) == typeid(Constant))
            return std::make_unique<Constant>(
                this->coefficient * dynamic_cast<Constant const &>(*term)
            );

        if (typeid(*term) == typeid(Term)) {
            dynamic_cast<Term &>(*term) *= this->coefficient;

            return term->simplified();
        }

        return (this->coefficient * std::move(*term.release())).simplified();
    }

    auto terms = std::make_unique<Terms>();
    terms->coefficient = this->coefficient;

    for (OwnedToken const &token : this->terms)
        *terms *= token->simplified();

    if (terms->terms.empty())
        return std::make_unique<Constant>(terms->coefficient);

    if (terms->terms.size() == 1) {
        auto &term = terms->terms[0];

        if (typeid(*term) == typeid(Constant))
            return std::make_unique<Constant>(
                terms->coefficient * dynamic_cast<Constant const &>(*term)
            );

        if (typeid(*term) == typeid(Term)) {
            dynamic_cast<Term &>(*term) *= terms->coefficient;

            return term->simplified();
        }

        return (terms->coefficient * std::move(*term.release())).simplified();
    }

    return terms;
}

mlp::OwnedToken mlp::Terms::derivative(
    Variable const &variable, std::uint32_t const order
) const {
    if (!order)
        return OwnedToken(this->clone());

    if (this->coefficient == 0 || !this->is_dependent_on(variable))
        return std::make_unique<Constant>(0);

    auto const result = std::make_unique<Expression>();

    for (std::size_t i = 0; i < this->terms.size(); ++i) {
        auto term = std::make_unique<Terms>();

        for (std::size_t j = 0; j < this->terms.size(); ++j) {
            if (i != j) {
                *term *= OwnedToken(this->terms[j]->clone());

                continue;
            }

            auto derivative = this->terms[i]->derivative(variable, 1);

            if (typeid(*derivative) == typeid(Constant)) {
                *term *= dynamic_cast<Constant const &>(*derivative);

                continue;
            }

            *term *= std::move(derivative);
        }

        *result += std::move(term);
    }

    Term const end = this->coefficient * *result;

    auto derivative = end.simplified();

    if (order > 1)
        return derivative->derivative(variable, order - 1)->simplified();

    return derivative;
}

mlp::OwnedToken mlp::Terms::integral(Variable const &variable) {
    if (!this->is_dependent_on(variable)) {
        auto terms = std::make_unique<Terms>();
        *terms *= Variable(variable);
        *terms *= Terms(*this);

        return terms;
    }

    if (this->is_linear_of(variable)) {
        Terms terms{};
        terms.coefficient = this->coefficient;

        for (OwnedToken const &token : this->terms)
            terms *= token->is_linear_of(variable)
                         ? token->integral(variable)
                         : Owned<Token>(token->clone());

        return terms.simplified();
    }

    throw std::runtime_error("Expression is not integrable!");
}

namespace mlp {
Terms operator*(std::double_t const lhs, Terms rhs) {
    return std::move(rhs *= lhs);
}

Terms operator*(Terms lhs, std::double_t const rhs) {
    return std::move(lhs *= rhs);
}

Terms operator/(Terms lhs, std::double_t const rhs) {
    return std::move(lhs /= rhs);
}

Terms operator*(Constant const &lhs, Terms rhs) {
    return std::move(rhs *= lhs);
}

Terms operator*(Terms lhs, Constant const &rhs) {
    return std::move(lhs *= rhs);
}

Terms operator/(Terms lhs, Constant const &rhs) {
    return std::move(lhs /= rhs);
}
} // namespace mlp