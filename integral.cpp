#include "token.h"

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

std::shared_ptr<Token> Function::integral(Variable const &variable) const {}

std::shared_ptr<Token> Term::integral(Variable const &variable) const {
    auto const &base_type = typeid(*this->base);
    auto const &power_type = typeid(*this->power);

    if (base_type == typeid(Variable) && power_type == typeid(Constant)) {
        auto power = std::dynamic_pointer_cast<Constant>(this->power);

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
}

std::shared_ptr<Token> Terms::integral(Variable const &variable) const {}

std::shared_ptr<Token> Expression::integral(Variable const &variable) const {
    auto const result = std::make_shared<Expression>();

    for (std::shared_ptr<Token> const &term : this->tokens) {
        if (auto const &token_type = typeid(*term);
            token_type == typeid(Operation)) {
            result->add_token(std::make_shared<Operation>(
                std::dynamic_pointer_cast<Operation>(term)->operation
            ));

            continue;
        }

        result->add_token(term->integral(variable));
    }

    return result->simplified();
}
