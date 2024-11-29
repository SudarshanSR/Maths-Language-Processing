#include "../include/constant.h"

#include "../include/expression.h"
#include "../include/function.h"
#include "../include/term.h"
#include "../include/terms.h"
#include "../include/variable.h"

mlp::Constant::Constant(std::double_t const value) : value_(value) {}

mlp::Constant &mlp::Constant::operator=(std::double_t const value) {
    this->value_ = value;

    return *this;
}

gsl::owner<mlp::Constant *> mlp::Constant::clone() const {
    return new Constant(this->value_);
}

gsl::owner<mlp::Constant *> mlp::Constant::move() && {
    return new Constant(std::move(*this));
}

mlp::Constant::operator std::string() const {
    return std::to_string(this->value_);
}

mlp::Constant::operator double() const { return this->value_; }

mlp::Constant mlp::Constant::operator-() const {
    return Constant{-this->value_};
}

mlp::Constant &mlp::Constant::operator++() {
    ++this->value_;

    return *this;
}

mlp::Constant &mlp::Constant::operator--() {
    --this->value_;

    return *this;
}

mlp::Constant &mlp::Constant::operator+=(std::double_t const rhs) {
    this->value_ += rhs;

    return *this;
}

mlp::Constant &mlp::Constant::operator-=(std::double_t const rhs) {
    this->value_ -= rhs;

    return *this;
}

mlp::Constant &mlp::Constant::operator*=(std::double_t const rhs) {
    this->value_ *= rhs;

    return *this;
}

mlp::Constant &mlp::Constant::operator/=(std::double_t const rhs) {
    this->value_ /= rhs;

    return *this;
}

mlp::Constant &mlp::Constant::operator^=(std::double_t const rhs) {
    this->value_ = std::pow(this->value_, rhs);

    return *this;
}

bool mlp::Constant::operator==(Constant const &rhs) const {
    return this->value_ == rhs.value_;
}

bool mlp::Constant::operator>(Constant const &rhs) const {
    return this->value_ > rhs.value_;
}

bool mlp::is_dependent_on(Constant const &, Variable const &) { return false; }

bool mlp::is_linear_of(Constant const &, Variable const &) { return false; }

mlp::token
mlp::evaluate(Constant const &token, std::map<Variable, SharedToken> const &) {
    return token;
}

mlp::token mlp::simplified(Constant const &token) { return token; }

mlp::token mlp::derivative(
    Constant const &token, Variable const &, std::uint32_t const order
) {
    if (!order)
        return token;

    return Constant(0);
}

mlp::token mlp::integral(Constant const &token, Variable const &variable) {
    return token.value() * variable;
}

namespace mlp {
Constant operator++(Constant &lhs, int) {
    Constant temp = lhs;
    ++lhs;
    return temp;
}

Constant operator--(Constant &lhs, int) {
    Constant temp = lhs;
    --lhs;
    return temp;
}

Constant operator+(Constant lhs, std::double_t const rhs) {
    return std::move(lhs += rhs);
}

Constant operator+(std::double_t const lhs, Constant rhs) {
    return std::move(rhs += lhs);
}

Constant operator-(Constant lhs, std::double_t const rhs) {
    return std::move(lhs -= rhs);
}

Constant operator-(std::double_t const lhs, Constant rhs) {
    return std::move(rhs -= lhs);
}

Constant operator*(Constant lhs, std::double_t const rhs) {
    return std::move(lhs *= rhs);
}

Constant operator*(std::double_t const lhs, Constant rhs) {
    return std::move(rhs *= lhs);
}

Constant operator/(Constant lhs, std::double_t const rhs) {
    return std::move(lhs /= rhs);
}

Constant operator/(std::double_t const lhs, Constant rhs) {
    return std::move(rhs /= lhs);
}

Constant operator^(Constant lhs, Constant const &rhs) {
    return std::move(lhs ^= rhs);
}

Constant operator^(Constant lhs, std::double_t const rhs) {
    return std::move(lhs ^= rhs);
}

Constant operator^(std::double_t const lhs, Constant rhs) {
    return std::move(rhs ^= lhs);
}
} // namespace mlp
