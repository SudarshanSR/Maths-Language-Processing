#include "../include/constant.h"

#include "../include/term.h"
#include "../include/variable.h"

mlp::Constant::Constant(std::double_t const value) : value(value) {}

mlp::Constant &mlp::Constant::operator=(std::double_t const value) {
    this->value = value;

    return *this;
}

gsl::owner<mlp::Constant *> mlp::Constant::clone() const {
    return new Constant(this->value);
}

gsl::owner<mlp::Constant *> mlp::Constant::move() && {
    return new Constant(std::move(*this));
}

mlp::Constant::operator std::string() const {
    return std::to_string(this->value);
}

mlp::Constant::operator double() const { return this->value; }

mlp::Constant mlp::Constant::operator-() const {
    return Constant{-this->value};
}

mlp::Constant &mlp::Constant::operator++() {
    ++this->value;

    return *this;
}

mlp::Constant &mlp::Constant::operator--() {
    --this->value;

    return *this;
}

mlp::Constant &mlp::Constant::operator+=(std::double_t const rhs) {
    this->value += rhs;

    return *this;
}

mlp::Constant &mlp::Constant::operator-=(std::double_t const rhs) {
    this->value -= rhs;

    return *this;
}

mlp::Constant &mlp::Constant::operator*=(std::double_t const rhs) {
    this->value *= rhs;

    return *this;
}

mlp::Constant &mlp::Constant::operator/=(std::double_t const rhs) {
    this->value /= rhs;

    return *this;
}

mlp::Constant &mlp::Constant::operator^=(std::double_t const rhs) {
    this->value = std::pow(this->value, rhs);

    return *this;
}

bool mlp::Constant::operator==(Constant const &rhs) const {
    return this->value == rhs.value;
}

bool mlp::Constant::operator>(Constant const &rhs) const {
    return this->value > rhs.value;
}

bool mlp::Constant::is_dependent_on(Variable const &variable) const {
    return false;
}

bool mlp::Constant::is_linear_of(Variable const &variable) const {
    return false;
}

mlp::OwnedToken
mlp::Constant::evaluate(std::map<Variable, SharedToken> const &values) const {
    return std::make_unique<Constant>(*this);
}

mlp::OwnedToken mlp::Constant::simplified() const {
    return Owned<Constant>(this->clone());
}

mlp::OwnedToken
mlp::Constant::derivative(Variable const &, std::uint32_t const) const {
    return std::make_unique<Constant>(0);
}

mlp::OwnedToken mlp::Constant::integral(Variable const &variable) const {
    return std::make_unique<Term>(this->value * variable);
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
