#include "token.h"

#include <cmath>
#include <map>
#include <ranges>

std::shared_ptr<Token> Constant::simplify() {
    return std::make_shared<Constant>(this->value);
}

std::shared_ptr<Token> Variable::simplify() {
    return std::make_shared<Variable>(this->var);
}

std::shared_ptr<Token> Operation::simplify() {
    throw std::runtime_error("Cannot simplify an operation!");
}

std::shared_ptr<Token> Function::simplify() {
    return std::make_shared<Function>(
        this->function, this->parameter->simplify()
    );
}

std::shared_ptr<Token> Term::simplify() {
    auto term = std::make_shared<Term>(
        this->coefficient.value, this->base, this->power
    );

    if (!term->power)
        term->power = std::make_shared<Constant>(1);

    term->base = term->base->simplify();
    term->power = term->power->simplify();

    if (typeid(*term->power) == typeid(Constant)) {
        if (term->coefficient.value == 1 &&
            std::dynamic_pointer_cast<Constant>(term->power)->value == 1)
            return term->base;

        if (std::dynamic_pointer_cast<Constant>(term->power)->value == 0)
            return std::make_shared<Constant>(term->coefficient);
    }

    if (typeid(*term->base) == typeid(Constant)) {
        auto const base = std::dynamic_pointer_cast<Constant>(term->base);

        if (typeid(*term->power) == typeid(Constant))
            return std::make_shared<Constant>(
                term->coefficient.value *
                std::powl(
                    base->value,
                    std::dynamic_pointer_cast<Constant>(term->power)->value
                )
            );

        if (term->coefficient == *base) {
            if (typeid(*term->power) == typeid(Expression)) {
                auto const power =
                    std::dynamic_pointer_cast<Expression>(term->power);
                power->add_token(std::make_shared<Operation>(Operation::add));
                power->add_token(std::make_shared<Constant>(1));
                term->coefficient.value = 1;
            }
        }
    }

    term->base = term->base->simplify();
    term->power = term->power->simplify();

    return term;
}

std::shared_ptr<Token> Terms::simplify() {
    auto terms = std::make_shared<Terms>();
    terms->coefficient = this->coefficient;
    terms->terms = this->terms;

    if (!terms->coefficient.value)
        return std::make_shared<Constant>(0);

    if (terms->terms.size() == 1) {
        auto term = terms->terms[0];

        if (typeid(*term) == typeid(Constant))
            return std::make_shared<Constant>(
                terms->coefficient.value *
                std::dynamic_pointer_cast<Constant>(term)->value
            );

        if (typeid(*term) == typeid(Term)) {
            std::dynamic_pointer_cast<Term>(term)->coefficient.value *=
                terms->coefficient.value;

            return term->simplify();
        }

        return std::make_shared<Term>(
                   terms->coefficient.value, term, std::make_shared<Constant>(1)
        )
            ->simplify();
    }

    std::map<Variable, std::shared_ptr<Token> *> variable_powers;

    std::int32_t i = 0;

    while (i < terms->terms.size()) {
        auto token = terms->terms[i]->simplify();

        if (typeid(*token) == typeid(Constant)) {
            auto const constant = std::dynamic_pointer_cast<Constant>(token);

            if (!constant->value)
                return std::make_shared<Constant>(0);

            terms->coefficient.value *= constant->value;
            terms->terms.erase(terms->terms.begin() + i);

            continue;
        }

        if (typeid(*token) == typeid(Variable)) {
            auto variable = std::dynamic_pointer_cast<Variable>(token);

            if (variable_powers.contains(*variable)) {
                auto const power = std::dynamic_pointer_cast<Expression>(
                    std::dynamic_pointer_cast<Term>(*variable_powers[*variable])
                        ->power
                );
                power->add_token(std::make_shared<Operation>(Operation::add));
                power->add_token(std::make_shared<Constant>(1));

                terms->terms.erase(terms->terms.begin() + 1);

                continue;
            }

            auto power = std::make_shared<Expression>();
            power->add_token(std::make_shared<Constant>(1));

            terms->terms[i] = std::make_shared<Term>(variable, power);
            variable_powers[*variable] = &terms->terms[i];

            ++i;

            continue;
        }

        if (typeid(*token) == typeid(Term)) {
            auto const term = std::dynamic_pointer_cast<Term>(token);
            terms->coefficient.value *= term->coefficient.value;
            term->coefficient.value = 1;

            if (typeid(*term->base) == typeid(Variable)) {
                auto variable = std::dynamic_pointer_cast<Variable>(term->base);

                if (variable_powers.contains(*variable)) {
                    auto const power = std::dynamic_pointer_cast<Expression>(
                        std::dynamic_pointer_cast<Term>(
                            *variable_powers[*variable]
                        )
                            ->power
                    );
                    power->add_token(std::make_shared<Operation>(Operation::add)
                    );
                    power->add_token(term->power);

                    terms->terms.erase(terms->terms.begin() + 1);

                    continue;
                }

                if (typeid(*term->power) != typeid(Expression)) {
                    auto const power = std::make_shared<Expression>();
                    power->add_token(term->power);
                    term->power = power;
                }

                terms->terms[i] = term;
                variable_powers[*variable] = &terms->terms[i];

                ++i;

                continue;
            }
        }

        terms->terms[i] = token;

        ++i;
    }

    for (auto const token : variable_powers | std::views::values) {
        if (auto const term = std::dynamic_pointer_cast<Term>(*token);
            std::dynamic_pointer_cast<Expression>(term->power)
                ->tokens.empty()) {
            *token = term->base;

            continue;
        }

        *token = (*token)->simplify();
    }

    if (terms->terms.size() == 1) {
        auto term = terms->terms[0];

        if (typeid(*term) == typeid(Constant))
            return std::make_shared<Constant>(
                terms->coefficient.value *
                std::dynamic_pointer_cast<Constant>(term)->value
            );

        if (typeid(*term) == typeid(Term)) {
            std::dynamic_pointer_cast<Term>(term)->coefficient.value *=
                terms->coefficient.value;

            return term->simplify();
        }

        return std::make_shared<Term>(
                   terms->coefficient.value, term, std::make_shared<Constant>(1)
        )
            ->simplify();
    }

    return terms;
}

