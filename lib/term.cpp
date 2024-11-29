#include "../include/term.h"

#include "../include/constant.h"
#include "../include/expression.h"
#include "../include/function.h"
#include "../include/terms.h"
#include "../include/variable.h"

#include <map>
#include <sstream>

mlp::Term::Term(
    std::double_t const coefficient, OwnedToken &&base, OwnedToken &&power
)
    : coefficient(coefficient), base(std::move(base)), power(std::move(power)) {
}

mlp::Term::Term(
    std::double_t const coefficient, Token const &base, Token const &power
)
    : coefficient(coefficient), base(new Token(base)), power(new Token(power)) {
}

mlp::Term::Term(OwnedToken &&base, OwnedToken &&power)
    : base(std::move(base)), power(std::move(power)) {}

mlp::Term::Term(Token const &base, Token const &power)
    : base(new Token(base)), power(new Token(power)) {}

mlp::Term::Term(Term const &term)
    : coefficient(term.coefficient), base(new Token(*term.base)),
      power(new Token(*term.power)) {}

mlp::Term &mlp::Term::operator=(Term const &term) {
    this->coefficient = term.coefficient;
    *this->base = *term.base;
    *this->power = *term.power;

    return *this;
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

    result << to_string(*this->base);

    if (!std::holds_alternative<Constant>(*this->power) ||
        std::get<Constant>(*this->power) != 1) {
        result << '^';
        result << to_string(*this->power);
    }

    result << ')';

    return result.str();
}

mlp::Term mlp::Term::operator-() const {
    return {
        -this->coefficient, std::make_unique<Token>(*this->base),
        std::make_unique<Token>(*this->power)
    };
}

mlp::Term &mlp::Term::operator*=(std::double_t const rhs) {
    if (rhs == 0) {
        *this->base = Constant(1);
        *this->power = Constant(1);
    }

    this->coefficient *= rhs;

    return *this;
}

mlp::Term &mlp::Term::operator/=(std::double_t const rhs) {
    if (rhs == 0) {
        *this->base = Constant(1);
        *this->power = Constant(1);
    }

    this->coefficient /= rhs;

    return *this;
}

bool mlp::is_dependent_on(Term const &token, Variable const &variable) {
    return is_dependent_on(*token.base, variable) ||
           is_dependent_on(*token.power, variable);
}

bool mlp::is_linear_of(Term const &token, Variable const &variable) {
    return is_dependent_on(token, variable) &&
           std::holds_alternative<Constant>(*token.power) &&
           std::get<Constant>(*token.power) == 1 &&
           is_linear_of(*token.base, variable);
}

mlp::Token
mlp::evaluate(Term const &token, std::map<Variable, Token> const &values) {
    Term const term{token};

    *term.base = evaluate(*term.base, values);
    *term.power = evaluate(*term.power, values);

    return simplified(term);
}

mlp::Token mlp::simplified(Term const &token) {
    Term term{token};

    *term.base = simplified(*term.base);
    *term.power = simplified(*term.power);

    if (std::holds_alternative<Constant>(*term.power)) {
        auto &power = std::get<Constant>(*term.power);

        if (term.coefficient == 1 && power == 1)
            return *term.base;

        if (power == 0)
            return Constant(term.coefficient);

        if (std::holds_alternative<Term>(*term.base)) {
            auto &base = std::get<Term>(*term.base);
            base.coefficient = base.coefficient ^ power;

            if (term.coefficient != 1) {
                base.coefficient *= term.coefficient;
                term.coefficient = 1;
            }

            if (std::holds_alternative<Constant>(*base.power)) {
                std::get<Constant>(*base.power) *= power;
            } else if (std::holds_alternative<Term>(*base.power)) {
                std::get<Term>(*base.power) *= power;
            } else if (std::holds_alternative<Terms>(*base.power)) {
                std::get<Terms>(*base.power) *= power.value();
            } else {
                Terms terms{};
                terms.coefficient = power;
                terms *= *base.power;

                *base.power = std::move(terms);
            }

            power = 1;

            if (power == 1)
                return *term.base;
        }
    }

    if (std::holds_alternative<Terms>(*term.base)) {
        auto &base = std::get<Terms>(*term.base);

        if (term.coefficient != 1) {
            base.coefficient *= term.coefficient;
            term.coefficient = 1;
        }

        *term.base = simplified(base);
    }

    if (std::holds_alternative<Constant>(*term.base)) {
        auto const &base = std::get<Constant>(*term.base);

        if (base == 1)
            return Constant(token.coefficient);

        if (std::holds_alternative<Constant>(*term.power))
            return Constant(
                term.coefficient * (base ^ std::get<Constant>(*term.power))
            );

        if (term.coefficient == base) {
            if (std::holds_alternative<Expression>(*term.power)) {
                auto &power = std::get<Expression>(*term.power);
                power += Constant{1};
                term.coefficient = 1;
            }
        }
    }

    *term.base = simplified(*term.base);
    *term.power = simplified(*term.power);

    return term;
}

