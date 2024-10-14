#include "token.h"

#include <cmath>
#include <map>
#include <ranges>

OwnedToken Constant::simplified() const { return this->clone(); }

OwnedToken Variable::simplified() const { return this->clone(); }

OwnedToken Function::simplified() const {
    return std::make_unique<Function>(
        this->function,
        dynamic_cast<Simplifiable &>(*this->parameter).simplified()
    );
}

OwnedToken Term::simplified() const {
    auto clone = this->clone();
    auto &term = dynamic_cast<Term &>(*clone);

    if (!term.power)
        term.power = std::make_unique<Constant>(1);

    term.base = dynamic_cast<Simplifiable &>(*term.base).simplified();
    term.power = dynamic_cast<Simplifiable &>(*term.power).simplified();

    if (typeid(*term.power) == typeid(Constant)) {
        auto &power = dynamic_cast<Constant &>(*term.power);

        if (term.coefficient.value == 1 && power.value == 1)
            return std::move(term.base);

        if (power.value == 0)
            return std::make_unique<Constant>(term.coefficient);

        if (typeid(*term.base) == typeid(Term)) {
            auto &base = dynamic_cast<Term &>(*term.base);
            base.coefficient.value =
                std::pow(base.coefficient.value, power.value);

            if (typeid(*base.power) == typeid(Constant)) {
                dynamic_cast<Constant &>(*base.power).value *= power.value;
            } else if (typeid(*base.power) == typeid(Term)) {
                dynamic_cast<Term &>(*base.power).coefficient.value *=
                    power.value;
            } else if (typeid(*base.power) == typeid(Terms)) {
                dynamic_cast<Terms &>(*base.power).coefficient.value *=
                    power.value;
            } else {
                auto terms = std::make_unique<Terms>();
                terms->coefficient = power;
                terms->add_term(std::move(base.power));

                base.power = std::move(terms);
            }

            power.value = 1;

            if (term.coefficient.value == 1 && power.value == 1)
                return std::move(term.base);
        }
    }

    if (typeid(*term.base) == typeid(Constant)) {
        auto const &base = dynamic_cast<Constant &>(*term.base);

        if (typeid(*term.power) == typeid(Constant))
            return std::make_unique<Constant>(
                term.coefficient.value *
                std::powl(
                    base.value, dynamic_cast<Constant &>(*term.power).value
                )
            );

        if (term.coefficient == base) {
            if (typeid(*term.power) == typeid(Expression)) {
                auto &power = dynamic_cast<Expression &>(*term.power);
                power.add_token(std::make_unique<Operation>(Operation::add));
                power.add_token(std::make_unique<Constant>(1));
                term.coefficient.value = 1;
            }
        }
    }

    term.base = dynamic_cast<Simplifiable &>(*term.base).simplified();
    term.power = dynamic_cast<Simplifiable &>(*term.power).simplified();

    return clone;
}

