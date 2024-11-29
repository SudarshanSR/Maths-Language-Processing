#include "../include/terms.h"

#include "../include/constant.h"
#include "../include/expression.h"
#include "../include/function.h"
#include "../include/term.h"
#include "../include/variable.h"

#include <map>
#include <sstream>

mlp::Terms::Terms(Terms const &terms) { *this = terms; }

mlp::Terms &mlp::Terms::operator=(Terms const &terms) {
    this->coefficient = terms.coefficient;
    this->terms.clear();

    for (auto const &term : terms.terms)
        *this *= *term;

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
        result << to_string(*this->terms[i]);

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
        terms *= *token;

    return terms;
}

mlp::Terms &mlp::Terms::operator*=(Token const &token) {
    return *this *= std::make_unique<Token>(token);
}

mlp::Terms &mlp::Terms::operator/=(Token const &token) {
    return *this /= std::make_unique<Token>(token);
}

mlp::Terms &mlp::Terms::operator*=(OwnedToken &&token) {
    return *this *= std::move(*token);
}

mlp::Terms &mlp::Terms::operator/=(OwnedToken &&token) {
    return *this /= std::move(*token);
}

mlp::Terms &mlp::Terms::operator*=(Token &&token) {
    if (std::holds_alternative<Constant>(token)) {
        this->coefficient *= std::get<Constant>(token);

        return *this;
    }

    if (std::holds_alternative<Variable>(token)) {
        auto const &variable = std::get<Variable>(token);

        for (OwnedToken &term : this->terms) {
            if (std::holds_alternative<Variable>(*term)) {
                if (auto const &v = std::get<Variable>(*term); v != variable)
                    continue;

                *term = variable ^ 2;

                return *this;
            }

            if (std::holds_alternative<Term>(*term)) {
                auto &t = std::get<Term>(*term);

                if (!std::holds_alternative<Variable>(*t.base))
                    continue;

                if (auto const &v = std::get<Variable>(*t.base); variable != v)
                    continue;

                if (!std::holds_alternative<Expression>(*t.power)) {
                    Expression power{};
                    power += std::move(t.power);
                    *t.power = std::move(power);
                }

                std::get<Expression>(*t.power) += Constant(1);

                *t.power = simplified(*t.power);

                return *this;
            }
        }

        this->terms.emplace_back(new Token(std::move(token)));

        return *this;
    }

    if (std::holds_alternative<Term>(token)) {
        auto &term = std::get<Term>(token);

        if (term.coefficient != 1) {
            *this *= term.coefficient;
            term.coefficient = 1;
        }

        if (!std::holds_alternative<Variable>(*term.base)) {
            this->terms.emplace_back(new Token(std::move(token)));

            return *this;
        }

        auto const &variable = std::get<Variable>(*term.base);

        if (!std::holds_alternative<Expression>(*term.power)) {
            Expression power{};
            power += std::move(term.power);
            *term.power = std::move(power);
        }

        auto &power = std::get<Expression>(*term.power);

        for (OwnedToken &t : this->terms) {
            if (std::holds_alternative<Variable>(*t)) {
                if (auto const &v = std::get<Variable>(*t); v != variable)
                    continue;

                power += Constant(1);
                *term.power = simplified(power);
                *t = std::move(token);

                return *this;
            }

            if (std::holds_alternative<Term>(*t)) {
                auto &term1 = std::get<Term>(*t);

                if (!std::holds_alternative<Variable>(*term1.base))
                    continue;

                if (auto const &v = std::get<Variable>(*term1.base);
                    variable != v)
                    continue;

                power += std::move(term1.power);
                *term.power = simplified(power);
                *t = std::move(token);

                return *this;
            }
        }

        *term.power = simplified(power);

        this->terms.emplace_back(new Token(std::move(token)));

        return *this;
    }

    if (std::holds_alternative<Terms>(token)) {
        auto &terms = std::get<Terms>(token);

        if (terms.coefficient != 1) {
            *this *= terms.coefficient;
            terms.coefficient = 1;
        }

        for (OwnedToken &term : terms.terms)
            *this *= std::move(term);

        return *this;
    }

    this->terms.emplace_back(new Token(std::move(token)));

    return *this;
}

