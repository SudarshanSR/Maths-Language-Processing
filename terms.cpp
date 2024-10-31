#include "token.h"

#include <ranges>
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

mlp::Terms &mlp::Terms::operator*=(OwnedToken &&token) {
    if (typeid(*token) == typeid(Constant)) {
        this->coefficient *= dynamic_cast<Constant const &>(*token);

        return *this;
    }

    if (typeid(*token) == typeid(Variable)) {
        auto const &variable = dynamic_cast<Variable const &>(*token);

        for (auto const &[i, term] : this->terms | std::views::enumerate) {
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

        this->terms.push_back(std::move(token));

        return *this;
    }

    if (typeid(*token) == typeid(Term)) {
        auto &term = dynamic_cast<Term &>(*token);

        if (term.coefficient != 1) {
            *this *= term.coefficient;
            term.coefficient = 1;
        }

        if (typeid(*term.base) != typeid(Variable)) {
            this->terms.push_back(std::move(token));

            return *this;
        }

        auto const &variable = dynamic_cast<Variable const &>(*term.base);

        if (typeid(*term.power) != typeid(Expression)) {
            auto power = std::make_unique<Expression>();
            *power += std::move(term.power);
            term.power = std::move(power);
        }

        auto &power = dynamic_cast<Expression &>(*term.power);

        for (auto const &[i, t] : this->terms | std::views::enumerate) {
            if (typeid(*t) == typeid(Variable)) {
                if (auto const &v = dynamic_cast<Variable const &>(*t);
                    v != variable)
                    continue;

                power += std::make_unique<Constant>(1);
                term.power = term.power->simplified();
                t = std::move(token);

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
                term.power = term.power->simplified();
                t = std::move(token);

                return *this;
            }
        }

        this->terms.push_back(std::move(token));

        return *this;
    }

    if (typeid(*token) == typeid(Terms)) {
        auto &terms = dynamic_cast<Terms &>(*token);

        if (terms.coefficient != 1) {
            *this *= terms.coefficient;
            terms.coefficient = 1;
        }

        for (OwnedToken &term : terms.terms)
            *this *= std::move(term);

        return *this;
    }

    this->terms.push_back(std::move(token));

    return *this;
}

mlp::Terms &mlp::Terms::operator/=(OwnedToken &&token) {
    if (typeid(*token) == typeid(Constant)) {
        this->coefficient /= dynamic_cast<Constant const &>(*token);

        return *this;
    }

    if (typeid(*token) == typeid(Variable)) {
        auto const &variable = dynamic_cast<Variable const &>(*token);

        for (auto const &[i, term] : this->terms | std::views::enumerate) {
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

        this->terms.push_back(
            std::make_unique<Term>(std::move(*token.release()) ^ -1)
        );

        return *this;
    }

    if (typeid(*token) == typeid(Term)) {
        auto &term = dynamic_cast<Term &>(*token);

        if (term.coefficient != 1) {
            *this /= term.coefficient;
            term.coefficient = 1;
        }

        if (typeid(*term.base) != typeid(Variable)) {
            this->terms.push_back(
                std::make_unique<Term>(std::move(*token.release()) ^ -1)
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

        for (auto const &[i, t] : this->terms | std::views::enumerate) {
            if (typeid(*t) == typeid(Variable)) {
                if (auto const &v = dynamic_cast<Variable const &>(*t);
                    v != variable)
                    continue;

                power += std::make_unique<Constant>(1);
                term.power = term.power->simplified();
                t = std::move(token);

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
                term.power = term.power->simplified();
                t = std::move(token);

                return *this;
            }
        }

        this->terms.push_back(
            std::make_unique<Term>(std::move(*token.release()) ^ -1)
        );

        return *this;
    }

    if (typeid(*token) == typeid(Terms)) {
        auto &terms = dynamic_cast<Terms &>(*token);

        if (terms.coefficient != 1) {
            *this /= terms.coefficient;
            terms.coefficient = 1;
        }

        for (OwnedToken &term : terms.terms)
            *this /= std::move(term);

        return *this;
    }

    this->terms.push_back(
        std::make_unique<Term>(std::move(*token.release()) ^ -1)
    );

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

mlp::Terms mlp::Terms::operator-() {
    Terms terms{};
    terms.coefficient = -this->coefficient;

    for (OwnedToken const &token : this->terms)
        terms *= OwnedToken(token->clone());

    return terms;
}

mlp::Terms operator*(std::double_t const lhs, mlp::Terms rhs) {
    return std::move(rhs *= lhs);
}

mlp::Terms operator*(mlp::Terms lhs, std::double_t const rhs) {
    return std::move(lhs *= rhs);
}

mlp::Terms operator/(mlp::Terms lhs, std::double_t const rhs) {
    return std::move(lhs /= rhs);
}

mlp::Terms operator*(mlp::Constant const &lhs, mlp::Terms rhs) {
    return std::move(rhs *= lhs);
}

mlp::Terms operator*(mlp::Terms lhs, mlp::Constant const &rhs) {
    return std::move(lhs *= rhs);
}

mlp::Terms operator/(mlp::Terms lhs, mlp::Constant const &rhs) {
    return std::move(lhs /= rhs);
}