OwnedToken Terms::simplified() const {
    if (this->coefficient.value == 0)
        return std::make_unique<Constant>(0);

    if (this->terms.empty())
        return this->coefficient.clone();

    if (this->terms.size() == 1) {
        auto term = this->terms[0]->clone();

        if (typeid(*term) == typeid(Constant))
            return std::make_unique<Constant>(
                this->coefficient.value * dynamic_cast<Constant &>(*term).value
            );

        if (typeid(*term) == typeid(Term)) {
            dynamic_cast<Term &>(*term).coefficient.value *=
                this->coefficient.value;

            return dynamic_cast<Simplifiable &>(*term).simplified();
        }

        return Term(
                   this->coefficient.value, std::move(term),
                   std::make_unique<Constant>(1)
        )
            .simplified();
    }

    auto clone = this->clone();
    auto &terms = dynamic_cast<Terms &>(*clone);

    std::map<Variable, OwnedToken *> variable_powers;

    std::int32_t i = 0;

    while (i < terms.terms.size()) {
        auto token = dynamic_cast<Simplifiable &>(*terms.terms[i]).simplified();

        if (typeid(*token) == typeid(Constant)) {
            auto const &constant = dynamic_cast<Constant &>(*token);

            if (constant.value == 0)
                return constant.clone();

            terms.coefficient.value *= constant.value;
            terms.terms.erase(terms.terms.begin() + i);

            continue;
        }

        if (typeid(*token) == typeid(Variable)) {
            auto variable = dynamic_cast<Variable &>(*token);

            if (variable_powers.contains(variable)) {
                auto &power = dynamic_cast<Expression &>(
                    *dynamic_cast<Term &>(**variable_powers[variable]).power
                );
                power.add_token(std::make_unique<Operation>(Operation::add));
                power.add_token(std::make_unique<Constant>(1));

                terms.terms.erase(terms.terms.begin() + 1);

                continue;
            }

            auto power = std::make_unique<Expression>();
            power->add_token(std::make_unique<Constant>(1));

            terms.terms[i] =
                std::make_unique<Term>(variable.clone(), std::move(power));
            variable_powers[variable] = &terms.terms[i];

            ++i;

            continue;
        }

        if (typeid(*token) == typeid(Term)) {
            auto &term = dynamic_cast<Term &>(*token);
            terms.coefficient.value *= term.coefficient.value;
            term.coefficient.value = 1;

            if (typeid(*term.base) == typeid(Variable)) {
                auto &variable = dynamic_cast<Variable &>(*term.base);

                if (variable_powers.contains(variable)) {
                    auto &power = dynamic_cast<Expression &>(
                        *dynamic_cast<Term &>(**variable_powers[variable]).power
                    );
                    power.add_token(std::make_unique<Operation>(Operation::add)
                    );
                    power.add_token(std::move(term.power));

                    terms.terms.erase(terms.terms.begin() + 1);

                    continue;
                }

                if (typeid(*term.power) != typeid(Expression)) {
                    auto power = std::make_unique<Expression>();
                    power->add_token(std::move(term.power));
                    term.power = std::move(power);
                }

                terms.terms[i] = std::move(token);
                variable_powers[variable] = &terms.terms[i];

                ++i;

                continue;
            }
        }

        terms.terms[i] = std::move(token);

        ++i;
    }

    for (auto const token : variable_powers | std::views::values) {
        if (auto &term = dynamic_cast<Term &>(**token);
            dynamic_cast<Expression &>(*term.power).tokens.empty()) {
            *token = std::move(term.base);

            continue;
        }

        *token = dynamic_cast<Simplifiable &>(**token).simplified();
    }

    if (terms.terms.empty())
        return std::make_unique<Constant>(terms.coefficient);

    if (terms.terms.size() == 1) {
        auto &term = terms.terms[0];

        if (typeid(*term) == typeid(Constant))
            return std::make_unique<Constant>(
                terms.coefficient.value * dynamic_cast<Constant &>(*term).value
            );

        if (typeid(*term) == typeid(Term)) {
            dynamic_cast<Term &>(*term).coefficient.value *=
                terms.coefficient.value;

            return dynamic_cast<Simplifiable &>(*term).simplified();
        }

        return Term(
                   terms.coefficient.value, std::move(term),
                   std::make_unique<Constant>(1)
        )
            .simplified();
    }

    return clone;
}

