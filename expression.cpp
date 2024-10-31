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
    if (typeid(*token) == typeid(Expression)) {
        for (auto &[sign, t] : dynamic_cast<Expression &>(*token).tokens)
            if (sign == Sign::pos)
                *this += std::move(t);
            else
                *this -= std::move(t);

        return *this;
    }

    if (typeid(*token) == typeid(Constant)) {
        auto const &constant = dynamic_cast<Constant const &>(*token);

        if (constant == 0)
            return *this;

        for (std::size_t i = 0; i < this->tokens.size(); ++i) {
            auto &[sign, t] = this->tokens[i];

            if (typeid(*t) != typeid(Constant))
                continue;

            auto &c = dynamic_cast<Constant &>(*t);

            if (sign == Sign::neg) {
                c = -c;
                sign = Sign::pos;
            }

            c += constant;

            if (c == 0)
                this->tokens.erase(this->tokens.begin() + i);

            else if (c < 0) {
                c = -c;
                sign = Sign::neg;
            }

            return *this;
        }
    }

    if (typeid(*token) == typeid(Variable)) {
        auto const &variable = dynamic_cast<Variable const &>(*token);

        for (std::size_t i = 0; i < this->tokens.size(); ++i) {
            auto &[sign, t] = this->tokens[i];
            if (typeid(*t) == typeid(Variable)) {
                if (auto const &v = dynamic_cast<Variable const &>(*t);
                    v != variable)
                    continue;

                if (sign == Sign::neg) {
                    this->tokens.erase(this->tokens.begin() + i);
                } else {
                    t = std::make_unique<Term>(2 * (variable ^ 1));
                }

                return *this;
            }

            if (typeid(*t) == typeid(Term)) {
                auto &term = dynamic_cast<Term &>(*t);

                if (typeid(*term.base) != typeid(Variable) ||
                    dynamic_cast<Variable const &>(*term.base) != variable ||
                    typeid(*term.power) != typeid(Constant) ||
                    dynamic_cast<Constant const &>(*term.power) != 1)
                    continue;

                ++term.coefficient;

                if (term.coefficient == 0)
                    this->tokens.erase(this->tokens.begin() + i);

                return *this;
            }
        }

        this->tokens.emplace_back(Sign::pos, std::move(token));

        return *this;
    }

    if (typeid(*token) == typeid(Term)) {
        auto &term = dynamic_cast<Term &>(*token);

        if (term.coefficient == 0)
            return *this;

        if (term.coefficient < 0) {
            term.coefficient = -term.coefficient;

            return *this -= std::move(token);
        }

        if (typeid(*term.base) != typeid(Variable) ||
            typeid(*term.power) != typeid(Constant) ||
            dynamic_cast<Constant const &>(*term.power) != 1) {
            this->tokens.emplace_back(Sign::pos, std::move(token));

            return *this;
        }

        auto const &variable = dynamic_cast<Variable const &>(*term.base);

        for (std::size_t i = 0; i < this->tokens.size(); ++i) {
            auto &[sign, t] = this->tokens[i];
            if (typeid(*t) == typeid(Variable)) {
                if (auto const &v = dynamic_cast<Variable const &>(*t);
                    v != variable)
                    continue;

                t = std::make_unique<Term>(
                    (1 + term.coefficient) * (variable ^ 1)
                );

                if (term.coefficient == 0)
                    this->tokens.erase(this->tokens.begin() + i);

                return *this;
            }

            if (typeid(*t) == typeid(Term)) {
                auto &term_ = dynamic_cast<Term &>(*t);

                if (typeid(*term_.base) != typeid(Variable) ||
                    dynamic_cast<Variable const &>(*term_.base) != variable ||
                    typeid(*term_.power) != typeid(Constant) ||
                    dynamic_cast<Constant const &>(*term_.power) != 1)
                    continue;

                term_.coefficient += term.coefficient;

                if (term_.coefficient == 0)
                    this->tokens.erase(this->tokens.begin() + i);

                return *this;
            }
        }
    }

    if (typeid(*token) == typeid(Terms)) {
        auto &terms = dynamic_cast<Terms &>(*token);

        if (terms.coefficient == 0)
            return *this;

        if (terms.coefficient < 0) {
            terms.coefficient = -terms.coefficient;

            this->tokens.emplace_back(Sign::neg, std::move(token));

            return *this;
        }
    }

    this->tokens.emplace_back(Sign::pos, std::move(token));

    return *this;
}

