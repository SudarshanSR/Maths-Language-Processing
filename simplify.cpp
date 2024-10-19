#include "token.h"

#include <cmath>
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
    return std::make_unique<Function>(
        this->function, this->parameter->simplified()
    );
}

mlp::OwnedToken mlp::Term::simplified() const {
    auto term = Owned<Term>(this->clone());

    term->base = term->base->simplified();
    term->power = term->power->simplified();

    if (typeid(*term->power) == typeid(Constant)) {
        auto &power = dynamic_cast<Constant &>(*term->power);

        if (term->coefficient == 1 && power.value == 1)
            return std::move(term->base);

        if (power.value == 0)
            return std::make_unique<Constant>(term->coefficient);

        if (typeid(*term->base) == typeid(Term)) {
            auto &base = dynamic_cast<Term &>(*term->base);
            base.coefficient = std::pow(base.coefficient, power.value);

            if (typeid(*base.power) == typeid(Constant)) {
                dynamic_cast<Constant &>(*base.power).value *= power.value;
            } else if (typeid(*base.power) == typeid(Term)) {
                dynamic_cast<Term &>(*base.power).coefficient *= power.value;
            } else if (typeid(*base.power) == typeid(Terms)) {
                dynamic_cast<Terms &>(*base.power).coefficient *= power.value;
            } else {
                auto terms = std::make_unique<Terms>();
                terms->coefficient = power.value;
                terms->add_term(std::move(base.power));

                base.power = std::move(terms);
            }

            power.value = 1;

            if (term->coefficient == 1 && power.value == 1)
                return std::move(term->base);
        }
    }

    if (typeid(*term->base) == typeid(Constant)) {
        auto const &base = dynamic_cast<Constant &>(*term->base);

        if (typeid(*term->power) == typeid(Constant))
            return std::make_unique<Constant>(
                term->coefficient *
                std::powl(
                    base.value, dynamic_cast<Constant &>(*term->power).value
                )
            );

        if (term->coefficient == base.value) {
            if (typeid(*term->power) == typeid(Expression)) {
                auto &power = dynamic_cast<Expression &>(*term->power);
                power.add_token(Sign::pos, std::make_unique<Constant>(1));
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
                this->coefficient * dynamic_cast<Constant &>(*term).value
            );

        if (typeid(*term) == typeid(Term)) {
            dynamic_cast<Term &>(*term).coefficient *= this->coefficient;

            return term->simplified();
        }

        return Term(
                   this->coefficient, std::move(term),
                   std::make_unique<Constant>(1)
        )
            .simplified();
    }

    auto terms = Owned<Terms>(this->clone());

    std::map<Variable, OwnedToken *> variable_powers;

    std::int32_t i = 0;

    while (i < terms->terms.size()) {
        auto t = terms->terms[i]->simplified();

        if (typeid(*t) == typeid(Constant)) {
            auto const &constant = dynamic_cast<Constant &>(*t);

            if (constant.value == 0)
                return std::make_unique<Constant>(0);

            terms->coefficient *= constant.value;
            terms->terms.erase(terms->terms.begin() + i);

            continue;
        }

        if (typeid(*t) == typeid(Variable)) {
            auto variable = dynamic_cast<Variable &>(*t);

            if (variable_powers.contains(variable)) {
                auto &power = dynamic_cast<Expression &>(
                    *dynamic_cast<Term &>(**variable_powers[variable]).power
                );
                power.add_token(Sign::pos, std::make_unique<Constant>(1));

                terms->terms.erase(terms->terms.begin() + 1);

                continue;
            }

            auto power = std::make_unique<Expression>();
            power->add_token(Sign::pos, std::make_unique<Constant>(1));

            terms->terms[i] = std::make_unique<Term>(
                Owned<Variable>(variable.clone()), std::move(power)
            );
            variable_powers[variable] = &terms->terms[i];

            ++i;

            continue;
        }

        if (typeid(*t) == typeid(Term)) {
            auto &term = dynamic_cast<Term &>(*t);
            terms->coefficient *= term.coefficient;
            term.coefficient = 1;

            if (typeid(*term.base) == typeid(Variable)) {
                auto &variable = dynamic_cast<Variable &>(*term.base);

                if (variable_powers.contains(variable)) {
                    auto &power = dynamic_cast<Expression &>(
                        *dynamic_cast<Term &>(**variable_powers[variable]).power
                    );
                    power.add_token(Sign::pos, std::move(term.power));

                    terms->terms.erase(terms->terms.begin() + 1);

                    continue;
                }

                if (typeid(*term.power) != typeid(Expression)) {
                    auto power = std::make_unique<Expression>();
                    power->add_token(Sign::pos, std::move(term.power));
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
            dynamic_cast<Expression &>(*term.power).empty()) {
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
                terms->coefficient * dynamic_cast<Constant &>(*term).value
            );

        if (typeid(*term) == typeid(Term)) {
            dynamic_cast<Term &>(*term).coefficient *= terms->coefficient;

            return term->simplified();
        }

        return Term(
                   terms->coefficient, std::move(term),
                   std::make_unique<Constant>(1)
        )
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

        return Term(-1, std::move(term), std::make_unique<Constant>(1))
            .simplified();
    }

    for (auto &t : tokens | std::views::values)
        t = t->simplified();

    std::pair<Sign, OwnedToken> *constant = nullptr;
    std::map<Variable, std::pair<Sign, OwnedToken> *> variable_multiples;

    for (int i = 0; i < tokens.size(); ++i) {
        Sign &prev = tokens[i].first;
        OwnedToken &left = tokens[i].second;

        if (typeid(*left) == typeid(Constant)) {
            if (!constant) {
                constant = &tokens[i];

                continue;
            }

            if (auto &[op, c] = *constant; op == prev)
                dynamic_cast<Constant &>(*c).value +=
                    dynamic_cast<Constant const &>(*left).value;

            else
                dynamic_cast<Constant &>(*c).value -=
                    dynamic_cast<Constant const &>(*left).value;

            tokens.erase(tokens.begin() + i--);

            continue;
        }

        if (typeid(*left) == typeid(Variable)) {
            auto &variable = dynamic_cast<Variable &>(*left);

            if (!variable_multiples.contains(variable)) {
                auto multiple = std::make_unique<Expression>();

                multiple->add_token(prev, std::make_unique<Constant>(1));

                auto terms = std::make_unique<Terms>();
                terms->add_term(std::move(multiple));
                terms->add_term(std::move(left));

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

            if (term.coefficient < 0) {
                prev = prev == Sign::pos ? Sign::neg : Sign::pos;

                term.coefficient = -term.coefficient;
            }

            if (typeid(*term.base) != typeid(Variable) ||
                typeid(*term.power) != typeid(Constant) ||
                dynamic_cast<Constant const &>(*term.power).value != 1)
                continue;

            auto &variable = dynamic_cast<Variable &>(*term.base);

            if (!variable_multiples.contains(variable)) {
                auto multiple = std::make_unique<Expression>();

                multiple->add_token(
                    prev, std::make_unique<Constant>(term.coefficient)
                );

                auto terms = std::make_unique<Terms>();
                terms->add_term(std::move(multiple));
                terms->add_term(std::move(term.base));

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

            if (terms.coefficient < 0) {
                prev = prev == Sign::pos ? Sign::neg : Sign::pos;

                terms.coefficient = -terms.coefficient;
            }
        }
    }

    if (constant) {
        auto &[sign, t] = *constant;

        auto &c = dynamic_cast<Constant &>(*t);

        if (c.value == 0) {
            for (int i = 0; i < tokens.size(); ++i) {
                if (tokens[i] != *constant)
                    continue;

                tokens.erase(tokens.begin() + i);

                break;
            }
        } else if (c.value < 0) {
            sign = sign == Sign::pos ? Sign::neg : Sign::pos;

            c.value = -c.value;
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

        return Term(-1, std::move(term), std::make_unique<Constant>(1))
            .simplified();
    }

    return expression;
}
