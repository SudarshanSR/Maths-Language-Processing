#include "../include/term.h"

#include "../include/expression.h"
#include "../include/function.h"
#include "../include/terms.h"
#include "../include/variable.h"

#include <map>
#include <sstream>
#include <utility>

mlp::Term::Term(Constant const coefficient, Token base, Token power)
    : coefficient(coefficient), base(new Token(std::move(base))),
      power(new Token(std::move(power))) {}

mlp::Term::Term(Token base, Token power)
    : base(new Token(std::move(base))), power(new Token(std::move(power))) {}

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

    if (std::holds_alternative<Constant>(*this->power)) {
        Constant const &power = std::get<Constant>(*this->power);

        if (power == 0.5)
            result << "\u221A" << *this->base;
        else if (power == 1.0 / 3.0)
            result << "\u221B" << *this->base;
        else if (power == 1.0 / 4.0)
            result << "\u221C" << *this->base;
        else if (power != 1)
            result << *this->base << '^' << *this->power;
        else
            result << *this->base;
    } else {
        result << *this->base << '^' << *this->power;
    }

    result << ')';

    return result.str();
}

mlp::Term mlp::Term::operator-() const {
    return {-this->coefficient, *this->base, *this->power};
}

mlp::Term &mlp::Term::operator*=(Constant const rhs) {
    if (rhs == 0) {
        *this->base = 1.0;
        *this->power = 1.0;
    }

    this->coefficient *= rhs;

    return *this;
}

mlp::Term &mlp::Term::operator/=(Constant const rhs) {
    if (rhs == 0) {
        *this->base = 1.0;
        *this->power = 1.0;
    }

    this->coefficient /= rhs;

    return *this;
}

bool mlp::Term::operator==(Term const &rhs) const {
    return this->coefficient == rhs.coefficient && *this->base == *rhs.base &&
           *this->power == *rhs.power;
}

bool mlp::is_dependent_on(Term const &token, Variable const variable) {
    return is_dependent_on(*token.base, variable) ||
           is_dependent_on(*token.power, variable);
}

bool mlp::is_linear_of(Term const &token, Variable const variable) {
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
            return term.coefficient;

        if (std::holds_alternative<Term>(*term.base)) {
            auto &base = std::get<Term>(*term.base);
            base.coefficient = std::pow(base.coefficient, power);

            if (term.coefficient != 1) {
                base.coefficient *= term.coefficient;
                term.coefficient = 1;
            }

            *base.power = *base.power * power;

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
            return token.coefficient;

        if (std::holds_alternative<Constant>(*term.power))
            return term.coefficient *
                   std::pow(base, std::get<Constant>(*term.power));

        if (term.coefficient == base) {
            if (std::holds_alternative<Expression>(*term.power)) {
                auto &power = std::get<Expression>(*term.power);
                power += 1.0;
                term.coefficient = 1;
            }
        }
    }

    *term.base = simplified(*term.base);
    *term.power = simplified(*term.power);

    return term;
}

mlp::Token mlp::derivative(
    Term const &token, Variable const variable, std::uint32_t const order
) {
    if (!order)
        return token;

    if (!is_dependent_on(token, variable))
        return 0.0;

    if (std::holds_alternative<Constant>(*token.power)) {
        if (std::holds_alternative<Constant>(*token.base))
            return 0.0;

        auto const &power = std::get<Constant>(*token.power);

        auto derivative = simplified(
            token.coefficient * power * pow(*token.base, power - 1.0) *
            mlp::derivative(*token.base, variable, 1)
        );

        if (order > 1)
            return simplified(mlp::derivative(derivative, variable, order - 1));

        return derivative;
    }

    if (std::holds_alternative<Constant>(*token.base)) {
        auto derivative = simplified(
            "ln"_f({*token.base}) * mlp::derivative(*token.power, variable, 1)
        );

        if (order > 1)
            return simplified(mlp::derivative(derivative, variable, order - 1));

        return derivative;
    }

    auto derivative = simplified(
        token *
        (*token.power * mlp::derivative(*token.base, variable, 1) /
             *token.base +
         mlp::derivative(*token.power, variable, 1) * "ln"_f({*token.base}))
    );

    if (order > 1)
        return simplified(mlp::derivative(derivative, variable, order - 1));

    return derivative;
}

mlp::Token mlp::integral(Term const &token, Variable variable) {
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
                terms *= "ln"_f({*token.base});
            } else {
                terms /= power + 1.0;
                terms *= pow(*token.base, power + 1.0);
            }
        } else if (!is_dependent_on(*token.power, variable)) {
            auto const expression = *token.power + 1.0;

            terms *= pow(*token.base, expression);
            terms /= expression;
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

        terms /= "ln"_f({*token.base});

        return simplified(terms);
    }

    throw std::runtime_error("Expression is not integrable!");
}

