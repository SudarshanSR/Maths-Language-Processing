#ifndef TOKENISE_H
#define TOKENISE_H

#include <ostream>
#include <string>
#include <vector>

struct Token {
    virtual ~Token() = default;

    explicit virtual operator std::string() const = 0;

    friend std::ostream &operator<<(std::ostream &os, Token const &token);
};

struct Constant final : Token {
    long double value;

    explicit Constant(long double value);

    explicit operator std::string() const override;

    bool operator==(Constant const &) const;
};

struct Variable final : Token {
    char var;

    explicit Variable(char var);

    explicit operator std::string() const override;

    bool operator==(Variable const &) const;
};

struct Operation final : Token {
    enum op { add, sub, mul, div, pow } operation;

    explicit Operation(op operation);

    static std::shared_ptr<Operation> from_char(char operation);

    explicit operator std::string() const override;
};

struct Function final : Token {
    std::string function;

    std::shared_ptr<Token> parameter = nullptr;

    explicit Function(std::string function,
                      std::shared_ptr<Token> const &parameter);

    explicit operator std::string() const override;

    void simplify();
};

struct Term final : Token {
    Constant coefficient{1};
    std::shared_ptr<Token> base = nullptr;
    std::shared_ptr<Token> power = nullptr;

    Term() = default;

    Term(long double coefficient, std::shared_ptr<Token> const &base,
         std::shared_ptr<Token> const &power);

    Term(std::shared_ptr<Token> const &base,
         std::shared_ptr<Token> const &power);

    Term(Term const &term) = default;

    explicit operator std::string() const override;
};

class Expression final : public Token {
  public:
    std::vector<std::shared_ptr<Token>> tokens;

    Expression() = default;

    Expression(Expression const &expression) = default;

    ~Expression() noexcept override = default;

    explicit operator std::string() const override;

    void add_token(std::shared_ptr<Token> const &token);

    [[nodiscard]] std::shared_ptr<Token> pop_token();
};

std::shared_ptr<Token> simplify(std::shared_ptr<Token> const &);

std::shared_ptr<Token> simplify(std::shared_ptr<Term> const &term);

std::shared_ptr<Token> simplify(std::shared_ptr<Expression> expression);

std::shared_ptr<Token> tokenise(std::string expression);

#endif // TOKENISE_H