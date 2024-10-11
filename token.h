#ifndef TOKENISE_H
#define TOKENISE_H

#include <memory>
#include <ostream>
#include <string>
#include <vector>

struct Variable;

struct Token {
    virtual ~Token() = default;

    virtual std::shared_ptr<Token> simplify() = 0;

    virtual std::shared_ptr<Token>
    derivative(Variable const &variable, std::uint32_t order) = 0;

    explicit virtual operator std::string() const = 0;
};

struct Constant final : Token {
    long double value;

    explicit Constant(long double value);

    std::shared_ptr<Token> simplify() override;

    std::shared_ptr<Token>
    derivative(Variable const &variable, std::uint32_t order) override;

    explicit operator std::string() const override;

    bool operator==(Constant const &) const;
};

struct Variable final : Token {
    char var;

    explicit Variable(char var);

    std::shared_ptr<Token> simplify() override;

    std::shared_ptr<Token>
    derivative(Variable const &variable, std::uint32_t order) override;

    explicit operator std::string() const override;

    bool operator==(Variable const &) const;
};

struct Operation final : Token {
    enum op { add, sub, mul, div, pow } operation;

    explicit Operation(op operation);

    std::shared_ptr<Token> simplify() override;

    std::shared_ptr<Token>
    derivative(Variable const &variable, std::uint32_t order) override;

    static std::shared_ptr<Operation> from_char(char operation);

    explicit operator std::string() const override;
};

struct Function final : Token {
    std::string function;

    std::shared_ptr<Token> parameter = nullptr;

    explicit
    Function(std::string function, std::shared_ptr<Token> const &parameter);

    std::shared_ptr<Token> simplify() override;

    std::shared_ptr<Token>
    derivative(Variable const &variable, std::uint32_t order) override;

    explicit operator std::string() const override;
};

struct Term final : Token {
    Constant coefficient{1};
    std::shared_ptr<Token> base = nullptr;
    std::shared_ptr<Token> power = nullptr;

    Term() = default;

    Term(
        long double coefficient, std::shared_ptr<Token> const &base,
        std::shared_ptr<Token> const &power
    );

    Term(
        std::shared_ptr<Token> const &base, std::shared_ptr<Token> const &power
    );

    std::shared_ptr<Token> simplify() override;

    std::shared_ptr<Token>
    derivative(Variable const &variable, std::uint32_t order) override;

    explicit operator std::string() const override;
};

struct Terms final : Token {
    Constant coefficient{1};

    std::vector<std::shared_ptr<Token>> terms;

    Terms() = default;

    std::shared_ptr<Token> simplify() override;

    std::shared_ptr<Token>
    derivative(Variable const &variable, std::uint32_t order) override;

    explicit operator std::string() const override;

    void add_term(std::shared_ptr<Token> const &term);
};

struct Expression final : Token {
    std::vector<std::shared_ptr<Token>> tokens;

    Expression() = default;

    std::shared_ptr<Token> simplify() override;

    explicit operator std::string() const override;

    std::shared_ptr<Token>
    derivative(Variable const &variable, std::uint32_t order) override;

    void add_token(std::shared_ptr<Token> const &token);

    [[nodiscard]] std::shared_ptr<Token> pop_token();
};

std::shared_ptr<Token> tokenise(std::string expression);

std::ostream &operator<<(std::ostream &os, Token const &token);

bool operator<(Variable const &lhs, Variable const &rhs);

#endif // TOKENISE_H
