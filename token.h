#ifndef TOKENISE_H
#define TOKENISE_H

#include "derivative.h"
#include "integral.h"
#include "simplify.h"

#include <map>
#include <ostream>
#include <string>
#include <vector>

struct Token {
    virtual ~Token() = default;

    explicit virtual operator std::string() const = 0;
};

struct Variable;

struct Evaluatable {
    virtual ~Evaluatable() = default;

    [[nodiscard]] virtual std::shared_ptr<Token>
    at(std::map<Variable, std::shared_ptr<Token>> const &values) = 0;
};

struct Dependent {
    virtual ~Dependent() = default;

    [[nodiscard]] virtual bool is_function_of(Variable const &variable
    ) const = 0;
};

struct Constant final : Token,
                        Evaluatable,
                        Simplifiable,
                        Differentiable,
                        Integrable {
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

struct Variable final : Token,
                        Dependent,
                        Evaluatable,
                        Simplifiable,
                        Differentiable,
                        Integrable {
    char var;

    explicit Variable(char var);

    [[nodiscard]] bool is_function_of(Variable const &variable) const override;

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

    explicit operator std::string() const override;
};

struct Function final : Token,
                        Dependent,
                        Evaluatable,
                        Simplifiable,
                        Differentiable,
                        Integrable {
    std::string function;

    std::shared_ptr<Token> parameter = nullptr;

    explicit
    Function(std::string function, std::shared_ptr<Token> const &parameter);

    [[nodiscard]] bool is_function_of(Variable const &variable) const override;

    [[nodiscard]] std::shared_ptr<Token>
    at(std::map<Variable, std::shared_ptr<Token>> const &values) override;

    [[nodiscard]] std::shared_ptr<Token> simplified() const override;

    [[nodiscard]] std::shared_ptr<Token>
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] std::shared_ptr<Token> integral(Variable const &variable
    ) const override;

    explicit operator std::string() const override;
};

struct Term final : Token,
                    Dependent,
                    Evaluatable,
                    Simplifiable,
                    Differentiable,
                    Integrable {
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

    [[nodiscard]] bool is_function_of(Variable const &variable) const override;

    [[nodiscard]] std::shared_ptr<Token>
    at(std::map<Variable, std::shared_ptr<Token>> const &values) override;

    [[nodiscard]] std::shared_ptr<Token> simplified() const override;

    [[nodiscard]] std::shared_ptr<Token>
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] std::shared_ptr<Token> integral(Variable const &variable
    ) const override;

    explicit operator std::string() const override;
};

struct Terms final : Token,
                     Dependent,
                     Evaluatable,
                     Simplifiable,
                     Differentiable,
                     Integrable {
    Constant coefficient{1};

    std::vector<std::shared_ptr<Token>> terms;

    Terms() = default;

    void add_term(std::shared_ptr<Token> const &token);

    [[nodiscard]] bool is_function_of(Variable const &variable) const override;

    [[nodiscard]] std::shared_ptr<Token> simplified() const override;

    [[nodiscard]] std::shared_ptr<Token>
    at(std::map<Variable, std::shared_ptr<Token>> const &values) override;

    [[nodiscard]] std::shared_ptr<Token>
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] std::shared_ptr<Token> integral(Variable const &variable
    ) const override;

    explicit operator std::string() const override;
};

struct Expression final : Token,
                          Dependent,
                          Evaluatable,
                          Simplifiable,
                          Differentiable,
                          Integrable {
    std::vector<std::shared_ptr<Token>> tokens;

    Expression() = default;

    void add_token(std::shared_ptr<Token> const &token);

    [[nodiscard]] std::shared_ptr<Token> pop_token();

    [[nodiscard]] bool is_function_of(Variable const &variable) const override;

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
