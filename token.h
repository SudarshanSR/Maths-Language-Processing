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
};

struct Variable final : Token {
    char var;

    explicit Variable(char var);

    explicit operator std::string() const override;
};

struct Operation final : Token {
    enum class op { add, sub, mul, div, pow };

    std::optional<op> operation;

    explicit Operation(char operation);

    explicit operator std::string() const override;
};

struct Function final : Token {
    std::string function;

    Constant *coefficient;
    Token *parameter;
    Token *power;

    explicit Function(std::string function, Constant *coefficient,
                      Token *parameter, Token *power);

    Function(Function const &function);

    explicit operator std::string() const override;
};

struct Term {
    Constant *coefficient = nullptr;
    Token *base = nullptr;
    Token *power = nullptr;
};

class Expression final : public Token {
  public:
    Expression() = default;

    explicit Expression(Term const &term);

    Expression(Expression const &expression);

    ~Expression() noexcept override;

    explicit operator std::string() const override;

    [[nodiscard]] std::vector<Token *> &tokens();

    [[nodiscard]] std::vector<Token *> const &tokens() const;

    void add_token(Token *token);

    Token *pop_token();

    void simplify();

  private:
    std::vector<Token *> tokens_;
};

Expression tokenise(std::string expression);

Token *copy(Token *token);

#endif // TOKENISE_H
