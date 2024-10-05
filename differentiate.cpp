#include "differentiate.h"

#include <map>

namespace {
std::map<std::string,
         std::vector<std::tuple<std::string, long double, long double>>>
    k_function_map{
        {"sin", {{"cos", 1, 1}}},
        {"cos", {{"sin", -1, 1}}},
        {"tan", {{"sec", 1, 2}}},
        {"sec", {{"sec", 1, 1}, {"tan", 1, 1}}},
        {"csc", {{"csc", -1, 1}, {"cot", 1, 1}}},
        {"cot", {{"csc", -1, 2}}},
        {"sinh", {{"cosh", 1, 1}}},
        {"cosh", {{"sinh", 1, 1}}},
        {"tanh", {{"sech", 1, 2}}},
        {"sech", {{"sech", 1, 1}, {"tanh", 1, 1}}},
        {"csch", {{"csch", -1, 1}, {"coth", 1, 1}}},
        {"coth", {{"csch", -1, 2}}},
        {"ln", {{"", 1, -1}}},
        {"e^", {{"e^", 1, 1}}},
    };

Term get_next_term(std::vector<Token *> const &tokens, int &i) {
    Term term{};

    Token *token = tokens[i];

    while (i < tokens.size() && typeid(*token) != typeid(Operation)) {
        if (typeid(*token) == typeid(Constant)) {
            if (term.coefficient)
                throw std::invalid_argument("Expression is not valid!");

            term.coefficient = dynamic_cast<Constant *>(token);
        } else if (typeid(*token) == typeid(Variable) ||
                   typeid(*token) == typeid(Expression) ||
                   typeid(*token) == typeid(Function)) {
            if (term.base)
                throw std::invalid_argument("Expression is not valid!");

            term.base = token;
        }

        token = tokens[++i];
    }

    return term;
}
} // namespace