std::shared_ptr<Token> Expression::simplify() {
    auto expression = std::make_shared<Expression>(*this);

    std::vector<std::shared_ptr<Token>> &tokens = expression->tokens;

    if (tokens.size() == 1)
        return expression->pop_token()->simplify();

    for (std::shared_ptr<Token> &token : tokens)
        if (typeid(*token) != typeid(Operation))
            token = token->simplify();

    int i;

    for (i = 1; i < tokens.size(); ++i) {
        std::shared_ptr<Token> &token = tokens[i];

        if (typeid(*token) != typeid(Operation))
            continue;

        if (i == tokens.size())
            throw std::invalid_argument("Expression is not valid!");

        std::shared_ptr<Token> left = tokens[i - 1];
        std::shared_ptr<Token> right = tokens[i + 1];

        auto operation = std::dynamic_pointer_cast<Operation>(token);

        if (!operation)
            continue;

        if (operation->operation == Operation::add) {
            if (typeid(*left) == typeid(Constant)) {
                if (auto left_constant =
                        std::dynamic_pointer_cast<Constant>(left);
                    left_constant->value == 0) {
                    --i;

                    tokens.erase(tokens.begin() + i);
                    tokens.erase(tokens.begin() + i);
                } else if (typeid(*right) == typeid(Constant)) {
                    auto right_constant =
                        std::dynamic_pointer_cast<Constant>(right);

                    right_constant->value += left_constant->value;

                    --i;

                    tokens.erase(tokens.begin() + i);
                    tokens.erase(tokens.begin() + i);
                }

                continue;
            }

            if (typeid(*right) == typeid(Constant)) {
                auto right_constant =
                    std::dynamic_pointer_cast<Constant>(right);

                if (right_constant->value > 0)
                    continue;

                if (right_constant->value != 0) {
                    right_constant->value = -right_constant->value;
                    operation->operation = Operation::sub;

                    continue;
                }

                tokens[i + 1] = left;

                --i;

                tokens.erase(tokens.begin() + i);
                tokens.erase(tokens.begin() + i);

                continue;
            }

            if (typeid(*right) == typeid(Terms)) {
                auto right_terms = std::dynamic_pointer_cast<Terms>(right);

                if (right_terms->coefficient.value >= 0)
                    continue;

                right_terms->coefficient.value =
                    -right_terms->coefficient.value;
                operation->operation = Operation::sub;

                continue;
            }

            if (typeid(*right) == typeid(Term)) {
                auto right_term = std::dynamic_pointer_cast<Term>(right);

                if (right_term->coefficient.value >= 0)
                    continue;

                right_term->coefficient.value = -right_term->coefficient.value;
                operation->operation = Operation::sub;

                continue;
            }
        } else if (operation->operation == Operation::sub) {
            if (typeid(*left) == typeid(Constant)) {
                if (typeid(*right) == typeid(Constant)) {
                    auto right_constant =
                        std::dynamic_pointer_cast<Constant>(right);

                    right_constant->value =
                        std::dynamic_pointer_cast<Constant>(left)->value -
                        right_constant->value;

                    --i;

                    tokens.erase(tokens.begin() + i);
                    tokens.erase(tokens.begin() + i);
                }

                continue;
            }

            if (typeid(*right) == typeid(Constant)) {
                auto right_constant =
                    std::dynamic_pointer_cast<Constant>(right);
                if (right_constant->value != 0)
                    continue;

                tokens[i + 1] = left;

                --i;

                tokens.erase(tokens.begin() + i);
                tokens.erase(tokens.begin() + i);
            }
        }
    }

    for (std::shared_ptr<Token> &token : tokens)
        if (typeid(*token) != typeid(Operation))
            token = token->simplify();

    if (tokens.size() == 1)
        return expression->pop_token();

    i = 0;

    while (i < tokens.size()) {
        std::shared_ptr<Token> &token = tokens[i];

        if (auto const &token_type = typeid(*token);
            token_type == typeid(Operation)) {
            auto const operation = std::dynamic_pointer_cast<Operation>(token);

            if (i == tokens.size() - 1)
                throw std::invalid_argument("Expression is not valid!");

            if (operation->operation == Operation::sub) {
                if (i != 0) {
                    ++i;

                    continue;
                }

                if (std::shared_ptr<Token> next = tokens[i + 1];
                    typeid(*next) == typeid(Constant)) {
                    auto const constant =
                        std::dynamic_pointer_cast<Constant>(next);

                    constant->value = -constant->value;
                } else if (typeid(*next) == typeid(Term)) {
                    auto const term = std::dynamic_pointer_cast<Term>(next);

                    term->coefficient.value = -term->coefficient.value;
                }
            }

            if (i == 0) {
                tokens.erase(tokens.begin());

                continue;
            }
        }

        ++i;
    }

    if (tokens.size() == 1)
        return expression->pop_token();

    return expression;
}