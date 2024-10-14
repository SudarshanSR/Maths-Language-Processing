#ifndef TOKENISE_H
#define TOKENISE_H

#include "derivative.h"
#include "integral.h"
#include "simplify.h"

#include <map>
#include <ostream>
#include <string>
#include <vector>

using SharedToken = std::shared_ptr<Token>;
using OwnedToken = std::unique_ptr<Token>;

struct Token {
    virtual ~Token() = default;

    [[nodiscard]] virtual OwnedToken clone() const = 0;

    explicit virtual operator std::string() const = 0;
};

struct Variable;

struct Evaluatable {
    virtual ~Evaluatable() = default;

    [[nodiscard]] virtual OwnedToken
    at(std::map<Variable, SharedToken> const &values) const = 0;
};

struct Dependent {
    virtual ~Dependent() = default;

    [[nodiscard]] virtual bool is_dependent_on(Variable const &variable
    ) const = 0;
};

struct Constant final : Token,
                        Evaluatable,
                        Simplifiable,
                        Differentiable,
                        Integrable {
    double value;

    explicit Constant(double value);

    [[nodiscard]] OwnedToken clone() const override;

    [[nodiscard]] OwnedToken simplified() const override;

    [[nodiscard]] OwnedToken at(std::map<Variable, SharedToken> const &values
    ) const override;

    [[nodiscard]] OwnedToken
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] OwnedToken integral(Variable const &variable) const override;

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

    [[nodiscard]] OwnedToken clone() const override;

    [[nodiscard]] bool is_dependent_on(Variable const &variable) const override;

    [[nodiscard]] OwnedToken simplified() const override;

    [[nodiscard]] OwnedToken
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] OwnedToken at(std::map<Variable, SharedToken> const &values
    ) const override;

    [[nodiscard]] OwnedToken integral(Variable const &variable) const override;

    explicit operator std::string() const override;

    bool operator==(Variable const &) const;
};

struct Operation final : Token {
    enum op { add, sub, mul, div, pow } operation;

    explicit Operation(op operation);

    [[nodiscard]] OwnedToken clone() const override;

    static std::unique_ptr<Operation> from_char(char operation);

    explicit operator std::string() const override;
};

struct Function final : Token,
                        Dependent,
                        Evaluatable,
                        Simplifiable,
                        Differentiable,
                        Integrable {
    std::string function;

    OwnedToken parameter = nullptr;

    explicit Function(std::string function, OwnedToken &&parameter);

    [[nodiscard]] OwnedToken clone() const override;

    [[nodiscard]] bool is_dependent_on(Variable const &variable) const override;

    [[nodiscard]] OwnedToken at(std::map<Variable, SharedToken> const &values
    ) const override;

    [[nodiscard]] OwnedToken simplified() const override;

    [[nodiscard]] OwnedToken
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] OwnedToken integral(Variable const &variable) const override;

    explicit operator std::string() const override;
};

struct Term final : Token,
                    Dependent,
                    Evaluatable,
                    Simplifiable,
                    Differentiable,
                    Integrable {
    Constant coefficient{1};
    OwnedToken base = nullptr;
    OwnedToken power = nullptr;

    Term() = default;

    Term(double coefficient, OwnedToken &&base, OwnedToken &&power);

    Term(OwnedToken &&base, OwnedToken &&power);

    [[nodiscard]] OwnedToken clone() const override;

    [[nodiscard]] bool is_dependent_on(Variable const &variable) const override;

    [[nodiscard]] OwnedToken at(std::map<Variable, SharedToken> const &values
    ) const override;

    [[nodiscard]] OwnedToken simplified() const override;

    [[nodiscard]] OwnedToken
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] OwnedToken integral(Variable const &variable) const override;

    explicit operator std::string() const override;
};

struct Terms final : Token,
                     Dependent,
                     Evaluatable,
                     Simplifiable,
                     Differentiable,
                     Integrable {
    Constant coefficient{1};

    std::vector<OwnedToken> terms;

    Terms() = default;

    [[nodiscard]] OwnedToken clone() const override;

    void add_term(OwnedToken &&token);

    [[nodiscard]] bool is_dependent_on(Variable const &variable) const override;

    [[nodiscard]] OwnedToken simplified() const override;

    [[nodiscard]] OwnedToken at(std::map<Variable, SharedToken> const &values
    ) const override;

    [[nodiscard]] OwnedToken
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] OwnedToken integral(Variable const &variable) const override;

    explicit operator std::string() const override;
};

struct Expression final : Token,
                          Dependent,
                          Evaluatable,
                          Simplifiable,
                          Differentiable,
                          Integrable {
    std::vector<OwnedToken> tokens;

    Expression() = default;

    [[nodiscard]] OwnedToken clone() const override;

    void add_token(OwnedToken &&token);

    [[nodiscard]] OwnedToken pop_token();

    [[nodiscard]] bool is_dependent_on(Variable const &variable) const override;

    [[nodiscard]] OwnedToken simplified() const override;

    [[nodiscard]] OwnedToken at(std::map<Variable, SharedToken> const &values
    ) const override;

    explicit operator std::string() const override;

    [[nodiscard]] OwnedToken
    derivative(Variable const &variable, std::uint32_t order) const override;

    [[nodiscard]] OwnedToken integral(Variable const &variable) const override;
};

OwnedToken tokenise(std::string expression);

std::ostream &operator<<(std::ostream &os, Token const &token);

bool operator<(Variable const &lhs, Variable const &rhs);

#endif // TOKENISE_H
