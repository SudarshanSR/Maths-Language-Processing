#include "../include/terms.h"

#include "../include/expression.h"
#include "../include/function.h"
#include "../include/term.h"
#include "../include/variable.h"

#include <algorithm>
#include <map>
#include <sstream>

mlp::Terms::Terms(Terms const &terms) { *this = terms; }

mlp::Terms &mlp::Terms::operator=(Terms const &terms) {
    this->coefficient = terms.coefficient;
    this->terms.clear();

    for (auto const &term : terms.terms)
        *this *= term;

    return *this;
}

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
        result << to_string(this->terms[i]);

        if (i != this->terms.size() - 1)
            result << '*';
    }

    result << ')';

    return result.str();
}

mlp::Terms mlp::Terms::operator-() const {
    Terms terms{};
    terms.coefficient = -this->coefficient;

    for (Token const &token : this->terms)
        terms *= token;

    return terms;
}

mlp::Terms &mlp::Terms::operator*=(Variable variable) {
    if (variable.coefficient != 1) {
        *this *= variable.coefficient;
        variable.coefficient = 1;
    }

    for (Token &term : this->terms) {
        if (std::holds_alternative<Variable>(term)) {
            auto const &v = std::get<Variable>(term);

            if (v != variable)
                continue;

            term = v * variable;

            return *this;
        }

        if (std::holds_alternative<Term>(term)) {
            auto &t = std::get<Term>(term);

            if (!std::holds_alternative<Variable>(*t.base))
                continue;

            auto &v = std::get<Variable>(*t.base);

            if (v.coefficient != 1) {
                *this *= pow(v.coefficient, *t.power);
                v.coefficient = 1;
            }

            if (variable != v)
                continue;

            if (!std::holds_alternative<Expression>(*t.power)) {
                Expression power{};
                power += *t.power;
                *t.power = std::move(power);
            }

            std::get<Expression>(*t.power) += 1.0;

            *t.power = simplified(*t.power);

            return *this;
        }
    }

    this->terms.emplace_back(variable);

    return *this;
}

mlp::Terms &mlp::Terms::operator*=(Function function) {
    this->terms.emplace_back(std::move(function));

    return *this;
}

mlp::Terms &mlp::Terms::operator*=(Term term) {
    if (term.coefficient != 1) {
        *this *= term.coefficient;
        term.coefficient = 1;
    }

    if (!std::holds_alternative<Variable>(*term.base)) {
        this->terms.emplace_back(std::move(term));

        return *this;
    }

    auto &variable = std::get<Variable>(*term.base);

    if (variable.coefficient != 1) {
        *this *= pow(variable.coefficient, *term.power);
        variable.coefficient = 1;
    }

    if (!std::holds_alternative<Expression>(*term.power)) {
        Expression power{};
        power += *term.power;
        *term.power = std::move(power);
    }

    auto &power = std::get<Expression>(*term.power);

    for (Token &t : this->terms) {
        if (std::holds_alternative<Variable>(t)) {
            if (auto const &v = std::get<Variable>(t); v != variable)
                continue;

            power += 1.0;
            *term.power = simplified(power);
            t = std::move(term);

            return *this;
        }

        if (std::holds_alternative<Term>(t)) {
            auto &term1 = std::get<Term>(t);

            if (!std::holds_alternative<Variable>(*term1.base))
                continue;

            auto &v = std::get<Variable>(*term1.base);

            if (v.coefficient != 1) {
                *this *= pow(v.coefficient, *term.power);
                v.coefficient = 1;
            }

            if (variable != v)
                continue;

            power += *term1.power;
            *term.power = simplified(power);
            t = std::move(term);

            return *this;
        }
    }

    *term.power = simplified(power);

    this->terms.emplace_back(std::move(term));

    return *this;
}

mlp::Terms &mlp::Terms::operator*=(Terms terms) {
    if (terms.coefficient != 1) {
        *this *= terms.coefficient;
        terms.coefficient = 1;
    }

    for (Token const &term : terms.terms)
        *this *= term;

    return *this;
}

mlp::Terms &mlp::Terms::operator*=(Expression expression) {
    this->terms.emplace_back(std::move(expression));

    return *this;
}

mlp::Terms &mlp::Terms::operator*=(Token token) {
    return std::visit(
        [this](auto &&var) -> Terms & { return *this *= var; }, token
    );
}

