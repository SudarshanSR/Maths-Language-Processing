#ifndef TOKEN_H
#define TOKEN_H

#include <ostream>
#include <string>
#include <vector>

namespace mlp {
struct Token;

using SharedToken = std::shared_ptr<Token>;
using OwnedToken = std::unique_ptr<Token>;

struct Token {
    virtual ~Token() = default;

    [[nodiscard]] virtual OwnedToken clone() const = 0;

    explicit virtual operator std::string() const = 0;
};

struct Constant final : Token {
    double value;

    explicit Constant(double value);

    [[nodiscard]] OwnedToken clone() const override;

    explicit operator std::string() const override;

    bool operator==(Constant const &) const;
};

struct Variable final : Token {
    char var;

    explicit Variable(char var);

    [[nodiscard]] OwnedToken clone() const override;

    explicit operator std::string() const override;

    bool operator==(Variable const &) const;
};

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

    [[nodiscard]] OwnedToken clone() const override;

    explicit operator std::string() const override;
};

struct Term final : Token {
    double coefficient{1};
    OwnedToken base;
    OwnedToken power;

    Term(double coefficient, OwnedToken &&base, OwnedToken &&power);

    Term(OwnedToken &&base, OwnedToken &&power);

    [[nodiscard]] OwnedToken clone() const override;

    explicit operator std::string() const override;
};

struct Terms final : Token {
    double coefficient{1};

    std::vector<OwnedToken> terms;

    Terms() = default;

    [[nodiscard]] OwnedToken clone() const override;

    void add_term(OwnedToken &&token);

    explicit operator std::string() const override;
};

struct Expression final : Token {
    std::vector<std::pair<Operation, OwnedToken>> tokens;

    Expression() = default;

    [[nodiscard]] OwnedToken clone() const override;

    void add_token(Operation const &operation, OwnedToken &&token);

    [[nodiscard]] std::pair<Operation, OwnedToken> pop_token();

    explicit operator std::string() const override;
};

OwnedToken tokenise(std::string expression);

std::ostream &operator<<(std::ostream &os, Token const &token);

bool operator<(Variable const &lhs, Variable const &rhs);
} // namespace mlp

#endif