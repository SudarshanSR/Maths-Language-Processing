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

mlp::Term mlp::Term::operator-() const {
    return {
        -this->coefficient, OwnedToken(this->base->clone()),
        OwnedToken(this->power->clone())
    };
}

mlp::Term &mlp::Term::operator*=(std::double_t const rhs) {
    if (rhs == 0) {
        this->base = std::make_unique<Constant>(1);
        this->power = std::make_unique<Constant>(1);
    }

    this->coefficient *= rhs;

    return *this;
}

mlp::Term &mlp::Term::operator/=(std::double_t const rhs) {
    if (rhs == 0) {
        this->base = std::make_unique<Constant>(1);
        this->power = std::make_unique<Constant>(1);
    }

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

bool mlp::is_dependent_on(Term const &token, Variable const &variable) {
    return is_dependent_on(to_variant(*token.base), variable) ||
           is_dependent_on(to_variant(*token.power), variable);
}

bool mlp::is_linear_of(Term const &token, Variable const &variable) {
    return is_dependent_on(token, variable) &&
           typeid(*token.power) == typeid(Constant) &&
           dynamic_cast<Constant const &>(*token.power) == 1 &&
           is_linear_of(to_variant(*token.base), variable);
}

mlp::token mlp::evaluate(
    Term const &token, std::map<Variable, SharedToken> const &values
) {
    Term term{token};

    term.base =
        OwnedToken(from_variant(evaluate(to_variant(*term.base), values)).move()
        );
    term.power = OwnedToken(
        from_variant(evaluate(to_variant(*term.power), values)).move()
    );

    return simplified(term);
}

mlp::token mlp::simplified(Term const &token) {
    Term term{token};

    term.base = OwnedToken(from_variant(simplified(*term.base)).move());
    term.power = OwnedToken(from_variant(simplified(*term.power)).move());

    if (typeid(*term.power) == typeid(Constant)) {
        auto &power = dynamic_cast<Constant &>(*term.power);

        if (term.coefficient == 1 && power == 1)
            return to_variant(*term.base);

        if (power == 0)
            return Constant(term.coefficient);

        if (typeid(*term.base) == typeid(Term)) {
            auto &base = dynamic_cast<Term &>(*term.base);
            base.coefficient = base.coefficient ^ power;

            if (term.coefficient != 1) {
                base.coefficient *= term.coefficient;
                term.coefficient = 1;
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
                return to_variant(*term.base);
        }
    }

    if (typeid(*term.base) == typeid(Terms)) {
        auto &base = dynamic_cast<Terms &>(*term.base);

        if (term.coefficient != 1) {
            base.coefficient *= term.coefficient;
            term.coefficient = 1;
        }
    }

    if (typeid(*term.base) == typeid(Constant)) {
        auto const &base = dynamic_cast<Constant const &>(*term.base);

        if (base == 1)
            return Constant(token.coefficient);

        if (typeid(*term.power) == typeid(Constant))
            return Constant(
                term.coefficient *
                (base ^ dynamic_cast<Constant const &>(*term.power))
            );

        if (term.coefficient == base) {
            if (typeid(*term.power) == typeid(Expression)) {
                auto &power = dynamic_cast<Expression &>(*term.power);
                power += Constant{1};
                term.coefficient = 1;
            }
        }
    }

    term.base = OwnedToken(from_variant(simplified(*term.base)).move());
    term.power = OwnedToken(from_variant(simplified(*term.power)).move());

    return term;
}

mlp::token mlp::derivative(
    Term const &token, Variable const &variable, std::uint32_t const order
) {
    if (!order)
        return token;

    if (!is_dependent_on(token, variable))
        return Constant(0);

    auto const &base_type = typeid(*token.base);

    if (auto const &power_type = typeid(*token.power);
        power_type == typeid(Constant)) {
        if (base_type == typeid(Constant))
            return Constant(0);

        auto const &power = dynamic_cast<Constant const &>(*token.power);

        auto derivative = simplified(
            token.coefficient * power.value() * (*token.base ^ power - 1.0) *
            from_variant(mlp::derivative(to_variant(*token.base), variable, 1))
        );

        if (order > 1)
            return simplified(mlp::derivative(derivative, variable, order - 1));

        return derivative;
    }

    if (base_type == typeid(Constant)) {
        auto derivative = simplified(
            "ln"_f(*token.base) *
            from_variant(mlp::derivative(to_variant(*token.power), variable, 1))
        );

        if (order > 1)
            return simplified(mlp::derivative(derivative, variable, order - 1));

        return derivative;
    }

    auto derivative = simplified(
        token *
        (*token.power *
             from_variant(
                 mlp::derivative(to_variant(*token.base), variable, 1)
             ) /
             *token.base +
         from_variant(mlp::derivative(to_variant(*token.power), variable, 1)) *
             "ln"_f(*token.base))
    );

    if (order > 1)
        return simplified(mlp::derivative(derivative, variable, order - 1));

    return derivative;
}

mlp::token mlp::integral(Term const &token, Variable const &variable) {
    if (!is_dependent_on(token, variable))
        return variable * token;

    auto const &power_type = typeid(*token.power);

    if (power_type == typeid(Constant)) {
        if (auto const &power = dynamic_cast<Constant const &>(*token.power);
            power == 1)
            return token.coefficient *
                   from_variant(integral(*token.base, variable));
    }

    if (is_linear_of(to_variant(*token.base), variable)) {
        Terms terms{};
        terms *= token.coefficient;

        if (power_type == typeid(Constant)) {
            if (auto const &power =
                    dynamic_cast<Constant const &>(*token.power);
                power == -1) {
                terms *= std::make_unique<Function>(
                    "ln", OwnedToken(token.base->clone())
                );
            } else {
                terms /= power + 1.0;
                terms *= std::make_unique<Term>(
                    std::move(*token.base->clone()) ^ power + 1.0
                );
            }
        } else if (!is_dependent_on(to_variant(*token.power), variable)) {
            auto expression = *token.power + Constant(1);

            terms *= std::make_unique<Term>(
                std::move(*token.base->clone()) ^ expression
            );
            terms /= std::move(expression);
        } else {
            throw std::runtime_error("Expression is not integrable!");
        }

        terms /= from_variant(derivative(to_variant(*token.base), variable, 1));

        return simplified(terms);
    }

    if (!is_dependent_on(to_variant(*token.base), variable) &&
        is_linear_of(to_variant(*token.power), variable)) {
        Terms terms{};
        terms *= Term(token);

        if (power_type == typeid(Variable) &&
            dynamic_cast<Variable const &>(*token.power) != variable)
            terms *= Variable(variable);

        else if (power_type != typeid(Variable))
            terms /=
                from_variant(derivative(to_variant(*token.power), variable, 1));

        terms /=
            std::make_unique<Function>("ln", OwnedToken(token.base->clone()));

        return simplified(terms);
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
    rhs.coefficient = lhs / rhs.coefficient;

    rhs.power = OwnedToken(from_variant(simplified(-*rhs.power)).move());

    return rhs;
}

Term operator/(Term lhs, std::double_t const rhs) { return lhs /= rhs; }
} // namespace mlp