mlp::Terms &mlp::Terms::operator/=(Variable variable) {
    if (variable.coefficient == 0)
        throw std::domain_error{"Division by 0!"};

    if (variable.coefficient != 1) {
        *this /= variable.coefficient;
        variable.coefficient = 1;
    }

    for (std::size_t i = 0; i < this->terms.size(); ++i) {
        Token &term = this->terms[i];

        if (std::holds_alternative<Variable>(term)) {
            auto &v = std::get<Variable>(term);

            if (v.coefficient != 1) {
                *this *= v.coefficient;
                v.coefficient = 1;
            }

            if (v != variable)
                continue;

            this->terms.erase(this->terms.begin() + i);

            return *this;
        }

        if (std::holds_alternative<Term>(term)) {
            auto &t = std::get<Term>(term);

            if (!std::holds_alternative<Variable>(*t.base))
                continue;

            auto &v = std::get<Variable>(*t.base);

            if (v.coefficient != 1) {
                *this *= pow(v.coefficient, *t.power);
                v.coefficient = 1;
            }

            if (variable != v)
                continue;

            if (!std::holds_alternative<Expression>(*t.power)) {
                Expression power{};
                power += *t.power;
                *t.power = std::move(power);
            }

            auto &power = std::get<Expression>(*t.power);
            power -= 1.0;

            *t.power = simplified(*t.power);

            return *this;
        }
    }

    this->terms.emplace_back(pow(variable, -1));

    return *this;
}

mlp::Terms &mlp::Terms::operator/=(Function const &function) {
    this->terms.emplace_back(pow(function, -1));

    return *this;
}

mlp::Terms &mlp::Terms::operator/=(Term term) {
    if (term.coefficient == 0)
        throw std::domain_error{"Division by 0!"};

    if (term.coefficient != 1) {
        *this /= term.coefficient;
        term.coefficient = 1;
    }

    if (!std::holds_alternative<Variable>(*term.base)) {
        this->terms.emplace_back(pow(term, -1));

        return *this;
    }

    auto &variable = std::get<Variable>(*term.base);

    if (variable.coefficient == 0)
        throw std::domain_error{"Division by 0!"};

    if (variable.coefficient != 1) {
        *this /= pow(variable.coefficient, *term.power);
        variable.coefficient = 1;
    }

    if (!std::holds_alternative<Expression>(*term.power)) {
        Expression power{};
        power -= *term.power;
        *term.power = std::move(power);
    }

    auto &power = std::get<Expression>(*term.power);

    for (Token &t : this->terms) {
        if (std::holds_alternative<Variable>(t)) {
            auto &v = std::get<Variable>(t);

            if (v.coefficient != 1) {
                *this *= v.coefficient;
                v.coefficient = 1;
            }

            if (v != variable)
                continue;

            power += 1.0;
            *term.power = simplified(power);
            t = std::move(term);

            return *this;
        }

        if (std::holds_alternative<Term>(t)) {
            auto &term1 = std::get<Term>(t);

            if (!std::holds_alternative<Variable>(*term1.base))
                continue;

            auto &v = std::get<Variable>(*term1.base);

            if (v.coefficient != 1) {
                *this *= pow(v.coefficient, *term1.power);
                v.coefficient = 1;
            }

            if (variable != v)
                continue;

            power += *term1.power;
            *term.power = simplified(power);
            t = std::move(term);

            return *this;
        }
    }

    *term.power = simplified(power);

    this->terms.emplace_back(pow(term, -1));

    return *this;
}

mlp::Terms &mlp::Terms::operator/=(Terms terms) {
    if (terms.coefficient == 0)
        throw std::domain_error{"Division by 0!"};

    if (terms.coefficient != 1) {
        *this /= terms.coefficient;
        terms.coefficient = 1;
    }

    for (Token const &term : terms.terms)
        *this /= term;

    return *this;
}

mlp::Terms &mlp::Terms::operator/=(Expression const &expression) {
    this->terms.emplace_back(pow(expression, -1));

    return *this;
}

mlp::Terms &mlp::Terms::operator/=(Token token) {
    return std::visit(
        [this](auto &&var) -> Terms & { return *this /= var; }, token
    );
}

mlp::Terms &mlp::Terms::operator*=(Constant const scalar) {
    this->coefficient *= scalar;

    return *this;
}

mlp::Terms &mlp::Terms::operator/=(Constant const scalar) {
    this->coefficient /= scalar;

    return *this;
}

bool mlp::Terms::operator==(Terms const &) const { return false; }

mlp::Token mlp::operator+(Terms lhs, Constant const rhs) {
    if (lhs.coefficient == 0)
        return rhs;

    if (rhs == 0)
        return lhs;

    Expression result;
    result += lhs;
    result += rhs;

    return result;
}

mlp::Token mlp::operator+(Terms const &lhs, Token const &rhs) {
    Expression result;
    result += lhs;
    result += rhs;

    return result;
}

mlp::Token mlp::operator-(Terms lhs, Constant const rhs) {
    if (lhs.coefficient == 0)
        return -rhs;

    if (rhs == 0)
        return lhs;

    Expression result;
    result += lhs;
    result -= rhs;

    return result;
}

mlp::Token mlp::operator-(Terms const &lhs, Token const &rhs) {
    Expression result;
    result += lhs;
    result -= rhs;

    return result;
}

mlp::Token mlp::operator*(Terms lhs, Constant const rhs) {
    if (rhs == 0)
        return 0.0;

    if (rhs == 1)
        return lhs;

    return lhs *= rhs;
}

mlp::Token mlp::operator*(Terms lhs, Token const &rhs) { return lhs *= rhs; }