mlp::Token mlp::derivative(
    Term const &token, Variable const &variable, std::uint32_t const order
) {
    if (!order)
        return token;

    if (!is_dependent_on(token, variable))
        return Constant(0);

    if (std::holds_alternative<Constant>(*token.power)) {
        if (std::holds_alternative<Constant>(*token.base))
            return Constant(0);

        auto const &power = std::get<Constant>(*token.power);

        auto derivative = simplified(
            token.coefficient * power.value() *
            (*token.base ^ power.value() - 1.0) *
            mlp::derivative(*token.base, variable, 1)
        );

        if (order > 1)
            return simplified(mlp::derivative(derivative, variable, order - 1));

        return derivative;
    }

    if (std::holds_alternative<Constant>(*token.base)) {
        auto derivative = simplified(
            "ln"_f(*token.base) * mlp::derivative(*token.power, variable, 1)
        );

        if (order > 1)
            return simplified(mlp::derivative(derivative, variable, order - 1));

        return derivative;
    }

    auto derivative = simplified(
        token *
        (*token.power * mlp::derivative(*token.base, variable, 1) /
             *token.base +
         mlp::derivative(*token.power, variable, 1) * "ln"_f(*token.base))
    );

    if (order > 1)
        return simplified(mlp::derivative(derivative, variable, order - 1));

    return derivative;
}

mlp::Token mlp::integral(Term const &token, Variable const &variable) {
    if (!is_dependent_on(token, variable))
        return variable * token;

    if (std::holds_alternative<Constant>(*token.power)) {
        if (auto const &power = std::get<Constant>(*token.power); power == 1)
            return token.coefficient * integral(*token.base, variable);
    }

    if (is_linear_of(*token.base, variable)) {
        Terms terms{};
        terms *= token.coefficient;

        if (std::holds_alternative<Constant>(*token.power)) {
            if (auto const &power = std::get<Constant>(*token.power);
                power == -1) {
                terms *= "ln"_f(*token.base);
            } else {
                terms /= power.value() + 1.0;
                terms *= *token.base ^ power.value() + 1.0;
            }
        } else if (!is_dependent_on(*token.power, variable)) {
            auto expression = *token.power + Constant(1);

            terms *= *token.base ^ expression;
            terms /= std::move(expression);
        } else {
            throw std::runtime_error("Expression is not integrable!");
        }

        terms /= derivative(*token.base, variable, 1);

        return simplified(terms);
    }

    if (!is_dependent_on(*token.base, variable) &&
        is_linear_of(*token.power, variable)) {
        Terms terms{};
        terms *= Term(token);

        if (std::holds_alternative<Variable>(*token.power) &&
            std::get<Variable>(*token.power) != variable)
            terms *= Variable(variable);

        else if (std::holds_alternative<Variable>(*token.power))
            terms /= derivative(*token.power, variable, 1);

        terms /= "ln"_f(*token.base);

        return simplified(terms);
    }

    throw std::runtime_error("Expression is not integrable!");
}

namespace mlp {
Term operator-(Term const &rhs) {
    return {
        -rhs.coefficient, std::make_unique<Token>(*rhs.base),
        std::make_unique<Token>(*rhs.power)
    };
}

Term operator*(std::double_t const lhs, Term rhs) { return rhs *= lhs; }

Term operator*(Term lhs, std::double_t const rhs) { return lhs *= rhs; }

Term operator/(std::double_t const lhs, Term rhs) {
    rhs.coefficient = lhs / rhs.coefficient;

    *rhs.power = simplified(-*rhs.power);

    return rhs;
}

Term operator/(Term lhs, std::double_t const rhs) { return lhs /= rhs; }
} // namespace mlp