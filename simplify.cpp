#include "token.h"

#include <map>
#include <ranges>
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

            if (term->coefficient == 1 && power == 1)
                return std::move(term->base);
        }
    }

    if (typeid(*term->base) == typeid(Constant)) {
        auto const &base = dynamic_cast<Constant const &>(*term->base);

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

    auto terms = Owned<Terms>(this->clone());

    std::map<Variable, OwnedToken *> variable_powers;

    std::int32_t i = 0;

    while (i < terms->terms.size()) {
        auto t = terms->terms[i]->simplified();

        if (typeid(*t) == typeid(Constant)) {
            auto const &constant = dynamic_cast<Constant const &>(*t);

            if (constant == 0)
                return std::make_unique<Constant>(0);

            *terms *= constant;
            terms->terms.erase(terms->terms.begin() + i);

            continue;
        }

        if (typeid(*t) == typeid(Variable)) {
            auto const &variable = dynamic_cast<Variable const &>(*t);

            if (variable_powers.contains(variable)) {
                auto &power = dynamic_cast<Expression &>(
                    *dynamic_cast<Term &>(**variable_powers[variable]).power
                );
                power += std::make_unique<Constant>(1);

                terms->terms.erase(terms->terms.begin() + 1);

                continue;
            }

            auto power = std::make_unique<Expression>();
            *power += std::make_unique<Constant>(1);

            terms->terms[i] =
                std::make_unique<Term>(variable ^ std::move(power));
            variable_powers[variable] = &terms->terms[i];

            ++i;

            continue;
        }

        if (typeid(*t) == typeid(Term)) {
            auto &term = dynamic_cast<Term &>(*t);
            *terms *= term.coefficient;
            term.coefficient = 1;

            if (typeid(*term.base) == typeid(Variable)) {
                auto const &variable =
                    dynamic_cast<Variable const &>(*term.base);

                if (variable_powers.contains(variable)) {
                    auto &power = dynamic_cast<Expression &>(
                        *dynamic_cast<Term &>(**variable_powers[variable]).power
                    );
                    power += std::make_unique<Constant>(1);

                    terms->terms.erase(terms->terms.begin() + 1);

                    continue;
                }

                if (typeid(*term.power) != typeid(Expression)) {
                    auto power = std::make_unique<Expression>();
                    *power += std::move(term.power);
                    term.power = std::move(power);
                }

                terms->terms[i] = std::move(t);
                variable_powers[variable] = &terms->terms[i];

                ++i;

                continue;
            }
        }

        terms->terms[i] = std::move(t);

        ++i;
    }

    for (auto const t : variable_powers | std::views::values) {
        if (auto &term = dynamic_cast<Term &>(**t);
            dynamic_cast<Expression const &>(*term.power).empty()) {
            *t = std::move(term.base);

            continue;
        }

        *t = (*t)->simplified();
    }

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
    auto expression = Owned<Expression>(this->clone());

    std::vector<std::pair<Sign, OwnedToken>> &tokens = expression->tokens;

    if (tokens.empty())
        return std::make_unique<Constant>(0);

    if (tokens.size() == 1) {
        auto &&[sign, term] = expression->pop_token();

        if (sign == Sign::pos)
            return term->simplified();

        return (-(std::move(*term.release()) ^ 1)).simplified();
    }

    for (auto &t : tokens | std::views::values)
        t = t->simplified();

    std::pair<Sign, OwnedToken> *constant = nullptr;
    std::map<Variable, std::pair<Sign, OwnedToken> *> variable_multiples;

    for (std::size_t i = 0; i < tokens.size(); ++i) {
        Sign &prev = tokens[i].first;
        OwnedToken &left = tokens[i].second;

        if (typeid(*left) == typeid(Constant)) {
            if (!constant) {
                constant = &tokens[i];

                continue;
            }

            if (auto &[op, c] = *constant; op == prev)
                dynamic_cast<Constant &>(*c) +=
                    dynamic_cast<Constant const &>(*left);

            else
                dynamic_cast<Constant &>(*c) -=
                    dynamic_cast<Constant const &>(*left);

            tokens.erase(tokens.begin() + i--);

            continue;
        }

        if (typeid(*left) == typeid(Variable)) {
            auto const &variable = dynamic_cast<Variable const &>(*left);

            if (!variable_multiples.contains(variable)) {
                auto multiple = std::make_unique<Expression>();

                multiple->add_token(prev, std::make_unique<Constant>(1));

                auto terms = std::make_unique<Terms>();
                *terms *= std::move(multiple);
                *terms *= std::move(left);

                tokens[i] = {Sign::pos, std::move(terms)};
                variable_multiples[variable] = &tokens[i];

                continue;
            }

            auto &terms =
                dynamic_cast<Terms &>(*variable_multiples[variable]->second);

            auto &multiple = dynamic_cast<Expression &>(*terms.terms[0]);
            multiple.add_token(prev, std::make_unique<Constant>(1));

            tokens.erase(tokens.begin() + i--);

            continue;
        }

        if (typeid(*left) == typeid(Term)) {
            auto &term = dynamic_cast<Term &>(*left);

            if (term.coefficient == 0) {
                tokens.erase(tokens.begin() + i--);

                continue;
            }

            if (term.coefficient < 0) {
                prev = prev == Sign::pos ? Sign::neg : Sign::pos;

                term.coefficient = -term.coefficient;
            }

            if (typeid(*term.base) != typeid(Variable) ||
                typeid(*term.power) != typeid(Constant) ||
                dynamic_cast<Constant const &>(*term.power) != 1)
                continue;

            auto const &variable = dynamic_cast<Variable const &>(*term.base);

            if (!variable_multiples.contains(variable)) {
                auto multiple = std::make_unique<Expression>();

                multiple->add_token(
                    prev, std::make_unique<Constant>(term.coefficient)
                );

                auto terms = std::make_unique<Terms>();
                *terms *= std::move(multiple);
                *terms *= std::move(term.base);

                tokens[i] = {Sign::pos, std::move(terms)};
                variable_multiples[variable] = &tokens[i];

                continue;
            }

            auto &terms =
                dynamic_cast<Terms &>(*variable_multiples[variable]->second);

            auto &multiple = dynamic_cast<Expression &>(*terms.terms[0]);
            multiple.add_token(
                prev, std::make_unique<Constant>(term.coefficient)
            );

            tokens.erase(tokens.begin() + i--);

            continue;
        }

        if (typeid(*left) == typeid(Terms)) {
            auto &terms = dynamic_cast<Terms &>(*left);

            if (terms.coefficient == 0) {
                tokens.erase(tokens.begin() + i--);

                continue;
            }

            if (terms.coefficient < 0) {
                prev = prev == Sign::pos ? Sign::neg : Sign::pos;

                terms.coefficient = -terms.coefficient;
            }
        }
    }

    if (constant) {
        auto &[sign, t] = *constant;

        if (auto &c = dynamic_cast<Constant &>(*t); c == 0) {
            for (std::size_t i = 0; i < tokens.size(); ++i) {
                if (tokens[i] != *constant)
                    continue;

                tokens.erase(tokens.begin() + i);

                break;
            }
        } else if (c < 0) {
            sign = sign == Sign::pos ? Sign::neg : Sign::pos;

            c = -c;
        }
    }

    for (auto const t : variable_multiples | std::views::values)
        t->second = t->second->simplified();

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
