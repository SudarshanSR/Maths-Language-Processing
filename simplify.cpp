#include "simplify.h"
#include "token.h"

#include <cmath>

namespace {
std::shared_ptr<Token> simplify(std::shared_ptr<Terms> const &terms) {
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

            return simplify(term);
        }

        return simplify(std::make_shared<Term>(
            terms->coefficient.value, term, std::make_shared<Constant>(1)
        ));
    }

    int i = 0;

    while (i < terms->terms.size()) {
        auto token = simplify(terms->terms[i]);

        if (typeid(*token) == typeid(Constant)) {
            auto const constant = std::dynamic_pointer_cast<Constant>(token);

            if (!constant->value)
                return std::make_shared<Constant>(0);

            terms->coefficient.value *= constant->value;
            terms->terms.erase(terms->terms.begin() + i);

            continue;
        }

        if (typeid(*token) == typeid(Term)) {
            auto const term = std::dynamic_pointer_cast<Term>(token);
            terms->coefficient.value *= term->coefficient.value;
            term->coefficient.value = 1;

            terms->terms[i] = term;
        } else {
            terms->terms[i] = token;
        }

        ++i;
    }

    return terms;
}

std::shared_ptr<Token> simplify(std::shared_ptr<Term> const &term) {
    if (!term->power)
        term->power = std::make_shared<Constant>(1);

    term->base = simplify(term->base);
    term->power = simplify(term->power);

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

    term->base = simplify(term->base);
    term->power = simplify(term->power);

    return term;
}

