#ifndef TOKEN_H
#define TOKEN_H

#include "interfaces.h"

#include <cmath>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include <gsl/gsl>

namespace mlp {
struct Token;

template <typename T> using Owned = std::unique_ptr<T>;

using SharedToken = std::shared_ptr<Token>;
using OwnedToken = Owned<Token>;

class Variable;

struct Differentiable {
    virtual ~Differentiable() = default;

    [[nodiscard]] OwnedToken derivative(
        Variable const &variable, std::uint32_t order,
        std::map<Variable, SharedToken> const &values
    ) const;

    [[nodiscard]] virtual OwnedToken
    derivative(Variable const &variable, std::uint32_t order) const = 0;
};

struct Integrable {
    virtual ~Integrable() = default;

    [[nodiscard]] OwnedToken integral(
        Variable const &variable, SharedToken const &from, SharedToken const &to
    );

    [[nodiscard]] virtual OwnedToken integral(Variable const &variable) = 0;
};

struct Term;

struct Exponentiable {
    virtual ~Exponentiable() = default;

    [[nodiscard]] virtual Term operator^(std::double_t exponent) const & = 0;

    [[nodiscard]] virtual Term operator^(Token const &exponent) const & = 0;

    [[nodiscard]] virtual Term operator^(OwnedToken &&exponent) const & = 0;

    [[nodiscard]] virtual Term operator^(std::double_t exponent) && = 0;

    [[nodiscard]] virtual Term operator^(Token const &exponent) && = 0;

    [[nodiscard]] virtual Term operator^(OwnedToken &&exponent) && = 0;
};

struct Token : Dependable<Variable>,
               Evaluatable<Variable, SharedToken, OwnedToken>,
               Simplifiable<OwnedToken>,
               Differentiable,
               Integrable,
               Exponentiable {
    [[nodiscard]] virtual gsl::owner<Token *> clone() const = 0;

    explicit virtual operator std::string() const = 0;

    [[nodiscard]] Term operator^(std::double_t exponent) const & override;

    [[nodiscard]] Term operator^(Token const &exponent) const & override;

    [[nodiscard]] Term operator^(OwnedToken &&exponent) const & override;

    [[nodiscard]] Term operator^(std::double_t exponent) && override;

    [[nodiscard]] Term operator^(Token const &exponent) && override;

    [[nodiscard]] Term operator^(OwnedToken &&exponent) && override;
};

class Constant final : public Token {
    std::double_t value;

  public:
    explicit Constant(std::double_t value);

    Constant &operator=(std::double_t value);

    [[nodiscard]] gsl::owner<Constant *> clone() const override;

    explicit operator std::string() const override;

    explicit(false) operator std::double_t() const;

    Constant operator-() const;

    friend bool operator==(Constant const &lhs, Constant const &rhs);

    friend bool operator!=(Constant const &lhs, Constant const &rhs);

    friend bool operator>(Constant const &lhs, Constant const &rhs);

    friend bool operator>=(Constant const &lhs, Constant const &rhs);

    friend bool operator<(Constant const &lhs, Constant const &rhs);

    friend bool operator<=(Constant const &lhs, Constant const &rhs);

    friend Constant &operator++(Constant &lhs);

    friend Constant operator++(Constant &lhs, int);

    friend Constant &operator+=(Constant &lhs, Constant const &rhs);

    friend Constant &operator+=(Constant &lhs, std::double_t rhs);

    friend Constant operator+(Constant lhs, Constant const &rhs);

    friend Constant operator+(Constant lhs, std::double_t rhs);

    friend Constant operator+(std::double_t lhs, Constant rhs);

    friend Constant &operator--(Constant &lhs);

    friend Constant operator--(Constant &lhs, int);

    friend Constant &operator-=(Constant &lhs, Constant const &rhs);

    friend Constant &operator-=(Constant &lhs, std::double_t rhs);

    friend Constant operator-(Constant lhs, Constant const &rhs);

    friend Constant operator-(Constant lhs, std::double_t rhs);

    friend Constant operator-(std::double_t lhs, Constant rhs);

    friend Constant &operator*=(Constant &lhs, Constant const &rhs);

    friend Constant &operator*=(Constant &lhs, std::double_t rhs);

    friend Constant operator*(Constant lhs, Constant const &rhs);

    friend Constant operator*(Constant lhs, std::double_t rhs);

    friend Constant operator*(std::double_t lhs, Constant rhs);

    friend Constant &operator/=(Constant &lhs, Constant const &rhs);

    friend Constant &operator/=(Constant &lhs, std::double_t rhs);

    friend Constant operator/(Constant lhs, Constant const &rhs);

    friend Constant operator/(Constant lhs, std::double_t rhs);

    friend Constant operator/(std::double_t lhs, Constant rhs);

    friend Constant &operator^=(Constant &lhs, Constant const &rhs);

    friend Constant &operator^=(Constant &lhs, std::double_t rhs);

    friend Constant operator^(Constant lhs, Constant const &rhs);

    friend Constant operator^(Constant lhs, std::double_t rhs);

    friend Constant operator^(std::double_t lhs, Constant rhs);

    [[nodiscard]] bool is_dependent_on(Variable const &variable) const override;

    [[nodiscard]] bool is_linear_of(Variable const &variable) const override;

    [[nodiscard]] OwnedToken
    evaluate(std::map<Variable, SharedToken> const &values) const override;

    [[nodiscard]] OwnedToken simplified() const override;

    [[nodiscard]] OwnedToken
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] OwnedToken integral(Variable const &variable) override;
};

class Variable final : public Token {
    char var;

  public:
    explicit Variable(char var);

    [[nodiscard]] gsl::owner<Variable *> clone() const override;

    explicit operator std::string() const override;

    bool operator==(Variable const &) const;

