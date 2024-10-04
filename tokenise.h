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

class Expression final : public Token {
  public:
    ~Expression() noexcept override;

    explicit operator std::string() const override;

    [[nodiscard]] std::vector<Token *> const &tokens() const;

    void add_token(Token *token);

  private:
    std::vector<Token *> tokens_;
};

Expression tokenise(std::string const &expression);

#endif // TOKENISE_H
