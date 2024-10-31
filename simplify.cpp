#include "token.h"

#include <map>
#include <typeinfo>

mlp::OwnedToken mlp::Constant::simplified() const {
    return std::unique_ptr<Constant>(this->clone());
}

mlp::OwnedToken mlp::Variable::simplified() const {
    return std::unique_ptr<Variable>(this->clone());
}

mlp::OwnedToken mlp::Function::simplified() const {
    OwnedToken simplified = this->parameter->simplified();

    if (typeid(*simplified) == typeid(Constant))
        return Function(this->function, std::move(simplified)).evaluate({});

    return std::make_unique<Function>(this->function, std::move(simplified));
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
            base.coefficient ^= power;

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
                power += std::make_unique<Constant>(1);
                term->coefficient = 1;
            }
        }
    }

    term->base = term->base->simplified();
    term->power = term->power->simplified();

    return term;
}

mlp::OwnedToken mlp::Terms::simplified() const {
    if (this->coefficient == 0)
        return std::make_unique<Constant>(0);

    if (this->terms.empty())
        return std::make_unique<Constant>(this->coefficient);

    if (this->terms.size() == 1) {
        auto term = OwnedToken(this->terms[0]->clone());

        if (typeid(*term) == typeid(Constant))
            return std::make_unique<Constant>(
                this->coefficient * dynamic_cast<Constant const &>(*term)
            );

        if (typeid(*term) == typeid(Term)) {
            dynamic_cast<Term &>(*term) *= this->coefficient;

            return term->simplified();
        }

        return (this->coefficient * (std::move(*term.release()) ^ 1))
            .simplified();
    }

    auto terms = std::make_unique<Terms>();
    terms->coefficient = this->coefficient;

    for (OwnedToken const &token : this->terms)
        *terms *= token->simplified();

    if (terms->terms.empty())
        return std::make_unique<Constant>(terms->coefficient);

    if (terms->terms.size() == 1) {
        auto &term = terms->terms[0];

        if (typeid(*term) == typeid(Constant))
            return std::make_unique<Constant>(
                terms->coefficient * dynamic_cast<Constant const &>(*term)
            );

        if (typeid(*term) == typeid(Term)) {
            dynamic_cast<Term &>(*term) *= terms->coefficient;

            return term->simplified();
        }

        return (terms->coefficient * (std::move(*term.release()) ^ 1))
            .simplified();
    }

    return terms;
}

mlp::OwnedToken mlp::Expression::simplified() const {
    if (this->tokens.empty())
        return std::make_unique<Constant>(0);

    if (this->tokens.size() == 1) {
        auto const expression = Owned<Expression>(this->clone());

        auto &&[sign, term] = expression->pop_token();

        if (sign == Sign::pos)
            return term->simplified();

        return (-(std::move(*term.release()) ^ 1)).simplified();
    }

    auto expression = std::make_unique<Expression>();

    std::vector<std::pair<Sign, OwnedToken>> const &tokens = expression->tokens;

    for (auto const &[sign, t] : this->tokens)
        expression->add_token(sign, t->simplified());

    if (tokens.empty())
        return std::make_unique<Constant>(0);

    if (tokens.size() == 1) {
        auto &&[sign, term] = expression->pop_token();

        if (sign == Sign::pos)
            return term->simplified();

        return (-(std::move(*term.release()) ^ 1)).simplified();
    }

    return expression;
}