Expression *differentiate(Term const &term, Variable const &variable) {
    auto *result = new Expression;

    if (!term.base) {
        result->add_token(new Constant(0));

        return result;
    }

    if (auto const *var = dynamic_cast<Variable *>(term.base);
        var && var->var != variable.var) {
        result->add_token(new Constant(0));

        return result;
    }

    Constant c{1};

    if (term.coefficient)
        c = *term.coefficient;

    if (!term.power) {
        if (c.value != 1 || dynamic_cast<Variable *>(term.base))
            result->add_token(new Constant(c.value));

        if (auto const *base = dynamic_cast<Expression *>(term.base))
            result->add_token(differentiate(*base, variable));

        else if (auto const *base = dynamic_cast<Function *>(term.base))
            result->add_token(differentiate(*base, variable));
    } else {
        if (auto const *p = dynamic_cast<Constant *>(term.power)) {
            long double const power = p->value;

            if (long double const coefficient = c.value * power;
                coefficient != 1 ||
                (power == 1 && dynamic_cast<Variable *>(term.base)))
                result->add_token(new Constant(c.value * power));

            if (power == 1) {
                if (auto const *base = dynamic_cast<Expression *>(term.base))
                    result->add_token(differentiate(*base, variable));

                else if (auto const *base = dynamic_cast<Function *>(term.base))
                    result->add_token(differentiate(*base, variable));

                return result;
            }

            result->add_token(copy(term.base));

            if (power != 2) {
                result->add_token(new Operation('^'));
                result->add_token(new Constant(power - 1));
            }

            if (auto const *base = dynamic_cast<Expression *>(term.base)) {
                result->add_token(new Operation('*'));
                result->add_token(differentiate(*base, variable));
            } else if (auto const *base = dynamic_cast<Function *>(term.base)) {
                result->add_token(new Operation('*'));
                result->add_token(differentiate(*base, variable));
            }
        } else if (auto *power = dynamic_cast<Variable *>(term.power)) {
            if (power->var != variable.var) {
                auto *coefficient = new Expression;

                if (c.value == 1)
                    coefficient->add_token(copy(&c));

                coefficient->add_token(copy(power));

                result->add_token(coefficient);

                result->add_token(copy(term.base));

                result->add_token(new Operation('^'));

                auto *new_power = new Expression;
                new_power->add_token(new Variable(power->var));
                new_power->add_token(new Operation('-'));
                new_power->add_token(new Constant(1));

                result->add_token(new_power);

                if (auto const *base = dynamic_cast<Expression *>(term.base)) {
                    result->add_token(new Operation('*'));
                    result->add_token(differentiate(*base, variable));
                } else if (auto const *base =
                               dynamic_cast<Function *>(term.base)) {
                    result->add_token(new Operation('*'));
                    result->add_token(differentiate(*base, variable));
                }
            } else {
                if (c.value != 1)
                    result->add_token(copy(&c));

                auto *expression_1 = new Expression;
                expression_1->add_token(copy(term.base));
                expression_1->add_token(new Operation('^'));
                expression_1->add_token(copy(term.power));

                result->add_token(expression_1);
                result->add_token(new Operation('*'));

                auto *expression_2 = new Expression;
                result->add_token(expression_2);

                if (auto *base = dynamic_cast<Constant *>(term.base)) {
                    expression_2->add_token(new Function(
                        "ln", new Constant(1), copy(base), new Constant(1)));

                    return result;
                }

                expression_2->add_token(copy(term.power));

                if (auto const *base = dynamic_cast<Expression *>(term.base)) {
                    expression_2->add_token(new Operation('*'));
                    expression_2->add_token(differentiate(*base, variable));
                } else if (auto const *base =
                               dynamic_cast<Function *>(term.base)) {
                    expression_2->add_token(new Operation('*'));
                    expression_2->add_token(differentiate(*base, variable));
                } else if (auto *base = dynamic_cast<Variable *>(term.base)) {
                    if (base->var != variable.var) {
                        expression_2->add_token(new Operation('*'));

                        expression_2->add_token(copy(base));
                    }
                }

                expression_2->add_token(new Operation('/'));
                expression_2->add_token(copy(term.base));

                expression_2->add_token(new Operation('+'));

                expression_2->add_token(new Function(
                    "ln", new Constant(1), copy(term.base), new Constant(1)));
            }
        } else {
            if (c.value != 1)
                result->add_token(copy(&c));

            auto *expression_1 = new Expression;
            expression_1->add_token(copy(term.base));
            expression_1->add_token(new Operation('^'));
            expression_1->add_token(copy(term.power));

            result->add_token(expression_1);
            result->add_token(new Operation('*'));

            auto *expression_2 = new Expression;
            result->add_token(expression_2);

            if (auto *base = dynamic_cast<Constant *>(term.base)) {
                expression_2->add_token(new Function(
                    "ln", new Constant(1), copy(base), new Constant(1)));

                return result;
            }

            expression_2->add_token(copy(term.power));

            if (auto const *base = dynamic_cast<Expression *>(term.base)) {
                expression_2->add_token(new Operation('*'));
                expression_2->add_token(differentiate(*base, variable));
            } else if (auto const *base = dynamic_cast<Function *>(term.base)) {
                expression_2->add_token(new Operation('*'));
                expression_2->add_token(differentiate(*base, variable));
            } else if (auto *base = dynamic_cast<Variable *>(term.base)) {
                if (base->var != variable.var) {
                    expression_2->add_token(new Operation('*'));

                    expression_2->add_token(copy(base));
                }
            }

            expression_2->add_token(new Operation('/'));
            expression_2->add_token(copy(term.base));

            expression_2->add_token(new Operation('+'));

            if (auto const *power = dynamic_cast<Expression *>(term.power)) {
                expression_2->add_token(differentiate(*power, variable));
                expression_2->add_token(new Operation('*'));
            } else if (auto const *power =
                           dynamic_cast<Function *>(term.power)) {
                expression_2->add_token(differentiate(*power, variable));
                expression_2->add_token(new Operation('*'));
            }

            expression_2->add_token(new Function(
                "ln", new Constant(1), copy(term.base), new Constant(1)));
        }
    }

    return result;
}