mlp::Expression &mlp::Expression::operator-=(OwnedToken &&token) {
    if (typeid(*token) == typeid(Expression)) {
        for (auto &[sign, t] : dynamic_cast<Expression &>(*token).tokens)
            if (sign == Sign::pos)
                *this -= std::move(t);
            else
                *this += std::move(t);

        return *this;
    }

    if (typeid(*token) == typeid(Constant)) {
        auto const &constant = dynamic_cast<Constant const &>(*token);

        if (constant == 0)
            return *this;

        for (std::size_t i = 0; i < this->tokens.size(); ++i) {
            auto &[sign, t] = this->tokens[i];

            if (typeid(*t) != typeid(Constant))
                continue;

            auto &c = dynamic_cast<Constant &>(*t);

            if (sign == Sign::neg) {
                c = -c;
                sign = Sign::pos;
            }

            c -= constant;

            if (c == 0)
                this->tokens.erase(this->tokens.begin() + i);

            else if (c < 0) {
                c = -c;
                sign = Sign::neg;
            }

            return *this;
        }
    }

    if (typeid(*token) == typeid(Variable)) {
        auto const &variable = dynamic_cast<Variable const &>(*token);

        for (std::size_t i = 0; i < this->tokens.size(); ++i) {
            auto &[sign, t] = this->tokens[i];

            if (typeid(*t) == typeid(Variable)) {
                if (auto const &v = dynamic_cast<Variable const &>(*t);
                    v != variable)
                    continue;

                if (sign == Sign::pos) {
                    this->tokens.erase(this->tokens.begin() + i);
                } else {
                    t = std::make_unique<Term>(2 * (variable ^ 1));
                }

                return *this;
            }

            if (typeid(*t) == typeid(Term)) {
                auto &term = dynamic_cast<Term &>(*t);

                if (typeid(*term.base) != typeid(Variable) ||
                    dynamic_cast<Variable const &>(*term.base) != variable ||
                    typeid(*term.power) != typeid(Constant) ||
                    dynamic_cast<Constant const &>(*term.power) != 1)
                    continue;

                --term.coefficient;

                if (term.coefficient == 0)
                    this->tokens.erase(this->tokens.begin() + i);

                return *this;
            }
        }

        this->tokens.emplace_back(Sign::neg, std::move(token));

        return *this;
    }

    if (typeid(*token) == typeid(Term)) {
        auto &term = dynamic_cast<Term &>(*token);

        if (term.coefficient == 0)
            return *this;

        if (term.coefficient < 0) {
            term.coefficient = -term.coefficient;

            this->tokens.emplace_back(Sign::pos, std::move(token));

            return *this;
        }

        if (typeid(*term.base) != typeid(Variable) ||
            typeid(*term.power) != typeid(Constant) ||
            dynamic_cast<Constant const &>(*term.power) != 1) {
            this->tokens.emplace_back(Sign::neg, std::move(token));

            return *this;
        }

        auto const &variable = dynamic_cast<Variable const &>(*term.base);

        for (std::size_t i = 0; i < this->tokens.size(); ++i) {
            auto &[sign, t] = this->tokens[i];
            if (typeid(*t) == typeid(Variable)) {
                if (auto const &v = dynamic_cast<Variable const &>(*t);
                    v != variable)
                    continue;

                t = std::make_unique<Term>(
                    (1 - term.coefficient) * (variable ^ 1)
                );

                if (term.coefficient == 0)
                    this->tokens.erase(this->tokens.begin() + i);

                return *this;
            }

            if (typeid(*t) == typeid(Term)) {
                auto &term_ = dynamic_cast<Term &>(*t);

                if (typeid(*term_.base) != typeid(Variable) ||
                    dynamic_cast<Variable const &>(*term_.base) != variable ||
                    typeid(*term_.power) != typeid(Constant) ||
                    dynamic_cast<Constant const &>(*term_.power) != 1)
                    continue;

                term_.coefficient -= term.coefficient;

                if (term_.coefficient == 0)
                    this->tokens.erase(this->tokens.begin() + i);

                return *this;
            }
        }
    }

    if (typeid(*token) == typeid(Terms)) {
        auto &terms = dynamic_cast<Terms &>(*token);

        if (terms.coefficient == 0)
            return *this;

        if (terms.coefficient < 0) {
            terms.coefficient = -terms.coefficient;

            return *this += std::move(token);
        }
    }

    this->tokens.emplace_back(Sign::neg, std::move(token));

    return *this;
}