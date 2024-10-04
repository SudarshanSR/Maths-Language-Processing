#include "differentiate.h"

namespace {
struct Term {
    Constant *constant = nullptr;
    Variable *variable = nullptr;
    Token *power = nullptr;
};
} // namespace

Expression differentiate(Expression const &expression,
                         Variable const &variable) {
    Expression result{};

    std::vector<Token *> const &tokens = expression.tokens();

    for (int i = 0; i < tokens.size(); ++i) {
        Token *token = tokens[i];

        if (typeid(*token) == typeid(Operation)) {
            result.add_token(new Operation(static_cast<std::string>(
                *dynamic_cast<Operation *>(token))[0]));

            continue;
        }

        Term term{};

        while (i < tokens.size() && typeid(*token) != typeid(Operation)) {
            if (typeid(*token) == typeid(Constant)) {
                if (term.constant)
                    throw std::invalid_argument("Expression is not valid!");

                term.constant = dynamic_cast<Constant *>(token);
            } else if (typeid(*token) == typeid(Variable)) {
                if (term.variable)
                    throw std::invalid_argument("Expression is not valid!");

                term.variable = dynamic_cast<Variable *>(token);
            }

            token = tokens[++i];
        }

        if (i < tokens.size()) {
            if (auto const *operation = dynamic_cast<Operation *>(token))
                if (operation->operation == Operation::op::pow) {
                    try {
                        token = tokens.at(++i);
                    } catch (std::out_of_range const &) {
                        throw std::invalid_argument("Expression is not valid!");
                    }

                    term.power = token;
                } else {
                    --i;
                }

            else
                --i;
        }

        if (!term.variable || term.variable->var != variable.var) {
            result.add_token(new Constant(0));

            continue;
        }

        Constant constant{1};

        if (term.constant)
            constant = *term.constant;

        auto *sub = new Expression;

        if (!term.power) {
            sub->add_token(new Constant(constant.value));
        } else {
            if (typeid(*term.power) == typeid(Constant)) {
                long double const power =
                    dynamic_cast<Constant *>(term.power)->value;

                sub->add_token(new Constant(constant.value * power));

                if (power == 1)
                    continue;

                sub->add_token(new Variable(term.variable->var));

                if (power != 2) {
                    sub->add_token(new Operation('^'));
                    sub->add_token(new Constant(power - 1));
                }
            } else if (typeid(*term.power) == typeid(Variable)) {
                if (auto const *power = dynamic_cast<Variable *>(term.power);
                    power->var != variable.var) {
                    // TODO -> MAKE VAR A CONST
                } else {
                    // TODO -> CHAIN POWER RULE
                }
            } else {
                // TODO -> CHAIN POWER RULE
                // sub->add_token(new Constant(constant.value));
                // sub->add_token(new Operation('*'));
                // sub->add_token(
                //     new Expression(differentiate(*term.power, variable)));
                // sub->add_token(new Operation('*'));
                // sub->add_token(new Variable(term.variable->var));
            }
        }

        result.add_token(sub);
    }

    return result;
}