std::shared_ptr<Token> simplify(std::shared_ptr<Expression> expression) {
    std::vector<std::shared_ptr<Token>> &tokens = expression->tokens;

    if (tokens.size() == 1)
        return simplify(expression->pop_token());

    for (std::shared_ptr<Token> &token : tokens)
        token = simplify(token);

    int i;

    for (i = 1; i < tokens.size(); ++i) {
        std::shared_ptr<Token> &token = tokens[i];

        if (typeid(*token) != typeid(Operation))
            continue;

        if (i == tokens.size())
            throw std::invalid_argument("Expression is not valid!");

        if (auto operation = std::dynamic_pointer_cast<Operation>(token);
            operation->operation != Operation::pow)
            continue;

        std::shared_ptr<Token> left = tokens[i - 1];
        std::shared_ptr<Token> right = tokens[i + 1];

        if (typeid(*left) == typeid(Constant)) {
            auto left_constant = std::dynamic_pointer_cast<Constant>(left);

            if (left_constant->value == 0 || left_constant->value == 1) {
                tokens.erase(tokens.begin() + i);
                tokens.erase(tokens.begin() + i);

                --i;

                continue;
            }

            if (typeid(*right) == typeid(Constant)) {
                auto right_constant =
                    std::dynamic_pointer_cast<Constant>(right);

                left_constant->value =
                    std::powl(left_constant->value, right_constant->value);

                tokens.erase(tokens.begin() + i);
                tokens.erase(tokens.begin() + i);

                --i;

                continue;
            }
        }

        if (typeid(*right) == typeid(Constant)) {
            auto right_constant = std::dynamic_pointer_cast<Constant>(right);

            if (right_constant->value == 1) {
                tokens.erase(tokens.begin() + i);
                tokens.erase(tokens.begin() + i);

                --i;

                continue;
            }

            if (right_constant->value == 0) {
                --i;

                tokens.erase(tokens.begin() + i);
                tokens.erase(tokens.begin() + i);

                right_constant->value = 1;

                continue;
            }
        }

        auto term = std::make_shared<Term>(1, left, right);

        tokens[i - 1] = term;

        tokens.erase(tokens.begin() + i);
        tokens.erase(tokens.begin() + i);

        --i;
    }

    for (i = 1; i < tokens.size(); ++i) {
        std::shared_ptr<Token> &token = tokens[i];

        if (typeid(*token) != typeid(Operation))
            continue;

        if (i == tokens.size())
            throw std::invalid_argument("Expression is not valid!");

        auto operation = std::dynamic_pointer_cast<Operation>(token);

        if (operation->operation == Operation::add ||
            operation->operation == Operation::sub ||
            operation->operation == Operation::div)
            continue;

        std::shared_ptr<Token> left = tokens[i - 1];
        std::shared_ptr<Token> right = tokens[i + 1];

        if (operation->operation == Operation::mul) {
            if (typeid(*left) == typeid(Constant)) {
                auto left_constant = std::dynamic_pointer_cast<Constant>(left);

                if (left_constant->value == 0) {
                    tokens.erase(tokens.begin() + i);
                    tokens.erase(tokens.begin() + i);

                    --i;

                    continue;
                }

                if (left_constant->value == 1) {
                    --i;

                    tokens.erase(tokens.begin() + i);
                    tokens.erase(tokens.begin() + i);

                    continue;
                }

                if (typeid(*right) == typeid(Constant)) {
                    std::dynamic_pointer_cast<Constant>(right)->value *=
                        left_constant->value;
                } else if (typeid(*right) == typeid(Term)) {
                    std::dynamic_pointer_cast<Term>(right)->coefficient.value *=
                        left_constant->value;
                } else {
                    tokens[i + 1] = std::make_shared<Term>(
                        left_constant->value, right,
                        std::make_shared<Constant>(1)
                    );
                }
            } else if (typeid(*right) == typeid(Constant)) {
                auto right_constant =
                    std::dynamic_pointer_cast<Constant>(right);

                if (right_constant->value == 0) {
                    --i;

                    tokens.erase(tokens.begin() + i);
                    tokens.erase(tokens.begin() + i);

                    continue;
                }

                if (right_constant->value == 1) {
                    tokens.erase(tokens.begin() + i);
                    tokens.erase(tokens.begin() + i);

                    --i;

                    continue;
                }

                if (typeid(*left) == typeid(Term)) {
                    std::dynamic_pointer_cast<Term>(left)->coefficient.value *=
                        right_constant->value;

                    tokens[i + 1] = left;
                } else if (typeid(*left) == typeid(Variable)) {
                    tokens[i + 1] = std::make_shared<Term>(
                        right_constant->value,
                        std::dynamic_pointer_cast<Variable>(left),
                        std::make_shared<Constant>(1)
                    );
                } else {
                    tokens[i + 1] = std::make_shared<Term>(
                        right_constant->value, left,
                        std::make_shared<Constant>(1)
                    );
                }
            } else {
                if (tokens.size() == 3)
                    continue;

                auto result = std::make_shared<Terms>();
                result->add_term(left);
                result->add_term(right);

                tokens[i + 1] = result;
            }
        } else if (operation->operation == Operation::div) {
            if (typeid(*left) == typeid(Constant)) {
                auto left_constant = std::dynamic_pointer_cast<Constant>(left);

                if (left_constant->value == 0) {
                    tokens.erase(tokens.begin() + i);
                    tokens.erase(tokens.begin() + i);

                    --i;

                    continue;
                }

                tokens[i + 1] = std::make_shared<Term>(
                    left_constant->value, right, std::make_shared<Constant>(-1)
                );

                --i;

                tokens.erase(tokens.begin() + i);
                tokens.erase(tokens.begin() + i);

                --i;

                continue;
            }

            auto result = std::make_shared<Terms>();
            result->add_term(left);
            result->add_term(
                std::make_shared<Term>(right, std::make_shared<Constant>(-1))
            );

            tokens[i + 1] = result;
        }

        --i;

        tokens.erase(tokens.begin() + i);
        tokens.erase(tokens.begin() + i);
    }

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
        token = simplify(token);

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
                if (std::shared_ptr<Token> next = tokens[i + 1];
                    typeid(*next) == typeid(Constant)) {
                    auto const constant =
                        std::dynamic_pointer_cast<Constant>(next);

                    constant->value = -constant->value;

                    operation->operation = Operation::add;
                } else if (typeid(*next) == typeid(Term)) {
                    auto const term = std::dynamic_pointer_cast<Term>(next);

                    term->coefficient.value = -term->coefficient.value;

                    operation->operation = Operation::add;
                }

                if (i == 0) {
                    tokens.erase(tokens.begin());

                    continue;
                }
            }

            if (i == 0 && operation->operation == Operation::add) {
                tokens.erase(tokens.begin());

                continue;
            }
        } else if (token_type == typeid(Constant)) {
            auto const constant = std::dynamic_pointer_cast<Constant>(token);

            if (i == tokens.size() - 1) {
                ++i;

                continue;
            }

            if (constant->value == 0) {
                if (typeid(*tokens[i + 1]) != typeid(Operation)) {
                    ++i;

                    continue;
                }

                auto const operation =
                    std::dynamic_pointer_cast<Operation>(tokens[i + 1]);

                // Simplifies 0 / expr

                if (operation->operation != Operation::div) {
                    tokens.erase(tokens.begin() + i);

                    continue;
                }

                do {
                    tokens.erase(tokens.begin() + i);

                    if (tokens.empty())
                        break;

                    if (i == tokens.size()) {
                        tokens.pop_back();

                        break;
                    }

                    if (typeid(*tokens[i]) != typeid(Operation))
                        continue;

                    if (auto const op =
                            std::dynamic_pointer_cast<Operation>(tokens[i]);
                        op->operation == Operation::add ||
                        op->operation == Operation::sub)
                        break;
                } while (i < tokens.size());

                if (i == tokens.size() - 1 &&
                    typeid(*tokens[i]) == typeid(Operation)) {
                    tokens.erase(tokens.begin() + i);
                }

                continue;
            }

            if (typeid(*tokens[i + 1]) == typeid(Operation)) {
                auto const operation =
                    std::dynamic_pointer_cast<Operation>(tokens[i + 1]);

                if (operation->operation == Operation::div) {
                    if (typeid(*tokens[i + 2]) == typeid(Constant)) {
                        auto const c =
                            std::dynamic_pointer_cast<Constant>(tokens[i + 2]);

                        c->value = constant->value / c->value;

                        tokens.erase(tokens.begin() + i);
                        tokens.erase(tokens.begin() + i);

                        continue;
                    }

                    if (typeid(*tokens[i + 2]) == typeid(Term)) {
                        auto const term =
                            std::dynamic_pointer_cast<Term>(tokens[i + 2]);

                        term->coefficient.value /= constant->value;

                        if (auto const &power_type = typeid(*term->power);
                            power_type == typeid(Constant)) {
                            auto power =
                                std::dynamic_pointer_cast<Constant>(term->power
                                );

                            power->value = -power->value;
                        } else if (power_type == typeid(Term)) {
                            auto power =
                                std::dynamic_pointer_cast<Term>(term->power);

                            power->coefficient.value =
                                -power->coefficient.value;
                        } else {
                            auto const p = std::make_shared<Term>();
                            p->coefficient.value = -1;
                            p->base = term->power;

                            term->power = p;
                        }

                        tokens.erase(tokens.begin() + i);
                        tokens.erase(tokens.begin() + i);

                        continue;
                    }

                    token = std::make_shared<Term>(
                        std::dynamic_pointer_cast<Constant>(tokens[i])->value,
                        tokens[i + 2], std::make_shared<Constant>(-1)
                    );

                    tokens.erase(tokens.begin() + i + 1);
                    tokens.erase(tokens.begin() + i + 1);
                }
            }
        } else if (token_type == typeid(Term)) {
            auto const term = std::dynamic_pointer_cast<Term>(token);

            if (i == tokens.size() - 1 ||
                typeid(*tokens[i + 1]) != typeid(Operation)) {
                ++i;

                continue;
            }

            auto operation =
                std::dynamic_pointer_cast<Operation>(tokens[i + 1]);

            if (operation->operation == Operation::pow) {
                if (!term->power) {
                    term->power = tokens[i + 2];
                } else {
                    if (auto const &power_type = typeid(*term->power);
                        power_type == typeid(Constant) &&
                        typeid(*tokens[i + 1]) == typeid(Constant)) {
                        std::dynamic_pointer_cast<Constant>(term->power)
                            ->value *=
                            std::dynamic_pointer_cast<Constant>(tokens[i + 2])
                                ->value;
                    } else if (power_type == typeid(Constant)) {
                        auto power = std::make_shared<Term>(
                            std::dynamic_pointer_cast<Constant>(term->power)
                                ->value,
                            tokens[i + 2], std::make_shared<Constant>(1)
                        );

                        term->power = power;
                    } else if (typeid(*tokens[i + 1]) == typeid(Constant)) {
                        auto power = std::make_shared<Term>(
                            std::dynamic_pointer_cast<Constant>(tokens[i + 2])
                                ->value,
                            term->power, std::make_shared<Constant>(1)
                        );

                        term->power = power;
                    } else {
                        auto power = std::make_shared<Terms>();
                        power->add_term(term->power);
                        power->add_term(tokens[i + 2]);

                        term->power = power;
                    }
                }

                tokens.erase(tokens.begin() + i + 1);
                tokens.erase(tokens.begin() + i + 1);
            } else if (operation->operation == Operation::mul && typeid(*tokens[i + 2]) == typeid(Constant)) {
                term->coefficient.value *=
                    std::dynamic_pointer_cast<Constant>(tokens[i + 2])->value;

                tokens.erase(tokens.begin() + i + 1);
                tokens.erase(tokens.begin() + i + 1);
            } else if (operation->operation == Operation::div && typeid(*tokens[i + 2]) == typeid(Constant)) {
                term->coefficient.value /=
                    std::dynamic_pointer_cast<Constant>(tokens[i + 2])->value;

                tokens.erase(tokens.begin() + i + 1);
                tokens.erase(tokens.begin() + i + 1);
            }
        }

        ++i;
    }

    if (tokens.size() == 1)
        return expression->pop_token();

    return expression;
}

std::shared_ptr<Function> simplify(std::shared_ptr<Function> function) {
    function->parameter = simplify(function->parameter);

    return function;
}
} // namespace

std::shared_ptr<Token> simplify(std::shared_ptr<Token> const &token) {
    if (typeid(*token) == typeid(Expression))
        return simplify(std::dynamic_pointer_cast<Expression>(token));

    if (typeid(*token) == typeid(Terms))
        return simplify(std::dynamic_pointer_cast<Terms>(token));

    if (typeid(*token) == typeid(Term))
        return simplify(std::dynamic_pointer_cast<Term>(token));

    if (typeid(*token) == typeid(Function))
        return simplify(std::dynamic_pointer_cast<Function>(token));

    return token;
}