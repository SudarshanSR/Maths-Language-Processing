#include "simplify.h"

#include <cmath>
#include <map>
#include <ranges>
#include <typeinfo>

mlp::OwnedToken mlp::simplified(Token const &token) {
    auto const &type = typeid(token);

    if (type == typeid(Constant))
        return simplified(dynamic_cast<Constant const &>(token));

    if (type == typeid(Variable))
        return simplified(dynamic_cast<Variable const &>(token));

    if (type == typeid(Function))
        return simplified(dynamic_cast<Function const &>(token));

    if (type == typeid(Term))
        return simplified(dynamic_cast<Term const &>(token));

    if (type == typeid(Terms))
        return simplified(dynamic_cast<Terms const &>(token));

    if (type == typeid(Expression))
        return simplified(dynamic_cast<Expression const &>(token));

    throw std::invalid_argument("Invalid argument!");
}

mlp::OwnedToken mlp::simplified(Constant const &token) { return token.clone(); }

mlp::OwnedToken mlp::simplified(Variable const &token) { return token.clone(); }

mlp::OwnedToken mlp::simplified(Function const &token) {
    return std::make_unique<Function>(
        token.function, simplified(*token.parameter)
    );
}

mlp::OwnedToken mlp::simplified(Term const &token) {
    auto clone = token.clone();
    auto &term = dynamic_cast<Term &>(*clone);

    if (!term.power)
        term.power = std::make_unique<Constant>(1);

    term.base = simplified(*term.base);
    term.power = simplified(*term.power);

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

    term.base = simplified(*term.base);
    term.power = simplified(*term.power);

    return clone;
}

mlp::OwnedToken mlp::simplified(Terms const &token) {
    if (token.coefficient.value == 0)
        return std::make_unique<Constant>(0);

    if (token.terms.empty())
        return token.coefficient.clone();

    if (token.terms.size() == 1) {
        auto term = token.terms[0]->clone();

        if (typeid(*term) == typeid(Constant))
            return std::make_unique<Constant>(
                token.coefficient.value * dynamic_cast<Constant &>(*term).value
            );

        if (typeid(*term) == typeid(Term)) {
            dynamic_cast<Term &>(*term).coefficient.value *=
                token.coefficient.value;

            return simplified(*term);
        }

        return simplified(Term(
            token.coefficient.value, std::move(term),
            std::make_unique<Constant>(1)
        ));
    }

    auto clone = token.clone();
    auto &terms = dynamic_cast<Terms &>(*clone);

    std::map<Variable, OwnedToken *> variable_powers;

    std::int32_t i = 0;

    while (i < terms.terms.size()) {
        auto t = simplified(*terms.terms[i]);

        if (typeid(*t) == typeid(Constant)) {
            auto const &constant = dynamic_cast<Constant &>(*t);

            if (constant.value == 0)
                return constant.clone();

            terms.coefficient.value *= constant.value;
            terms.terms.erase(terms.terms.begin() + i);

            continue;
        }

        if (typeid(*t) == typeid(Variable)) {
            auto variable = dynamic_cast<Variable &>(*t);

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

        if (typeid(*t) == typeid(Term)) {
            auto &term = dynamic_cast<Term &>(*t);
            terms.coefficient.value *= term.coefficient.value;
            term.coefficient.value = 1;

            if (typeid(*term.base) == typeid(Variable)) {
                auto &variable = dynamic_cast<Variable &>(*term.base);

                if (variable_powers.contains(variable)) {
                    auto &power = dynamic_cast<Expression &>(
                        *dynamic_cast<Term &>(**variable_powers[variable]).power
                    );
                    power.add_token(
                        std::make_unique<Operation>(Operation::add)
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

                terms.terms[i] = std::move(t);
                variable_powers[variable] = &terms.terms[i];

                ++i;

                continue;
            }
        }

        terms.terms[i] = std::move(t);

        ++i;
    }

    for (auto const t : variable_powers | std::views::values) {
        if (auto &term = dynamic_cast<Term &>(**t);
            dynamic_cast<Expression &>(*term.power).tokens.empty()) {
            *t = std::move(term.base);

            continue;
        }

        *t = simplified(**t);
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

            return simplified(*term);
        }

        return simplified(Term(
            terms.coefficient.value, std::move(term),
            std::make_unique<Constant>(1)
        ));
    }

    return clone;
}

mlp::OwnedToken mlp::simplified(Expression const &token) {
    auto clone = token.clone();
    auto &expression = dynamic_cast<Expression &>(*clone);

    std::vector<OwnedToken> &tokens = expression.tokens;

    if (tokens.size() == 1)
        return simplified(*expression.pop_token());

    for (OwnedToken &t : tokens)
        if (typeid(*t) != typeid(Operation))
            t = simplified(*t);

    for (int i = 1; i < tokens.size(); ++i) {
        OwnedToken &t = tokens[i];

        if (typeid(*t) != typeid(Operation))
            continue;

        if (i == tokens.size())
            throw std::invalid_argument("Expression is not valid!");

        OwnedToken &left = tokens[i - 1];
        OwnedToken &right = tokens[i + 1];

        auto &operation = dynamic_cast<Operation &>(*t);

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
        OwnedToken &t = tokens[i];

        auto const &token_type = typeid(*t);

        if (token_type == typeid(Variable)) {
            auto &variable = dynamic_cast<Variable &>(*t);

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
            terms->add_term(std::move(t));

            tokens[i] = std::move(terms);
            variable_multiples[variable] = &tokens[i];

            continue;
        }

        if (token_type == typeid(Operation)) {
            auto const &operation = dynamic_cast<Operation &>(*t);

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

    for (auto const t : variable_multiples | std::views::values) {
        if (auto &terms = dynamic_cast<Terms &>(**t);
            dynamic_cast<Expression &>(*terms.terms[0]).tokens.empty()) {
            *t = std::move(terms.terms[1]);

            continue;
        }

        *t = simplified(**t);
    }

    if (tokens.size() == 1)
        return expression.pop_token();

    return clone;
}