Expression *differentiate(Function const &function, Variable const &variable) {
    auto *result = new Expression;

    if (auto const *p = dynamic_cast<Variable *>(function.parameter)) {
        if (p->var != variable.var) {
            result->add_token(new Constant(0));

            return result;
        }
    }

    if (auto functions = k_function_map[function.function];
        functions.size() == 1) {
        auto const &[name, coefficient, power] = functions[0];

        result->add_token(new Function(name, new Constant(coefficient),
                                       function.parameter,
                                       new Constant(power)));
    } else {
        auto *expression = new Expression;

        for (int i = 0; i < functions.size() - 1; ++i) {
            auto const &[name, coefficient, power] = functions[i];

            expression->add_token(new Function(name, new Constant(coefficient),
                                               function.parameter,
                                               new Constant(power)));
            expression->add_token(new Operation('*'));
        }

        auto const &[name, coefficient, power] = functions.back();

        expression->add_token(new Function(name, new Constant(coefficient),
                                           function.parameter,
                                           new Constant(power)));

        result->add_token(expression);
    }

    if (typeid(*function.parameter) != typeid(Variable)) {
        result->add_token(new Operation('*'));

        if (auto const *p = dynamic_cast<Expression *>(function.parameter))
            result->add_token(differentiate(*p, variable));

        else if (auto const *p = dynamic_cast<Term *>(function.parameter))
            result->add_token(differentiate(*p, variable));
    }

    return result;
}

Expression *differentiate(Expression const &expression,
                          Variable const &variable) {
    auto *result = new Expression;

    std::vector<Token *> const &tokens = expression.tokens();

    for (int i = 0; i < tokens.size(); ++i) {
        if (Token *token = tokens[i]; typeid(*token) == typeid(Operation)) {
            result->add_token(new Operation(static_cast<std::string>(
                *dynamic_cast<Operation *>(token))[0]));

            continue;
        }

        Term term = get_next_term(tokens, i);

        if (i < tokens.size() - 1) {
            if (auto const *operation = dynamic_cast<Operation *>(tokens[i])) {
                if (operation->operation == Operation::op::mul) {
                    auto *v = new Expression(get_next_term(tokens, ++i));
                    auto *u = new Expression(term);

                    auto *u_d = differentiate(term, variable);
                    auto *v_d = differentiate(*v, variable);

                    result->add_token(u);
                    result->add_token(new Operation('*'));
                    result->add_token(v_d);

                    result->add_token(new Operation('+'));

                    result->add_token(u_d);
                    result->add_token(new Operation('*'));
                    result->add_token(v);

                    continue;
                }

                if (operation->operation == Operation::op::div) {
                    auto *v = new Expression(get_next_term(tokens, ++i));
                    auto *u = new Expression(term);

                    auto *u_d = differentiate(term, variable);
                    auto *v_d = differentiate(*v, variable);

                    auto *numerator = new Expression;

                    numerator->add_token(u_d);
                    numerator->add_token(new Operation('*'));
                    numerator->add_token(v);

                    numerator->add_token(new Operation('-'));

                    numerator->add_token(u);
                    numerator->add_token(new Operation('*'));
                    numerator->add_token(v_d);

                    auto *denominator = new Expression;
                    denominator->add_token(v);
                    denominator->add_token(new Operation('^'));
                    denominator->add_token(new Constant(2));

                    result->add_token(numerator);
                    result->add_token(new Operation('/'));
                    result->add_token(denominator);

                    continue;
                }

                if (operation->operation != Operation::op::pow) {
                    --i;

                    result->add_token(differentiate(term, variable));

                    continue;
                }

                try {
                    term.power = tokens.at(++i);
                } catch (std::out_of_range const &) {
                    throw std::invalid_argument("Expression is not valid!");
                }

                if (!term.base) {
                    if (term.coefficient) {
                        term.base = term.coefficient;
                        term.coefficient = nullptr;
                    } else {
                        term.base = new Constant(1);
                    }
                }
            } else {
                --i;
            }
        }

        result->add_token(differentiate(term, variable));
    }

    return result;
}