#include "token.h"

#include <sstream>

gsl::owner<mlp::Expression *> mlp::Expression::clone() const {
    auto *expression = new Expression();

    for (auto const &[operation, token] : this->tokens)
        expression->add_token(operation, OwnedToken(token->clone()));

    return expression;
}

void mlp::Expression::add_token(Sign const sign, OwnedToken &&token) {
    if (sign == Sign::pos)
        *this += std::move(token);

    else
        *this -= std::move(token);
}

std::pair<mlp::Sign, mlp::OwnedToken> mlp::Expression::pop_token() {
    auto token = std::move(this->tokens.back());

    this->tokens.pop_back();

    return token;
}

bool mlp::Expression::empty() const { return this->tokens.empty(); }

mlp::Expression::operator std::string() const {
    std::stringstream result;

    if (this->tokens.size() == 1) {
        result << this->tokens[0].first
               << static_cast<std::string>(*this->tokens[0].second);
    } else {
        result << '(';

        for (auto const &[operation, token] : this->tokens)
            result << operation << static_cast<std::string>(*token);

        result << ')';
    }

    return result.str();
}

mlp::Expression &mlp::Expression::operator+=(OwnedToken &&token) {
    this->tokens.emplace_back(Sign::pos, std::move(token));

    return *this;
}

mlp::Expression &mlp::Expression::operator-=(OwnedToken &&token) {
    this->tokens.emplace_back(Sign::neg, std::move(token));

    return *this;
}