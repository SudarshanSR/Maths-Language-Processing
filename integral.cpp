#include "token.h"

#include <map>

namespace {
std::map<std::string, std::string> k_function_map{
    {"sin", "-cos({0})"},
    {"cos", "sin({0})"},
    // {"tan", "ln(sec({0}))"},
    // {"sec", "ln(sec({0}) + tan({0}))"},
    // {"csc", "ln(csc({0}) - cot({0}))"},
    // {"cot", "ln(sin({0}))"},
    {"sinh", "cosh({0})"},
    {"cosh", "sinh({0})"},
    // {"tanh", "ln(cosh({0}))"},
    {"sech", "atan(sinh({0}))"},
    {"csch", "acoth(cosh({0}))"},
    // {"coth", "ln(sinh({0}))"},
    {"ln", "{0}ln({0}) - {0}"},
    {"asin", "{0}asin({0}) + (1 - {0}^2)^0.5"},
    {"acos", "{0}acos({0}) - (1 - {0}^2)^0.5"},
    {"atan", "{0}atan({0}) - ln(1 + {0}^2) / 2"},
    {"asec", "{0}asec({0}) - ln({0} + ({0}^2 - 1)^0.5)"},
    {"acsc", "{0}acsc({0}) + ln({0} + ({0}^2 - 1)^0.5)"},
    {"acot", "{0}acot({0}) + ln(1 + {0}^2) / 2"},
};
} // namespace

std::shared_ptr<Token> Constant::integral(Variable const &variable) const {
    return std::make_shared<Term>(
        this->value, std::make_shared<Variable>(variable),
        std::make_shared<Constant>(1)
    );
}

std::shared_ptr<Token> Variable::integral(Variable const &variable) const {
    if (*this == variable)
        return std::make_shared<Term>(
            0.5, std::make_shared<Variable>(variable),
            std::make_shared<Constant>(2)
        );

    auto terms = std::make_shared<Terms>();
    terms->add_term(std::make_shared<Variable>(variable));
    terms->add_term(std::make_shared<Variable>(*this));

    return terms;
}

std::shared_ptr<Token> Operation::integral(Variable const &variable) const {
    throw std::runtime_error("Operator cannot be integrated!");
}

std::shared_ptr<Token> Function::integral(Variable const &variable) const {
    if (typeid(*this->parameter) == typeid(Variable)) {
        auto const parameter =
            std::dynamic_pointer_cast<Variable>(this->parameter);

        auto const terms = std::make_shared<Terms>();

        if (*parameter != variable) {
            terms->add_term(std::make_shared<Variable>(variable));
            terms->add_term(std::make_shared<Function>(*this));
        } else {
            auto string = static_cast<std::string>(*this->parameter);

            terms->add_term(tokenise(std::vformat(
                k_function_map.at(this->function), std::make_format_args(string)
            )));
        }

        return terms->simplified();
    }

    throw std::runtime_error("Expression is not integrable!");
}

std::shared_ptr<Token> Term::integral(Variable const &variable) const {
    auto const &base_type = typeid(*this->base);
    auto const &power_type = typeid(*this->power);

    if (base_type == typeid(Variable) && power_type == typeid(Constant)) {
        auto const power = std::dynamic_pointer_cast<Constant>(this->power);

        if (power->value == -1) {
            return std::make_shared<Term>(
                this->coefficient.value,
                std::make_shared<Function>("ln", this->base),
                std::make_shared<Constant>(1)
            );
        }

        return std::make_shared<Term>(
            this->coefficient.value / (power->value + 1), this->base,
            std::make_shared<Constant>(power->value + 1)
        );
    }

    throw std::runtime_error("Expression is not integrable!");
}

std::shared_ptr<Token> Terms::integral(Variable const &variable) const {
    throw std::runtime_error("Expression is not integrable!");
}

std::shared_ptr<Token> Expression::integral(Variable const &variable) const {
    auto const result = std::make_shared<Expression>();

    for (std::shared_ptr<Token> const &term : this->tokens) {
        if (auto const &token_type = typeid(*term);
            token_type == typeid(Operation)) {
            result->add_token(term);

            continue;
        }

        result->add_token(term->integral(variable));
    }

    return result->simplified();
}
