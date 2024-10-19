#ifndef TOKEN_H
#define TOKEN_H

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

struct Token {
    virtual ~Token() = default;

    [[nodiscard]] virtual gsl::owner<Token *> clone() const = 0;

    explicit virtual operator std::string() const = 0;
};

struct Constant final : Token {
    double value;

    explicit Constant(double value);

    [[nodiscard]] gsl::owner<Constant *> clone() const override;

    explicit operator std::string() const override;

    bool operator==(Constant const &) const;
};

struct Variable final : Token {
    char var;

    explicit Variable(char var);

    [[nodiscard]] gsl::owner<Variable *> clone() const override;

    explicit operator std::string() const override;

    bool operator==(Variable const &) const;
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

struct Function final : Token {
    std::string function;

    OwnedToken parameter;

    explicit Function(std::string function, OwnedToken &&parameter);

    Function(Function const &function);

    Function(Function &&function) = default;

    [[nodiscard]] gsl::owner<Function *> clone() const override;

    explicit operator std::string() const override;
};

struct Term final : Token {
    double coefficient{1};
    OwnedToken base;
    OwnedToken power;

    Term(double coefficient, OwnedToken &&base, OwnedToken &&power);

    Term(OwnedToken &&base, OwnedToken &&power);

    [[nodiscard]] gsl::owner<Term *> clone() const override;

    explicit operator std::string() const override;
};

struct Terms final : Token {
    double coefficient{1};

    std::vector<OwnedToken> terms;

    Terms() = default;

    [[nodiscard]] gsl::owner<Terms *> clone() const override;

    void add_term(OwnedToken &&token);

    explicit operator std::string() const override;
};

struct Expression final : Token {
    std::vector<std::pair<Sign, OwnedToken>> tokens;

    Expression() = default;

    [[nodiscard]] gsl::owner<Expression *> clone() const override;

    void add_token(Sign sign, OwnedToken &&token);

    [[nodiscard]] std::pair<Sign, OwnedToken> pop_token();

    explicit operator std::string() const override;
};

OwnedToken tokenise(std::string expression);

std::ostream &operator<<(std::ostream &os, Token const &token);

bool operator<(Variable const &lhs, Variable const &rhs);

std::ostream &operator<<(std::ostream &os, Sign sign);
} // namespace mlp

std::string to_string(mlp::Sign sign);

#endif