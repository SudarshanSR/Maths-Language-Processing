#include "token.h"

#include <sstream>

void mlp::Terms::add_term(OwnedToken &&token) {
    auto const &term_type = typeid(*token);

    if (term_type == typeid(Constant)) {
        this->coefficient *= dynamic_cast<Constant const&>(*token);

        return;
    }

    if (term_type == typeid(Term)) {
        auto &term = dynamic_cast<Term &>(*token);
        *this *= term.coefficient;
        term.coefficient = 1;

        this->terms.push_back(std::move(token));

        return;
    }

    if (term_type == typeid(Terms)) {
        auto &terms = dynamic_cast<Terms &>(*token);
        *this *= terms.coefficient;
        terms.coefficient = 1;

        for (OwnedToken &term : terms.terms)
            this->add_term(std::move(term));

        return;
    }

    this->terms.push_back(std::move(token));
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
        result << static_cast<std::string>(*this->terms[i]);

        if (i != this->terms.size() - 1)
            result << '*';
    }

    result << ')';

    return result.str();
}

mlp::Terms &mlp::Terms::operator*=(OwnedToken &&token) {
    this->add_term(std::move(token));

    return *this;
}

mlp::Terms &mlp::Terms::operator/=(OwnedToken &&token) {
    this->add_term(
        std::make_unique<Term>(std::move(token), std::make_unique<Constant>(-1))
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

mlp::Terms operator-(mlp::Terms const &rhs) {
    mlp::Terms terms{};
    terms.coefficient = -rhs.coefficient;

    for (mlp::OwnedToken const &token : rhs.terms)
        terms.add_term(mlp::OwnedToken(token->clone()));

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