mlp::Terms &mlp::Terms::operator/=(Token &&token) {
    if (std::holds_alternative<Constant>(token)) {
        this->coefficient /= std::get<Constant>(token);

        return *this;
    }

    if (std::holds_alternative<Variable>(token)) {
        auto const &variable = std::get<Variable>(token);

        for (std::size_t i = 0; i < this->terms.size(); ++i) {
            OwnedToken const &term = this->terms[i];

            if (std::holds_alternative<Variable>(*term)) {
                if (auto const &v = std::get<Variable>(*term); v != variable)
                    continue;

                this->terms.erase(this->terms.begin() + i);

                return *this;
            }

            if (std::holds_alternative<Term>(*term)) {
                auto &t = std::get<Term>(*term);

                if (!std::holds_alternative<Variable>(*t.base))
                    continue;

                if (auto const &v = std::get<Variable>(*t.base); variable != v)
                    continue;

                if (!std::holds_alternative<Expression>(*t.power)) {
                    Expression power{};
                    power += std::move(t.power);
                    *t.power = std::move(power);
                }

                auto &power = std::get<Expression>(*t.power);
                power -= Constant(1);

                *t.power = simplified(*t.power);

                return *this;
            }
        }

        this->terms.emplace_back(new Token(std::move(token) ^ -1));

        return *this;
    }

    if (std::holds_alternative<Term>(token)) {
        auto &term = std::get<Term>(token);

        if (term.coefficient != 1) {
            *this /= term.coefficient;
            term.coefficient = 1;
        }

        if (!std::holds_alternative<Variable>(*term.base)) {
            this->terms.emplace_back(new Token(std::move(token) ^ -1));

            return *this;
        }

        auto const &variable = std::get<Variable>(*term.base);

        if (!std::holds_alternative<Expression>(*term.power)) {
            Expression power{};
            power -= std::move(term.power);
            *term.power = std::move(power);
        }

        auto &power = std::get<Expression>(*term.power);

        for (OwnedToken &t : this->terms) {
            if (std::holds_alternative<Variable>(*t)) {
                if (auto const &v = std::get<Variable>(*t); v != variable)
                    continue;

                power += Constant(1);
                *term.power = simplified(power);
                *t = std::move(token);

                return *this;
            }

            if (std::holds_alternative<Term>(*t)) {
                auto &term1 = std::get<Term>(*t);

                if (!std::holds_alternative<Variable>(*term1.base))
                    continue;

                if (auto const &v = std::get<Variable>(*term1.base);
                    variable != v)
                    continue;

                power += std::move(term1.power);
                *term.power = simplified(power);
                *t = std::move(token);

                return *this;
            }
        }

        *term.power = simplified(power);

        this->terms.emplace_back(new Token(std::move(token) ^ -1));

        return *this;
    }

    if (std::holds_alternative<Terms>(token)) {
        auto &terms = std::get<Terms>(token);

        if (terms.coefficient != 1) {
            *this /= terms.coefficient;
            terms.coefficient = 1;
        }

        for (OwnedToken &term : terms.terms)
            *this /= std::move(term);

        return *this;
    }

    this->terms.emplace_back(new Token(std::move(token) ^ -1));

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
            return is_dependent_on(*term, variable);
        }
    );
}

bool is_linear_of(Terms const &token, Variable const &variable) {
    if (!is_dependent_on(token, variable))
        return false;

    bool is_linear = false;

    for (OwnedToken const &t : token.terms) {
        if (!is_linear_of(*t, variable))
            continue;

        if (is_linear)
            return false;

        is_linear = true;
    }

    return is_linear;
}

Token evaluate(Terms const &token, std::map<Variable, Token> const &values) {
    Terms terms{token};

    for (OwnedToken &term : terms.terms)
        *term = evaluate(*term, values);

    return simplified(terms);
}

Token simplified(Terms const &token) {
    if (token.coefficient == 0)
        return Constant(0);

    if (token.terms.empty())
        return Constant(token.coefficient);

    if (token.terms.size() == 1) {
        auto term = *token.terms[0];

        if (std::holds_alternative<Constant>(term))
            return Constant(token.coefficient * std::get<Constant>(term));

        if (std::holds_alternative<Term>(term)) {
            std::get<Term>(term) *= token.coefficient;

            return simplified(term);
        }

        return simplified(token.coefficient * std::move(term));
    }

    Terms terms{};
    terms.coefficient = token.coefficient;

    for (OwnedToken const &token_ : token.terms)
        terms *= simplified(*token_);

    if (terms.terms.empty())
        return Constant(terms.coefficient);

    if (terms.terms.size() == 1) {
        auto term = *terms.terms[0];

        if (std::holds_alternative<Constant>(term))
            return Constant(terms.coefficient * std::get<Constant>(term));

        if (std::holds_alternative<Term>(term)) {
            std::get<Term>(term) *= terms.coefficient;

            return simplified(term);
        }

        return simplified(terms.coefficient * std::move(term));
    }

    return terms;
}

Token derivative(
    Terms const &token, Variable const &variable, std::uint32_t const order
) {
    if (!order)
        return token;

    if (token.coefficient == 0 || !is_dependent_on(token, variable))
        return Constant(0);

    Expression result{};

    for (std::size_t i = 0; i < token.terms.size(); ++i) {
        Terms term{};

        for (std::size_t j = 0; j < token.terms.size(); ++j) {
            if (i != j) {
                term *= *token.terms[j];

                continue;
            }

            Token const &derivative =
                mlp::derivative(*token.terms[i], variable, 1);

            if (std::holds_alternative<Constant>(derivative)) {
                term *= std::get<Constant>(derivative).value();

                continue;
            }

            term *= derivative;
        }

        result += std::move(term);
    }

    auto derivative = simplified(token.coefficient * result);

    if (order > 1)
        return simplified(mlp::derivative(derivative, variable, order - 1));

    return derivative;
}

Token integral(Terms const &token, Variable const &variable) {
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
            terms *= is_linear_of(*token_, variable)
                         ? integral(*token_, variable)
                         : *token_;

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