OwnedToken Expression::simplified() const {
    auto clone = this->clone();
    auto &expression = dynamic_cast<Expression &>(*clone);

    std::vector<OwnedToken> &tokens = expression.tokens;

    if (tokens.size() == 1)
        return dynamic_cast<Simplifiable &>(*expression.pop_token())
            .simplified();

    for (OwnedToken &token : tokens)
        if (typeid(*token) != typeid(Operation))
            token = dynamic_cast<Simplifiable &>(*token).simplified();

    for (int i = 1; i < tokens.size(); ++i) {
        OwnedToken &token = tokens[i];

        if (typeid(*token) != typeid(Operation))
            continue;

        if (i == tokens.size())
            throw std::invalid_argument("Expression is not valid!");

        OwnedToken &left = tokens[i - 1];
        OwnedToken &right = tokens[i + 1];

        auto &operation = dynamic_cast<Operation &>(*token);

        if (operation.operation == Operation::add) {
            if (typeid(*left) == typeid(Constant)) {
                if (auto const left_constant = dynamic_cast<Constant &>(*left);
                    left_constant.value == 0) {
                    --i;

                    tokens.erase(tokens.begin() + i);
                    tokens.erase(tokens.begin() + i);
                } else if (typeid(*right) == typeid(Constant)) {
                    auto &right_constant = dynamic_cast<Constant &>(*right);

                    right_constant.value += left_constant.value;

                    --i;

                    tokens.erase(tokens.begin() + i);
                    tokens.erase(tokens.begin() + i);
                }

                continue;
            }

            if (typeid(*right) == typeid(Constant)) {
                auto &right_constant = dynamic_cast<Constant &>(*right);

                if (right_constant.value > 0)
                    continue;

                if (right_constant.value != 0) {
                    right_constant.value = -right_constant.value;
                    operation.operation = Operation::sub;

                    continue;
                }

                tokens[i + 1] = std::move(left);

                --i;

                tokens.erase(tokens.begin() + i);
                tokens.erase(tokens.begin() + i);

                continue;
            }

            if (typeid(*right) == typeid(Terms)) {
                auto &right_terms = dynamic_cast<Terms &>(*right);

                if (right_terms.coefficient.value >= 0)
                    continue;

                right_terms.coefficient.value = -right_terms.coefficient.value;
                operation.operation = Operation::sub;

                continue;
            }

            if (typeid(*right) == typeid(Term)) {
                auto &right_term = dynamic_cast<Term &>(*right);

                if (right_term.coefficient.value >= 0)
                    continue;

                right_term.coefficient.value = -right_term.coefficient.value;
                operation.operation = Operation::sub;

                continue;
            }
        } else if (operation.operation == Operation::sub) {
            if (typeid(*left) == typeid(Constant)) {
                if (typeid(*right) == typeid(Constant)) {
                    auto &right_constant = dynamic_cast<Constant &>(*right);

                    right_constant.value =
                        dynamic_cast<Constant &>(*left).value -
                        right_constant.value;

                    --i;

                    tokens.erase(tokens.begin() + i);
                    tokens.erase(tokens.begin() + i);
                }

                continue;
            }

            if (typeid(*right) == typeid(Constant)) {
                if (dynamic_cast<Constant &>(*right).value != 0)
                    continue;

                tokens[i + 1] = std::move(left);

                --i;

                tokens.erase(tokens.begin() + i);
                tokens.erase(tokens.begin() + i);
            }
        }
    }

    if (tokens.size() == 1)
        return expression.pop_token();

    std::map<Variable, OwnedToken *> variable_multiples;

    for (int i = 0; i < tokens.size(); ++i) {
        OwnedToken &token = tokens[i];

        auto const &token_type = typeid(*token);

        if (token_type == typeid(Variable)) {
            auto &variable = dynamic_cast<Variable &>(*token);

            if (variable_multiples.contains(variable)) {
                auto &terms =
                    dynamic_cast<Terms &>(**variable_multiples[variable]);

                auto &multiple = dynamic_cast<Expression &>(*terms.terms[0]);
                multiple.add_token(std::move(tokens[i - 1]));
                multiple.add_token(std::make_unique<Constant>(1));

                --i;

                tokens.erase(tokens.begin() + i);
                tokens.erase(tokens.begin() + i);

                --i;

                continue;
            }

            auto multiple = std::make_unique<Expression>();

            if (i != 0)
                multiple->add_token(std::move(tokens[i - 1]));

            multiple->add_token(std::make_unique<Constant>(1));

            auto terms = std::make_unique<Terms>();
            terms->add_term(std::move(multiple));
            terms->add_term(std::move(token));

            tokens[i] = std::move(terms);
            variable_multiples[variable] = &tokens[i];

            continue;
        }

        if (token_type == typeid(Operation)) {
            auto &operation = dynamic_cast<Operation &>(*token);

            if (i == tokens.size() - 1)
                throw std::invalid_argument("Expression is not valid!");

            if (operation.operation == Operation::sub) {
                if (i != 0)
                    continue;

                if (OwnedToken &next = tokens[i + 1];
                    typeid(*next) == typeid(Constant)) {
                    auto &constant = dynamic_cast<Constant &>(*next);

                    constant.value = -constant.value;
                } else if (typeid(*next) == typeid(Term)) {
                    auto &term = dynamic_cast<Term &>(*next);

                    term.coefficient.value = -term.coefficient.value;
                } else if (typeid(*next) == typeid(Terms)) {
                    auto &terms = dynamic_cast<Terms &>(*next);

                    terms.coefficient.value = -terms.coefficient.value;
                } else {
                    tokens[i + 1] = std::make_unique<Term>(
                        -1, std::move(next), std::make_unique<Constant>(1)
                    );
                }

                --i;
            } else if (i == 0) {
                --i;
            } else {
                continue;
            }

            tokens.erase(tokens.begin());
        }
    }

    for (auto const token : variable_multiples | std::views::values) {
        if (auto &terms = dynamic_cast<Terms &>(**token);
            dynamic_cast<Expression &>(*terms.terms[0]).tokens.empty()) {
            *token = std::move(terms.terms[1]);

            continue;
        }

        *token = dynamic_cast<Simplifiable &>(**token).simplified();
    }

    if (tokens.size() == 1)
        return expression.pop_token();

    return clone;
}