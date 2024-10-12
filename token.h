#ifndef TOKENISE_H
#define TOKENISE_H

#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

struct Variable;

struct Token {
    virtual ~Token() = default;

    [[nodiscard]] virtual std::shared_ptr<Token> simplified() const = 0;

    [[nodiscard]] virtual std::shared_ptr<Token>
    at(std::map<Variable, std::shared_ptr<Token>> const &values) = 0;

    [[nodiscard]] virtual std::shared_ptr<Token>
    derivative(Variable const &variable, std::uint32_t order) const = 0;

    [[nodiscard]] virtual std::shared_ptr<Token>
    integral(Variable const &variable) const = 0;

    explicit virtual operator std::string() const = 0;
};

struct Constant final : Token {
    double value;

    explicit Constant(double value);

    [[nodiscard]] std::shared_ptr<Token> simplified() const override;

    [[nodiscard]] std::shared_ptr<Token>
    at(std::map<Variable, std::shared_ptr<Token>> const &values) override;

    [[nodiscard]] std::shared_ptr<Token>
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] std::shared_ptr<Token> integral(Variable const &variable
    ) const override;

    explicit operator std::string() const override;

    bool operator==(Constant const &) const;
};

struct Variable final : Token {
    char var;

    explicit Variable(char var);

    [[nodiscard]] std::shared_ptr<Token> simplified() const override;

    [[nodiscard]] std::shared_ptr<Token>
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] std::shared_ptr<Token>
    at(std::map<Variable, std::shared_ptr<Token>> const &values) override;

    [[nodiscard]] std::shared_ptr<Token> integral(Variable const &variable
    ) const override;

    explicit operator std::string() const override;

    bool operator==(Variable const &) const;
};

struct Operation final : Token {
    enum op { add, sub, mul, div, pow } operation;

    explicit Operation(op operation);

    static std::shared_ptr<Operation> from_char(char operation);

    [[nodiscard]] std::shared_ptr<Token> simplified() const override;

    [[nodiscard]] std::shared_ptr<Token>
    at(std::map<Variable, std::shared_ptr<Token>> const &values) override;

    [[nodiscard]] std::shared_ptr<Token>
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] std::shared_ptr<Token> integral(Variable const &variable
    ) const override;

    explicit operator std::string() const override;
};

struct Function final : Token {
    std::string function;

    std::shared_ptr<Token> parameter = nullptr;

    explicit
    Function(std::string function, std::shared_ptr<Token> const &parameter);

    [[nodiscard]] std::shared_ptr<Token>
    at(std::map<Variable, std::shared_ptr<Token>> const &values) override;

    [[nodiscard]] std::shared_ptr<Token> simplified() const override;

    [[nodiscard]] std::shared_ptr<Token>
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] std::shared_ptr<Token> integral(Variable const &variable
    ) const override;

    explicit operator std::string() const override;
};

struct Term final : Token {
    Constant coefficient{1};
    std::shared_ptr<Token> base = nullptr;
    std::shared_ptr<Token> power = nullptr;

    Term() = default;

    Term(
        double coefficient, std::shared_ptr<Token> const &base,
        std::shared_ptr<Token> const &power
    );

    Term(
        std::shared_ptr<Token> const &base, std::shared_ptr<Token> const &power
    );

    [[nodiscard]] std::shared_ptr<Token>
    at(std::map<Variable, std::shared_ptr<Token>> const &values) override;

    [[nodiscard]] std::shared_ptr<Token> simplified() const override;

    [[nodiscard]] std::shared_ptr<Token>
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] std::shared_ptr<Token> integral(Variable const &variable
    ) const override;

    explicit operator std::string() const override;
};

struct Terms final : Token {
    Constant coefficient{1};

    std::vector<std::shared_ptr<Token>> terms;

    Terms() = default;

    void add_term(std::shared_ptr<Token> const &token);

    [[nodiscard]] std::shared_ptr<Token> simplified() const override;

    [[nodiscard]] std::shared_ptr<Token>
    at(std::map<Variable, std::shared_ptr<Token>> const &values) override;

    [[nodiscard]] std::shared_ptr<Token>
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] std::shared_ptr<Token> integral(Variable const &variable
    ) const override;

    explicit operator std::string() const override;
};

struct Expression final : Token {
    std::vector<std::shared_ptr<Token>> tokens;

    Expression() = default;

    void add_token(std::shared_ptr<Token> const &token);

    [[nodiscard]] std::shared_ptr<Token> pop_token();

    [[nodiscard]] std::shared_ptr<Token> simplified() const override;

    [[nodiscard]] std::shared_ptr<Token>
    at(std::map<Variable, std::shared_ptr<Token>> const &values) override;

    explicit operator std::string() const override;

    [[nodiscard]] std::shared_ptr<Token>
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] std::shared_ptr<Token> integral(Variable const &variable
    ) const override;
};

std::shared_ptr<Token> tokenise(std::string expression);

std::ostream &operator<<(std::ostream &os, Token const &token);

bool operator<(Variable const &lhs, Variable const &rhs);

#endif // TOKENISE_H
