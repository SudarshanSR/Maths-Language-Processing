#ifndef TOKENISE_H
#define TOKENISE_H

#include <memory>
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

    std::shared_ptr<Constant> coefficient = nullptr;
    std::shared_ptr<Token> parameter = nullptr;
    std::shared_ptr<Token> power = nullptr;

    explicit Function(std::string function,
                      std::shared_ptr<Constant> const &coefficient,
                      std::shared_ptr<Token> const &parameter,
                      std::shared_ptr<Token> const &power);

    Function(Function const &function) = default;

    explicit operator std::string() const override;
};

struct Term final : Token {
    std::shared_ptr<Constant> coefficient = nullptr;
    std::shared_ptr<Token> base = nullptr;
    std::shared_ptr<Token> power = nullptr;

    Term() = default;

    Term(std::shared_ptr<Constant> const &coefficient,
         std::shared_ptr<Token> const &base,
         std::shared_ptr<Token> const &power);

    Term(Term const &term) = default;

    explicit operator std::string() const override;

    [[nodiscard]] static std::shared_ptr<Token>
    simplify(std::shared_ptr<Term> const &term);
};

class Expression final : public Token {
  public:
    Expression() = default;

    Expression(Expression const &expression) = default;

    ~Expression() noexcept override = default;

    explicit operator std::string() const override;

    [[nodiscard]] std::vector<std::shared_ptr<Token>> &tokens();

    [[nodiscard]] std::vector<std::shared_ptr<Token>> const &tokens() const;

    void add_token(std::shared_ptr<Token> const &token);

    [[nodiscard]] std::shared_ptr<Token> pop_token();

    [[nodiscard]] static std::shared_ptr<Token>
    simplify(std::shared_ptr<Expression> expression);

  private:
    std::vector<std::shared_ptr<Token>> tokens_;
};

Expression tokenise(std::string expression);

#endif // TOKENISE_H