mlp::Token mlp::operator*(Terms lhs, Expression rhs) { return rhs *= lhs; }

mlp::Token mlp::operator/(Terms lhs, Constant const rhs) {
    if (rhs == 0)
        throw std::domain_error{"Division by 0!"};

    if (rhs == 1)
        return lhs;

    return lhs /= rhs;
}

mlp::Token mlp::operator/(Terms lhs, Token const &rhs) { return lhs /= rhs; }

mlp::Token mlp::pow(Constant const lhs, Terms rhs) {
    if (lhs == 0) {
        if (rhs.coefficient == 0 || rhs.coefficient == HUGE_VAL ||
            rhs.coefficient == -HUGE_VAL)
            throw std::domain_error{"Indeterminate!"};

        return 0.0;
    }

    if (lhs == 1)
        return 1.0;

    Constant const coefficient = std::pow(lhs, rhs.coefficient);
    rhs.coefficient = 1;

    return Term{coefficient, lhs, rhs};
}

namespace mlp {
Token pow(Terms lhs, Constant const rhs) {
    if (lhs.coefficient == 0) {
        if (rhs == 0 || rhs == HUGE_VAL || rhs == -HUGE_VAL)
            throw std::domain_error{"Indeterminate!"};

        return 0.0;
    }

    if (rhs == 1)
        return lhs;

    Terms terms;
    terms.coefficient = std::pow(lhs.coefficient, rhs);

    for (Token const &token : lhs.terms)
        terms *= pow(token, rhs);

    return terms;
}

Token pow(Terms const &lhs, Token const &rhs) {
    Terms terms;
    terms *= pow(lhs.coefficient, rhs);

    for (Token const &token : lhs.terms)
        terms *= pow(token, rhs);

    return terms;
}

bool is_dependent_on(Terms const &token, Variable variable) {
    return std::ranges::any_of(
        token.terms,
        [variable](Token const &term) -> bool {
            return is_dependent_on(term, variable);
        }
    );
}

bool is_linear_of(Terms const &token, Variable const variable) {
    if (!is_dependent_on(token, variable))
        return false;

    bool is_linear = false;

    for (Token const &t : token.terms) {
        if (!is_linear_of(t, variable))
            continue;

        if (is_linear)
            return false;

        is_linear = true;
    }

    return is_linear;
}

Token evaluate(Terms const &token, std::map<Variable, Token> const &values) {
    Terms terms{token};

    for (Token &term : terms.terms)
        term = evaluate(term, values);

    return simplified(terms);
}

Token simplified(Terms const &token) {
    if (token.coefficient == 0)
        return 0.0;

    if (token.terms.empty())
        return token.coefficient;

    if (token.terms.size() == 1) {
        auto term = token.terms[0];

        if (std::holds_alternative<Constant>(term))
            return token.coefficient * std::get<Constant>(term);

        if (std::holds_alternative<Term>(term)) {
            std::get<Term>(term) *= token.coefficient;

            return simplified(term);
        }

        return simplified(token.coefficient * term);
    }

    Terms terms{};
    terms.coefficient = token.coefficient;

    for (Token const &token_ : token.terms)
        terms *= simplified(token_);

    if (terms.terms.empty())
        return terms.coefficient;

    if (terms.terms.size() == 1) {
        auto term = terms.terms[0];

        if (std::holds_alternative<Constant>(term))
            return terms.coefficient * std::get<Constant>(term);

        if (std::holds_alternative<Term>(term)) {
            std::get<Term>(term) *= terms.coefficient;

            return simplified(term);
        }

        return simplified(terms.coefficient * term);
    }

    return terms;
}

Token derivative(
    Terms const &token, Variable const variable, std::uint32_t const order
) {
    if (!order)
        return token;

    if (token.coefficient == 0 || !is_dependent_on(token, variable))
        return 0.0;

    Expression result{};

    for (std::size_t i = 0; i < token.terms.size(); ++i) {
        Terms term{};

        for (std::size_t j = 0; j < token.terms.size(); ++j) {
            if (i != j) {
                term *= token.terms[j];

                continue;
            }

            term *= mlp::derivative(token.terms[i], variable, 1);
        }

        result += term;
    }

    auto derivative = simplified(token.coefficient * result);

    if (order > 1)
        return simplified(mlp::derivative(derivative, variable, order - 1));

    return derivative;
}

Token integral(Terms const &token, Variable const variable) {
    auto const simplified = mlp::simplified(token);

    if (!std::holds_alternative<Terms>(simplified))
        return integral(simplified, variable);

    auto const &t = std::get<Terms>(simplified);

    if (!is_dependent_on(t, variable))
        return variable * t;

    if (is_linear_of(t, variable)) {
        Terms terms{};
        terms.coefficient = t.coefficient;

        for (Token const &token_ : t.terms)
            terms *= is_linear_of(token_, variable) ? integral(token_, variable)
                                                    : token_;

        return mlp::simplified(terms);
    }

    throw std::runtime_error("Expression is not integrable!");
}
} // namespace mlp