mlp::Token mlp::operator+(Term lhs, Constant const rhs) {
    if (lhs.coefficient == 0)
        return rhs;

    if (rhs == 0)
        return lhs;

    Expression result;
    result += lhs;
    result += rhs;

    return result;
}

mlp::Token mlp::operator+(Term lhs, Variable rhs) {
    if (lhs.coefficient == 0 && rhs.coefficient == 0)
        return 0.0;

    if (lhs.coefficient == 0)
        return rhs;

    if (rhs.coefficient == 0)
        return lhs;

    Expression result;
    result += lhs;
    result += rhs;

    return result;
}

mlp::Token mlp::operator+(Term const &lhs, Function rhs) {
    if (lhs.coefficient == 0)
        return rhs;

    Expression result;
    result += lhs;
    result += rhs;

    return result;
}

mlp::Token mlp::operator+(Term lhs, Term rhs) {
    if (lhs.coefficient == 0 && rhs.coefficient == 0)
        return 0.0;

    if (lhs.coefficient == 0)
        return rhs;

    if (rhs.coefficient == 0)
        return lhs;

    if (*lhs.base == *rhs.base && *lhs.power == *rhs.power) {
        lhs.coefficient += rhs.coefficient;

        return lhs;
    }

    Expression result;
    result += lhs;
    result += rhs;

    return result;
}

mlp::Token mlp::operator+(Term lhs, Terms rhs) {
    if (lhs.coefficient == 0 && rhs.coefficient == 0)
        return 0.0;

    if (lhs.coefficient == 0)
        return rhs;

    if (rhs.coefficient == 0)
        return lhs;

    Expression result;
    result += lhs;
    result += rhs;

    return result;
}

mlp::Token mlp::operator+(Term const &lhs, Expression rhs) {
    return rhs += lhs;
}

mlp::Token mlp::operator-(Term lhs, Constant const rhs) {
    if (lhs.coefficient == 0)
        return -rhs;

    if (rhs == 0)
        return lhs;

    Expression result;
    result += lhs;
    result -= rhs;

    return result;
}

mlp::Token mlp::operator-(Term lhs, Variable const rhs) {
    if (lhs.coefficient == 0 && rhs.coefficient == 0)
        return 0.0;

    if (lhs.coefficient == 0)
        return -rhs;

    if (rhs.coefficient == 0)
        return lhs;

    Expression result;
    result += lhs;
    result -= rhs;

    return result;
}

mlp::Token mlp::operator-(Term const &lhs, Function rhs) {
    if (lhs.coefficient == 0)
        return rhs;

    Expression result;
    result += lhs;
    result -= rhs;

    return result;
}

mlp::Token mlp::operator-(Term lhs, Term const &rhs) {
    if (lhs.coefficient == 0 && rhs.coefficient == 0)
        return 0.0;

    if (lhs.coefficient == 0)
        return -rhs;

    if (rhs.coefficient == 0)
        return lhs;

    if (*lhs.base == *rhs.base && *lhs.power == *rhs.power) {
        lhs.coefficient -= rhs.coefficient;

        if (lhs.coefficient == 0)
            return 0.0;

        return lhs;
    }

    Expression result;
    result += lhs;
    result -= rhs;

    return result;
}

mlp::Token mlp::operator-(Term lhs, Terms const &rhs) {
    if (lhs.coefficient == 0 && rhs.coefficient == 0)
        return 0.0;

    if (lhs.coefficient == 0)
        return -rhs;

    if (rhs.coefficient == 0)
        return lhs;

    Expression result;
    result += lhs;
    result -= rhs;

    return result;
}

mlp::Token mlp::operator-(Term const &lhs, Expression rhs) {
    return -(rhs -= lhs);
}

mlp::Token mlp::operator*(Term lhs, Constant const rhs) {
    if (rhs == 0)
        return 0.0;

    if (rhs == 1)
        return lhs;

    return lhs *= rhs;
}

mlp::Token mlp::operator*(Term lhs, Variable const rhs) {
    if (lhs.coefficient == 0 || rhs.coefficient == 0)
        return 0.0;

    if (std::holds_alternative<Variable>(*lhs.base) &&
        std::get<Variable>(*lhs.base) == rhs) {
        *lhs.power = *lhs.power + 1.0;
        lhs.coefficient *= rhs.coefficient;

        return lhs;
    }

    Terms result;
    result *= lhs;
    result *= rhs;

    return result;
}

mlp::Token mlp::operator*(Term lhs, Function const &rhs) {
    if (lhs.coefficient == 0)
        return 0.0;

    if (std::holds_alternative<Function>(*lhs.base) &&
        std::get<Function>(*lhs.base) == rhs) {
        *lhs.power = *lhs.power + 1.0;

        return lhs;
    }

    Terms result;
    result *= lhs;
    result *= rhs;

    return result;
}

