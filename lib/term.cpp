#include "../include/term.h"

#include "../include/constant.h"
#include "../include/expression.h"
#include "../include/function.h"
#include "../include/terms.h"
#include "../include/variable.h"

#include <sstream>

mlp::Term::Term(
    std::double_t const coefficient, OwnedToken &&base, OwnedToken &&power
)
    : coefficient(coefficient), base(std::move(base)), power(std::move(power)) {
}

mlp::Term::Term(std::double_t const coefficient, Token &&base, Token &&power)
    : coefficient(coefficient), base(std::move(base).move()),
      power(std::move(power).move()) {}

mlp::Term::Term(OwnedToken &&base, OwnedToken &&power)
    : base(std::move(base)), power(std::move(power)) {}

mlp::Term::Term(Token &&base, Token &&power)
    : base(std::move(base).move()), power(std::move(power).move()) {}

mlp::Term::Term(Term const &term)
    : coefficient(term.coefficient), base(term.base->clone()),
      power(term.power->clone()) {}

gsl::owner<mlp::Term *> mlp::Term::clone() const { return new Term(*this); }

gsl::owner<mlp::Term *> mlp::Term::move() && {
    return new Term(std::move(*this));
}

mlp::Term::operator std::string() const {
    std::stringstream result;

    if (this->coefficient != 1) {
        if (this->coefficient == -1)
            result << "-";

        else
            result << this->coefficient;
    }

    result << '(';

    result << *this->base;

    if (typeid(*this->power) != typeid(Constant) ||
        dynamic_cast<Constant const &>(*this->power) != 1) {
        result << '^';
        result << *this->power;
    }

    result << ')';

    return result.str();
}

mlp::Term &mlp::Term::operator*=(std::double_t const rhs) {
    this->coefficient *= rhs;

    return *this;
}

mlp::Term &mlp::Term::operator*=(Constant const &rhs) {
    this->coefficient *= rhs;

    return *this;
}

mlp::Term &mlp::Term::operator/=(std::double_t const rhs) {
    this->coefficient /= rhs;

    return *this;
}

mlp::Term &mlp::Term::operator/=(Constant const &rhs) {
    this->coefficient /= rhs;

    return *this;
}

mlp::Terms::Terms(Terms const &terms) {
    this->coefficient = terms.coefficient;

    for (auto const &term : terms.terms)
        *this *= OwnedToken(term->clone());
}

gsl::owner<mlp::Terms *> mlp::Terms::clone() const { return new Terms(*this); }

gsl::owner<mlp::Terms *> mlp::Terms::move() && {
    return new Terms(std::move(*this));
}

bool mlp::Term::is_dependent_on(Variable const &variable) const {
    return this->base->is_dependent_on(variable) ||
           this->power->is_dependent_on(variable);
}

bool mlp::Term::is_linear_of(Variable const &variable) const {
    return this->is_dependent_on(variable) &&
           typeid(*this->power) == typeid(Constant) &&
           dynamic_cast<Constant const &>(*this->power) == 1 &&
           this->base->is_linear_of(variable);
}

mlp::OwnedToken
mlp::Term::evaluate(std::map<Variable, SharedToken> const &values) const {
    Term term{*this};

    term.base = term.base->evaluate(values);
    term.power = term.power->evaluate(values);

    return term.simplified();
}

mlp::OwnedToken mlp::Term::simplified() const {
    auto term = Owned<Term>(this->clone());

    term->base = term->base->simplified();
    term->power = term->power->simplified();

    if (typeid(*term->power) == typeid(Constant)) {
        auto &power = dynamic_cast<Constant &>(*term->power);

        if (term->coefficient == 1 && power == 1)
            return std::move(term->base);

        if (power == 0)
            return std::make_unique<Constant>(term->coefficient);

        if (typeid(*term->base) == typeid(Term)) {
            auto &base = dynamic_cast<Term &>(*term->base);
            base.coefficient = base.coefficient ^ power;

            if (term->coefficient != 1) {
                base.coefficient *= term->coefficient;
                term->coefficient = 1;
            }

            if (typeid(*base.power) == typeid(Constant)) {
                dynamic_cast<Constant &>(*base.power) *= power;
            } else if (typeid(*base.power) == typeid(Term)) {
                dynamic_cast<Term &>(*base.power) *= power;
            } else if (typeid(*base.power) == typeid(Terms)) {
                dynamic_cast<Terms &>(*base.power) *= power;
            } else {
                auto terms = std::make_unique<Terms>();
                terms->coefficient = power;
                *terms *= std::move(base.power);

                base.power = std::move(terms);
            }

            power = 1;

            if (power == 1)
                return std::move(term->base);
        }
    }

    if (typeid(*term->base) == typeid(Terms)) {
        auto &base = dynamic_cast<Terms &>(*term->base);

        if (term->coefficient != 1) {
            base.coefficient *= term->coefficient;
            term->coefficient = 1;
        }
    }

    if (typeid(*term->base) == typeid(Constant)) {
        auto const &base = dynamic_cast<Constant const &>(*term->base);

        if (base == 1)
            return std::make_unique<Constant>(this->coefficient);

        if (typeid(*term->power) == typeid(Constant))
            return std::make_unique<Constant>(
                term->coefficient *
                (base ^ dynamic_cast<Constant const &>(*term->power))
            );

        if (term->coefficient == base) {
            if (typeid(*term->power) == typeid(Expression)) {
                auto &power = dynamic_cast<Expression &>(*term->power);
                power += Constant{1};
                term->coefficient = 1;
            }
        }
    }

    term->base = term->base->simplified();
    term->power = term->power->simplified();

    return term;
}

