#include "token.h"

#include <sstream>

mlp::Term::Term(
    std::double_t const coefficient, OwnedToken &&base, OwnedToken &&power
)
    : coefficient(coefficient), base(std::move(base)), power(std::move(power)) {
}

mlp::Term::Term(OwnedToken &&base, OwnedToken &&power)
    : base(std::move(base)), power(std::move(power)) {}

mlp::Term::Term(Term const &term)
    : coefficient(term.coefficient), base(term.base->clone()),
      power(term.power->clone()) {}

gsl::owner<mlp::Term *> mlp::Term::clone() const {
    return new Term(
        this->coefficient, OwnedToken(this->base->clone()),
        OwnedToken(this->power->clone())
    );
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
        dynamic_cast<Constant const&>(*this->power) != 1) {
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

gsl::owner<mlp::Terms *> mlp::Terms::clone() const {
    auto *terms = new Terms;
    terms->coefficient = this->coefficient;

    for (auto const &term : this->terms)
        terms->add_term(OwnedToken(term->clone()));

    return terms;
}

namespace mlp {
Term operator-(Term const &rhs) {
    return {
        -rhs.coefficient, OwnedToken(rhs.base->clone()),
        OwnedToken(rhs.power->clone())
    };
}

Term operator*(std::double_t const lhs, Term rhs) {
    return std::move(rhs *= lhs);
}

Term operator*(Term lhs, std::double_t const rhs) {
    return std::move(lhs *= rhs);
}

Term operator/(std::double_t const lhs, Term rhs) {
    rhs /= lhs;

    Expression power{};
    power -= std::move(rhs.power);
    rhs.power = power.simplified();

    return std::move(rhs);
}

Term operator/(Term lhs, std::double_t const rhs) {
    return std::move(lhs /= rhs);
}

Term operator*(Constant const &lhs, Term rhs) { return std::move(rhs *= lhs); }

Term operator*(Term lhs, Constant const &rhs) { return std::move(lhs *= rhs); }

Term operator/(Constant const &lhs, Term rhs) {
    rhs /= lhs;

    Expression power{};
    power -= std::move(rhs.power);
    rhs.power = power.simplified();

    return std::move(rhs);
}

Term operator/(Term lhs, Constant const &rhs) { return std::move(lhs /= rhs); }
} // namespace mlp