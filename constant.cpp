#include "token.h"

mlp::Constant::Constant(std::double_t const value) : value(value) {}

mlp::Constant &mlp::Constant::operator=(std::double_t const value) {
    this->value = value;

    return *this;
}

gsl::owner<mlp::Constant *> mlp::Constant::clone() const {
    return new Constant(this->value);
}

mlp::Constant::operator std::string() const {
    return std::to_string(this->value);
}

mlp::Constant mlp::Constant::operator-() const {
    return Constant{-this->value};
}

namespace mlp {
bool operator==(Constant const &lhs, Constant const &rhs) {
    return lhs.value == rhs.value;
}

bool operator==(std::double_t const lhs, Constant const &rhs) {
    return lhs == rhs.value;
}

bool operator==(Constant const &lhs, std::double_t const rhs) {
    return lhs.value == rhs;
}

bool operator!=(Constant const &lhs, Constant const &rhs) {
    return !(lhs == rhs);
}

bool operator!=(std::double_t const lhs, Constant const &rhs) {
    return !(lhs == rhs);
}

bool operator!=(Constant const &lhs, std::double_t const rhs) {
    return !(lhs == rhs);
}

bool operator<(Constant const &lhs, Constant const &rhs) {
    return lhs.value < rhs.value;
}

bool operator<(std::double_t const lhs, Constant const &rhs) {
    return lhs < rhs.value;
}

bool operator<(Constant const &lhs, std::double_t const rhs) {
    return lhs.value < rhs;
}

bool operator>=(Constant const &lhs, Constant const &rhs) {
    return !(lhs < rhs);
}

bool operator>=(std::double_t const lhs, Constant const &rhs) {
    return !(lhs < rhs);
}

bool operator>=(Constant const &lhs, std::double_t const rhs) {
    return !(lhs < rhs);
}

bool operator>(Constant const &lhs, Constant const &rhs) { return rhs < lhs; }

bool operator>(std::double_t const lhs, Constant const &rhs) {
    return rhs < lhs;
}

bool operator>(Constant const &lhs, std::double_t const rhs) {
    return rhs < lhs;
}

bool operator<=(Constant const &lhs, Constant const &rhs) { return rhs >= lhs; }

bool operator<=(std::double_t const lhs, Constant const &rhs) {
    return rhs >= lhs;
}

bool operator<=(Constant const &lhs, std::double_t const rhs) {
    return rhs >= lhs;
}

Constant &operator+=(Constant &lhs, Constant const &rhs) {
    lhs.value += rhs.value;

    return lhs;
}

Constant &operator+=(Constant &lhs, std::double_t const rhs) {
    lhs.value += rhs;

    return lhs;
}

Constant operator+(Constant lhs, Constant const &rhs) {
    return std::move(lhs += rhs);
}

Constant operator+(Constant lhs, std::double_t const rhs) {
    return std::move(lhs += rhs);
}

Constant operator+(std::double_t const lhs, Constant rhs) {
    return std::move(rhs += lhs);
}

Constant &operator-=(Constant &lhs, Constant const &rhs) {
    lhs.value -= rhs.value;

    return lhs;
}

Constant &operator-=(Constant &lhs, std::double_t const rhs) {
    lhs.value -= rhs;

    return lhs;
}

Constant operator-(Constant lhs, Constant const &rhs) {
    return std::move(lhs -= rhs);
}

Constant operator-(Constant lhs, std::double_t const rhs) {
    return std::move(lhs -= rhs);
}

Constant operator-(std::double_t const lhs, Constant rhs) {
    return std::move(rhs -= lhs);
}

Constant &operator*=(Constant &lhs, Constant const &rhs) {
    lhs.value *= rhs.value;

    return lhs;
}

Constant &operator*=(Constant &lhs, std::double_t const rhs) {
    lhs.value *= rhs;

    return lhs;
}

Constant operator*(Constant lhs, Constant const &rhs) {
    return std::move(lhs *= rhs);
}

Constant operator*(Constant lhs, std::double_t const rhs) {
    return std::move(lhs *= rhs);
}

Constant operator*(std::double_t const lhs, Constant rhs) {
    return std::move(rhs *= lhs);
}

Constant &operator/=(Constant &lhs, Constant const &rhs) {
    lhs.value /= rhs.value;

    return lhs;
}

Constant &operator/=(Constant &lhs, std::double_t const rhs) {
    lhs.value /= rhs;

    return lhs;
}

Constant operator/(Constant lhs, Constant const &rhs) {
    return std::move(lhs /= rhs);
}

Constant operator/(Constant lhs, std::double_t const rhs) {
    return std::move(lhs /= rhs);
}

Constant operator/(std::double_t const lhs, Constant rhs) {
    return std::move(rhs /= lhs);
}

Constant &operator^=(Constant &lhs, Constant const &rhs) {
    lhs.value = std::pow(lhs.value, rhs.value);

    return lhs;
}

Constant &operator^=(Constant &lhs, std::double_t const rhs) {
    lhs.value = std::pow(lhs.value, rhs);

    return lhs;
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

std::double_t &operator+=(std::double_t &lhs, mlp::Constant const &rhs) {
    lhs += rhs.value;

    return lhs;
}

std::double_t &operator-=(std::double_t &lhs, mlp::Constant const &rhs) {
    lhs += rhs.value;

    return lhs;
}

std::double_t &operator*=(std::double_t &lhs, mlp::Constant const &rhs) {
    lhs *= rhs.value;

    return lhs;
}

std::double_t &operator/=(std::double_t &lhs, mlp::Constant const &rhs) {
    lhs /= rhs.value;

    return lhs;
}

std::double_t &operator^=(std::double_t &lhs, mlp::Constant const &rhs) {
    lhs = std::pow(lhs, rhs.value);

    return lhs;
}