mlp::Token mlp::operator*(Term lhs, Term const &rhs) {
    if (lhs.coefficient == 0 || rhs.coefficient == 0)
        return 0.0;

    if (std::holds_alternative<Term>(*lhs.base) &&
        std::get<Term>(*lhs.base) == rhs) {
        *lhs.power = *lhs.power + *rhs.power;
        lhs.coefficient *= rhs.coefficient;

        return lhs;
    }

    Terms result;
    result *= lhs;
    result *= rhs;

    return result;
}

mlp::Token mlp::operator*(Term const &lhs, Terms rhs) { return rhs *= lhs; }

mlp::Token mlp::operator*(Term lhs, Expression const &rhs) { return rhs * lhs; }

mlp::Token mlp::operator/(Term lhs, Constant const rhs) {
    if (rhs == 0)
        throw std::domain_error{"Division by 0!"};

    if (rhs == 1)
        return lhs;

    return lhs /= rhs;
}

mlp::Token mlp::operator/(Term lhs, Variable const rhs) {
    if (rhs.coefficient == 0)
        throw std::domain_error{"Division by 0!"};

    if (lhs.coefficient == 0)
        return 0.0;

    if (std::holds_alternative<Variable>(*lhs.base) &&
        std::get<Variable>(*lhs.base) == rhs) {
        *lhs.power = *lhs.power - 1.0;
        lhs.coefficient /= rhs.coefficient;

        return lhs;
    }

    Terms result;
    result *= lhs;
    result /= rhs;

    return result;
}

mlp::Token mlp::operator/(Term lhs, Function const &rhs) {
    if (lhs.coefficient == 0)
        return 0.0;

    if (std::holds_alternative<Function>(*lhs.base) &&
        std::get<Function>(*lhs.base) == rhs) {
        *lhs.power = *lhs.power - 1.0;

        return lhs;
    }

    Terms result;
    result *= lhs;
    result *= rhs;

    return result;
}

mlp::Token mlp::operator/(Term lhs, Term const &rhs) {
    if (rhs.coefficient == 0)
        throw std::domain_error{"Division by 0!"};

    if (lhs.coefficient == 0)
        return 0.0;

    if (std::holds_alternative<Term>(*lhs.base) &&
        std::get<Term>(*lhs.base) == rhs) {
        *lhs.power = *lhs.power - *rhs.power;
        lhs.coefficient /= rhs.coefficient;

        return lhs;
    }

    Terms result;
    result *= lhs;
    result *= rhs;

    return result;
}

mlp::Token mlp::operator/(Term const &lhs, Terms const &rhs) {
    return pow(rhs / lhs, -1);
}

mlp::Token mlp::operator/(Term lhs, Expression const &rhs) {
    return pow(rhs / lhs, -1);
}

mlp::Token mlp::pow(Term lhs, Constant const rhs) {
    if (lhs.coefficient == 0) {
        if (rhs == 0 || rhs == HUGE_VAL || rhs == -HUGE_VAL)
            throw std::domain_error{"Indeterminate!"};

        return 0.0;
    }

    if (rhs == 0)
        return 1.0;

    if (rhs == 1)
        return lhs;

    lhs.coefficient = std::pow(lhs.coefficient, rhs);
    *lhs.power = *lhs.power * rhs;

    return lhs;
}

mlp::Token mlp::pow(Term const &lhs, Variable rhs) {
    if (rhs.coefficient == 0 && lhs.coefficient == 0)
        throw std::domain_error{"Indeterminate!"};

    if (rhs.coefficient == 0)
        return 1.0;

    if (lhs.coefficient == 0)
        return 0.0;

    return pow(lhs.coefficient, rhs) * pow(*lhs.base, *lhs.power * rhs);
}

mlp::Token mlp::pow(Term const &lhs, Function rhs) {
    if (lhs.coefficient == 0)
        return 0.0;

    return pow(lhs.coefficient, rhs) * pow(*lhs.base, *lhs.power * rhs);
}

mlp::Token mlp::pow(Term const &lhs, Term rhs) {
    if (rhs.coefficient == 0 && lhs.coefficient == 0)
        throw std::domain_error{"Indeterminate!"};

    if (rhs.coefficient == 0)
        return 1.0;

    if (lhs.coefficient == 0)
        return 0.0;

    return pow(lhs.coefficient, rhs) * pow(*lhs.base, *lhs.power * rhs);
}

mlp::Token mlp::pow(Term const &lhs, Terms rhs) {
    if (rhs.coefficient == 0 && lhs.coefficient == 0)
        throw std::domain_error{"Indeterminate!"};

    if (rhs.coefficient == 0)
        return 1.0;

    if (lhs.coefficient == 0)
        return 0.0;

    return pow(lhs.coefficient, rhs) * pow(*lhs.base, *lhs.power * rhs);
}