    friend bool operator<(Variable const &lhs, Variable const &rhs);

    [[nodiscard]] bool is_dependent_on(Variable const &variable) const override;

    [[nodiscard]] bool is_linear_of(Variable const &variable) const override;

    [[nodiscard]] OwnedToken
    evaluate(std::map<Variable, SharedToken> const &values) const override;

    [[nodiscard]] OwnedToken simplified() const override;

    [[nodiscard]] OwnedToken
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] OwnedToken integral(Variable const &variable) override;
};

enum class Sign { pos, neg };

struct Operation final {
    enum op { add, sub, mul, div, pow } operation;

    explicit Operation(op operation);

    Operation(Operation const &) = default;

    static std::optional<Operation> from_char(char operation);

    explicit operator std::string() const;

    bool operator==(Operation const &) const = default;
};

class Function final : public Token {
    std::string function;

    OwnedToken parameter;

  public:
    explicit Function(std::string function, OwnedToken &&parameter);

    Function(Function const &function);

    Function(Function &&function) = default;

    [[nodiscard]] gsl::owner<Function *> clone() const override;

    explicit operator std::string() const override;

    [[nodiscard]] bool is_dependent_on(Variable const &variable) const override;

    [[nodiscard]] bool is_linear_of(Variable const &variable) const override;

    [[nodiscard]] OwnedToken
    evaluate(std::map<Variable, SharedToken> const &values) const override;

    [[nodiscard]] OwnedToken simplified() const override;

    [[nodiscard]] OwnedToken
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] OwnedToken integral(Variable const &variable) override;
};

struct Term final : Token {
    std::double_t coefficient{1};
    OwnedToken base;
    OwnedToken power;

    Term(std::double_t coefficient, OwnedToken &&base, OwnedToken &&power);

    Term(OwnedToken &&base, OwnedToken &&power);

    Term(Term const &term);

    [[nodiscard]] gsl::owner<Term *> clone() const override;

    explicit operator std::string() const override;

    friend Term operator-(Term const &rhs);

    Term &operator*=(std::double_t rhs);

    friend Term operator*(std::double_t lhs, Term rhs);

    friend Term operator*(Term lhs, std::double_t rhs);

    Term &operator/=(std::double_t rhs);

    friend Term operator/(std::double_t lhs, Term rhs);

    friend Term operator/(Term lhs, std::double_t rhs);

    Term &operator*=(Constant const &rhs);

    friend Term operator*(Constant const &lhs, Term rhs);

    friend Term operator*(Term lhs, Constant const &rhs);

    Term &operator/=(Constant const &rhs);

    friend Term operator/(Constant const &lhs, Term rhs);

    friend Term operator/(Term lhs, Constant const &rhs);

    [[nodiscard]] bool is_dependent_on(Variable const &variable) const override;

    [[nodiscard]] bool is_linear_of(Variable const &variable) const override;

    [[nodiscard]] OwnedToken
    evaluate(std::map<Variable, SharedToken> const &values) const override;

    [[nodiscard]] OwnedToken simplified() const override;

    [[nodiscard]] OwnedToken
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] OwnedToken integral(Variable const &variable) override;
};

class Terms final : public Token {
    std::vector<OwnedToken> terms;

  public:
    Constant coefficient{1};

    Terms() = default;

    [[nodiscard]] gsl::owner<Terms *> clone() const override;

    explicit operator std::string() const override;

    Terms operator-();

    Terms &operator*=(OwnedToken &&token);

    Terms &operator/=(OwnedToken &&token);

    Terms &operator*=(std::double_t scalar);

    friend Terms operator*(std::double_t lhs, Terms rhs);

    friend Terms operator*(Terms lhs, std::double_t rhs);

    Terms &operator/=(std::double_t scalar);

    friend Terms operator/(std::double_t lhs, Terms rhs);

    friend Terms operator/(Terms lhs, std::double_t rhs);

    Terms &operator*=(Constant const &rhs);

    friend Terms operator*(Terms lhs, Constant const &rhs);

    Terms &operator/=(Constant const &rhs);

    friend Terms operator/(Terms lhs, Constant const &rhs);

    [[nodiscard]] bool is_dependent_on(Variable const &variable) const override;

    [[nodiscard]] bool is_linear_of(Variable const &variable) const override;

    [[nodiscard]] OwnedToken
    evaluate(std::map<Variable, SharedToken> const &values) const override;

    [[nodiscard]] OwnedToken simplified() const override;

    [[nodiscard]] OwnedToken
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] OwnedToken integral(Variable const &variable) override;
};

class Expression final : public Token {
    std::vector<std::pair<Sign, OwnedToken>> tokens;

  public:
    Expression() = default;

    [[nodiscard]] gsl::owner<Expression *> clone() const override;

    void add_token(Sign sign, OwnedToken &&token);

    [[nodiscard]] std::pair<Sign, OwnedToken> pop_token();

    [[nodiscard]] bool empty() const;

    explicit operator std::string() const override;

    Expression &operator+=(OwnedToken &&token);

    Expression &operator-=(OwnedToken &&token);

    [[nodiscard]] bool is_dependent_on(Variable const &variable) const override;

    [[nodiscard]] bool is_linear_of(Variable const &variable) const override;

    [[nodiscard]] OwnedToken
    evaluate(std::map<Variable, SharedToken> const &values) const override;

    [[nodiscard]] OwnedToken simplified() const override;

    [[nodiscard]] OwnedToken
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] OwnedToken integral(Variable const &variable) override;
};

OwnedToken tokenise(std::string expression);

std::ostream &operator<<(std::ostream &os, Token const &token);

std::ostream &operator<<(std::ostream &os, Sign sign);
} // namespace mlp

std::string to_string(mlp::Sign sign);

#endif