#include "../include/terms.h"

#include "../include/constant.h"
#include "../include/expression.h"
#include "../include/function.h"
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
    return *this *= OwnedToken(token.clone());
}

mlp::Terms &mlp::Terms::operator/=(Token const &token) {
    return *this /= OwnedToken(token.clone());
}

mlp::Terms &mlp::Terms::operator*=(OwnedToken &&token) {
    return *this *= std::move(*token);
}

mlp::Terms &mlp::Terms::operator/=(OwnedToken &&token) {
    return *this /= std::move(*token);
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

                dynamic_cast<Expression &>(*t.power) += Constant(1);

                t.power = OwnedToken(from_variant(simplified(*t.power)).move());

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

                power += Constant(1);
                term.power = OwnedToken(from_variant(simplified(power)).move());
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
                term.power = OwnedToken(from_variant(simplified(power)).move());
                t = OwnedToken(std::move(token).move());

                return *this;
            }
        }

        term.power = OwnedToken(from_variant(simplified(power)).move());

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
                power -= Constant(1);

                t.power = OwnedToken(from_variant(simplified(*t.power)).move());

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

                power += Constant(1);
                term.power = OwnedToken(from_variant(simplified(power)).move());
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
                term.power = OwnedToken(from_variant(simplified(power)).move());
                t = OwnedToken(std::move(token).move());

                return *this;
            }
        }

        term.power = OwnedToken(from_variant(simplified(power)).move());

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

namespace mlp {
bool is_dependent_on(Terms const &token, Variable const &variable) {
    return std::ranges::any_of(
        token.terms,
        [variable](OwnedToken const &term) -> bool {
            return is_dependent_on(to_variant(*term), variable);
        }
    );
}

bool is_linear_of(Terms const &token, Variable const &variable) {
    if (!is_dependent_on(token, variable))
        return false;

    bool is_linear = false;

    for (OwnedToken const &t : token.terms) {
        if (!is_linear_of(to_variant(*t), variable))
            continue;

        if (is_linear)
            return false;

        is_linear = true;
    }

    return is_linear;
}

token evaluate(
    Terms const &token, std::map<Variable, SharedToken> const &values
) {
    Terms terms{token};

    for (OwnedToken &term : terms.terms)
        term =
            OwnedToken(from_variant(evaluate(to_variant(*term), values)).move()
            );

    return simplified(terms);
}

token simplified(Terms const &token) {
    if (token.coefficient == 0)
        return Constant(0);

    if (token.terms.empty())
        return Constant(token.coefficient);

    if (token.terms.size() == 1) {
        auto term = OwnedToken(token.terms[0]->clone());

        if (typeid(*term) == typeid(Constant))
            return Constant(
                token.coefficient * dynamic_cast<Constant const &>(*term)
            );

        if (typeid(*term) == typeid(Term)) {
            dynamic_cast<Term &>(*term) *= token.coefficient;

            return simplified(*term);
        }

        return simplified(token.coefficient * std::move(*term));
    }

    Terms terms{};
    terms.coefficient = token.coefficient;

    for (OwnedToken const &token_ : token.terms)
        terms *= from_variant(simplified(*token_));

    if (terms.terms.empty())
        return Constant(terms.coefficient);

    if (terms.terms.size() == 1) {
        auto &term = terms.terms[0];

        if (typeid(*term) == typeid(Constant))
            return Constant(
                terms.coefficient * dynamic_cast<Constant const &>(*term)
            );

        if (typeid(*term) == typeid(Term)) {
            dynamic_cast<Term &>(*term) *= terms.coefficient;

            return simplified(*term);
        }

        return simplified(terms.coefficient * std::move(*term));
    }

    return terms;
}

token derivative(
    Terms const &token, Variable const &variable, std::uint32_t const order
) {
    if (!order)
        return token;

    if (token.coefficient == 0 || !is_dependent_on(token, variable))
        return Constant(0);

    Expression result{};

    for (std::size_t i = 0; i < token.terms.size(); ++i) {
        auto term = std::make_unique<Terms>();

        for (std::size_t j = 0; j < token.terms.size(); ++j) {
            if (i != j) {
                *term *= OwnedToken(token.terms[j]->clone());

                continue;
            }

            Token const &derivative = from_variant(
                mlp::derivative(to_variant(*token.terms[i]), variable, 1)
            );

            if (typeid(derivative) == typeid(Constant)) {
                *term *= dynamic_cast<Constant const &>(derivative);

                continue;
            }

            *term *= derivative;
        }

        result += std::move(term);
    }

    auto derivative = simplified(token.coefficient * result);

    if (order > 1)
        return simplified(mlp::derivative(derivative, variable, order - 1));

    return derivative;
}

token integral(Terms const &token, Variable const &variable) {
    auto const simplified = mlp::simplified(token);

    if (!std::holds_alternative<Terms>(simplified))
        return integral(simplified, variable);

    auto const &t = std::get<Terms>(simplified);

    if (!is_dependent_on(t, variable))
        return variable * t;

    if (is_linear_of(t, variable)) {
        Terms terms{};
        terms.coefficient = t.coefficient;

        for (OwnedToken const &token_ : t.terms)
            terms *= is_linear_of(to_variant(*token_), variable)
                         ? OwnedToken(
                               from_variant(integral(*token_, variable)).move()
                           )
                         : OwnedToken(token_->clone());

        return mlp::simplified(terms);
    }

    throw std::runtime_error("Expression is not integrable!");
}

Terms operator*(std::double_t const lhs, Terms rhs) {
    return std::move(rhs *= lhs);
}

Terms operator*(Terms lhs, std::double_t const rhs) {
    return std::move(lhs *= rhs);
}

Terms operator/(Terms lhs, std::double_t const rhs) {
    return std::move(lhs /= rhs);
}
} // namespace mlp