mlp::OwnedToken mlp::Term::derivative(
    Variable const &variable, std::uint32_t const order
) const {
    if (!order)
        return OwnedToken(this->clone());

    if (!this->is_dependent_on(variable))
        return std::make_unique<Constant>(0);

    auto const &base_type = typeid(*this->base);

    if (auto const &power_type = typeid(*this->power);
        power_type == typeid(Constant)) {
        if (base_type == typeid(Constant))
            return std::make_unique<Constant>(0);

        auto const &power = dynamic_cast<Constant const &>(*this->power);

        Terms terms{};
        terms *= std::make_unique<Term>(
            this->coefficient * power * (*this->base ^ power - 1.0)
        );
        terms *= this->base->derivative(variable, 1);

        auto derivative = terms.simplified();

        if (order > 1)
            return derivative->derivative(variable, order - 1)->simplified();

        return derivative;
    }

    Terms result{};

    result *= OwnedToken(this->clone());

    if (base_type == typeid(Constant)) {
        result *=
            std::make_unique<Function>("ln", OwnedToken(this->base->clone()));
        result *= this->power->derivative(variable, 1);

        auto derivative = result.simplified();

        if (order > 1)
            return derivative->derivative(variable, order - 1)->simplified();

        return derivative;
    }

    Terms terms_1{};
    terms_1 *= OwnedToken(this->power->clone());
    terms_1 *= this->base->derivative(variable, 1);
    terms_1 /= OwnedToken(this->base->clone());

    Terms terms_2{};
    terms_2 *= this->power->derivative(variable, 1);
    terms_2 *=
        std::make_unique<Function>("ln", OwnedToken(this->base->clone()));

    auto expression = std::make_unique<Expression>();
    *expression += std::move(terms_1);
    *expression += std::move(terms_2);

    result *= std::move(expression);

    auto derivative = result.simplified();

    if (order > 1)
        return derivative->derivative(variable, order - 1)->simplified();

    return derivative;
}

mlp::OwnedToken mlp::Term::integral(Variable const &variable) const {
    if (!this->is_dependent_on(variable)) {
        auto terms = std::make_unique<Terms>();
        *terms *= Variable(variable);
        *terms *= Term(*this);

        return terms;
    }

    auto const &power_type = typeid(*this->power);

    if (this->base->is_linear_of(variable)) {
        Terms terms{};
        terms *= this->coefficient;

        if (power_type == typeid(Constant)) {
            if (auto const &power =
                    dynamic_cast<Constant const &>(*this->power);
                power == -1) {
                terms *= std::make_unique<Function>(
                    "ln", OwnedToken(this->base->clone())
                );
            } else {
                terms /= power + 1.0;
                terms *= std::make_unique<Term>(
                    std::move(*this->base->clone()) ^ power + 1.0
                );
            }
        } else if (!this->power->is_dependent_on(variable)) {
            auto expression = std::make_unique<Expression>();
            *expression += OwnedToken(this->power->clone());
            *expression += Constant(1);

            terms *= std::make_unique<Term>(
                std::move(*this->base->clone()) ^
                OwnedToken(expression->clone())
            );
            terms /= std::move(expression);
        } else {
            throw std::runtime_error("Expression is not integrable!");
        }

        terms /= this->base->derivative(variable, 1)->simplified();

        return terms.simplified();
    }

    if (!this->base->is_dependent_on(variable) &&
        this->power->is_linear_of(variable)) {
        Terms terms{};
        terms *= Term(*this);

        if (power_type == typeid(Variable) &&
            dynamic_cast<Variable const &>(*this->power) != variable)
            terms *= Variable(variable);

        else if (power_type != typeid(Variable))
            terms /= this->power->derivative(variable, 1)->simplified();

        terms /=
            std::make_unique<Function>("ln", OwnedToken(this->base->clone()));

        return terms.simplified();
    }

    throw std::runtime_error("Expression is not integrable!");
}

namespace mlp {
Term operator-(Term const &rhs) {
    return {
        -rhs.coefficient, OwnedToken(rhs.base->clone()),
        OwnedToken(rhs.power->clone())
    };
}

Term operator*(std::double_t const lhs, Term rhs) { return rhs *= lhs; }

Term operator*(Term lhs, std::double_t const rhs) { return lhs *= rhs; }

Term operator/(std::double_t const lhs, Term rhs) {
    rhs /= lhs;

    Expression power{};
    power -= std::move(rhs.power);
    rhs.power = power.simplified();

    return rhs;
}

Term operator/(Term lhs, std::double_t const rhs) { return lhs /= rhs; }

Term operator*(Constant const &lhs, Term rhs) { return rhs *= lhs; }

Term operator*(Term lhs, Constant const &rhs) { return lhs *= rhs; }

Term operator/(Constant const &lhs, Term rhs) {
    rhs /= lhs;

    Expression power{};
    power -= std::move(rhs.power);
    rhs.power = power.simplified();

    return rhs;
}

Term operator/(Term lhs, Constant const &rhs) { return lhs /= rhs; }
} // namespace mlp