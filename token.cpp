#include "token.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>

namespace {
std::map<char, char> k_parenthesis_map{{'(', ')'}, {'[', ']'}, {'{', '}'}};

std::set<std::string> k_functions{
    "sin",  "cos",  "tan",  "sec",  "csc",  "cot", "sinh",
    "cosh", "tanh", "sech", "csch", "coth", "ln",
};

std::shared_ptr<Token> get_next_token(std::string const &expression, int &i) {
    char character = expression[i];

    if (k_parenthesis_map.contains(character)) {
        int count = 1;
        char const original = character;

        character = expression[++i];

        std::size_t const start = i;

        while (i < expression.size() && count) {
            if (character == original)
                ++count;

            else if (character == k_parenthesis_map[original])
                --count;

            character = expression[++i];
        }

        if (count)
            throw std::invalid_argument("Expression is not valid!");

        std::size_t const end = i - start - 1;

        return Expression::simplify(std::make_shared<Expression>(
            tokenise(expression.substr(start, end))));
    }

    for (int const offset : {4, 3, 2})
        if (i + offset < expression.size())
            if (std::string const &fn = expression.substr(i, offset);
                k_functions.contains(fn)) {
                i += offset;

                return std::make_shared<Function>(
                    fn, nullptr, get_next_token(expression, i), nullptr);
            }

    if (character == 'e' && i < expression.size() - 1 &&
        expression[i + 1] == '^') {
        i += 2;

        return std::make_shared<Function>(
            "e^", nullptr, get_next_token(expression, i), nullptr);
    }

    if (auto op = std::make_unique<Operation>(character); op->operation)
        return op;

    if ('0' <= character && character <= '9') {
        std::string number;

        while (i < expression.size() && '0' < character && character < '9') {
            number.push_back(character);

            character = expression[++i];
        }

        if (i < expression.size()) {
            if (character != '.') {
                --i;

                return std::make_shared<Constant>(std::stoull(number));
            }

            do {
                number.push_back(character);

                character = expression[++i];
            } while (i < expression.size() && '0' <= character &&
                     character <= '9');

            if (i < expression.size())
                --i;

            return std::make_shared<Constant>(std::stold(number));
        }

        return std::make_shared<Constant>(std::stoull(number));
    }

    if (('A' <= character && character <= 'Z') ||
        ('a' <= character && character <= 'z'))
        return std::make_shared<Variable>(character);

    return nullptr;
}
} // namespace

Constant::Constant(long double const value) : value(value) {}

Constant::operator std::string() const { return std::to_string(this->value); }

Variable::Variable(char const var) : var(var) {}

Variable::operator std::string() const { return {this->var}; }

Operation::Operation(char const operation) {
    switch (operation) {
    case '+':
        this->operation = op::add;
        break;

    case '-':
        this->operation = op::sub;
        break;

    case '*':
        this->operation = op::mul;
        break;

    case '/':
        this->operation = op::div;
        break;

    case '^':
        this->operation = op::pow;
        break;

    default:
        break;
    }
}

Operation::operator std::string() const {
    if (!this->operation)
        return "";

    switch (*this->operation) {
    case op::add:
        return "+";
    case op::sub:
        return "-";
    case op::mul:
        return "*";
    case op::div:
        return "/";
    case op::pow:
        return "^";
    }

    return "";
}

Function::Function(std::string function,
                   std::shared_ptr<Constant> const &coefficient,
                   std::shared_ptr<Token> const &parameter,
                   std::shared_ptr<Token> const &power)
    : function(std::move(function)), coefficient(coefficient),
      parameter(parameter), power(power) {}

Function::operator std::string() const {
    std::stringstream result;

    if (this->coefficient->value != 1) {
        if (this->coefficient->value == -1)
            result << "-";

        else
            result << *this->coefficient;
    }

    result << this->function << '('
           << static_cast<std::string>(*this->parameter) << ')';

    if (this->power)
        if (auto const power = std::dynamic_pointer_cast<Constant>(this->power);
            power && power->value != 1)
            result << '^' << static_cast<std::string>(*this->power);

    return result.str();
}

Term::Term(std::shared_ptr<Constant> const &coefficient,
           std::shared_ptr<Token> const &base,
           std::shared_ptr<Token> const &power)
    : coefficient(coefficient), base(base), power(power) {}

Term::operator std::string() const {
    std::stringstream result;

    if (this->coefficient && this->coefficient->value != 1) {
        if (this->coefficient->value == -1)
            result << "-";

        else
            result << *this->coefficient;
    }

    if (this->base)
        result << *this->base;

    if (this->power) {
        result << '^';
        result << *this->power;
    }

    return result.str();
}

std::shared_ptr<Token> Term::simplify(std::shared_ptr<Term> const &term) {
    if (term->coefficient) {
        if (term->coefficient->value == 0 || !term->base) {
            auto coefficient = term->coefficient;
            term->coefficient = nullptr;

            return coefficient;
        }
    } else if (term->base) {
        auto base = term->base;
        term->base = nullptr;

        return base;
    }

    return term;
}

Expression::operator std::string() const {
    std::stringstream result;

    if (this->tokens_.size() == 1) {
        result << static_cast<std::string>(*this->tokens_[0]);
    } else {
        result << '(';

        for (std::shared_ptr<Token> const &token : this->tokens_)
            result << static_cast<std::string>(*token);

        result << ')';
    }

    return result.str();
}

std::vector<std::shared_ptr<Token>> &Expression::tokens() {
    return this->tokens_;
}

std::vector<std::shared_ptr<Token>> const &Expression::tokens() const {
    return this->tokens_;
}

void Expression::add_token(std::shared_ptr<Token> const &token) {
    this->tokens_.push_back(token);
}

std::shared_ptr<Token> Expression::pop_token() {
    std::shared_ptr<Token> token = this->tokens_.back();

    this->tokens_.pop_back();

    return token;
}

std::shared_ptr<Token>
Expression::simplify(std::shared_ptr<Expression> expression) {
    std::vector<std::shared_ptr<Token>> &tokens = expression->tokens_;

    for (std::shared_ptr<Token> &token : tokens) {
        if (auto const e = std::dynamic_pointer_cast<Expression>(token))
            token = simplify(e);
        else if (auto term = std::dynamic_pointer_cast<Term>(token))
            token = Term::simplify(term);
    }

    int i = 0;

    while (i < tokens.size()) {
        std::shared_ptr<Token> &token = tokens[i];

        if (auto const operation =
                std::dynamic_pointer_cast<Operation>(token)) {
            if (i == tokens.size() - 1) {
                ++i;

                continue;
            }

            if (operation->operation == Operation::op::sub) {
                std::shared_ptr<Token> next = tokens[i + 1];

                if (auto const constant =
                        std::dynamic_pointer_cast<Constant>(next)) {
                    constant->value = -constant->value;

                    operation->operation = Operation::op::add;
                } else if (auto const term =
                               std::dynamic_pointer_cast<Term>(next)) {
                    term->coefficient->value = -term->coefficient->value;

                    operation->operation = Operation::op::add;
                } else if (auto const function =
                               std::dynamic_pointer_cast<Function>(next)) {
                    function->coefficient->value =
                        -function->coefficient->value;

                    operation->operation = Operation::op::add;
                }

                if (i == 0) {
                    tokens.erase(tokens.begin());

                    continue;
                }
            }

            if (i == 0 && operation->operation == Operation::op::add) {
                tokens.erase(tokens.begin());

                continue;
            }

            ++i;

            continue;
        }

        if (auto const constant = std::dynamic_pointer_cast<Constant>(token)) {
            if (i == tokens.size() - 1) {
                ++i;

                continue;
            }

            if (constant->value == 0) {
                if (auto const operation =
                        std::dynamic_pointer_cast<Operation>(tokens[i + 1])) {
                    // Simplifies 0 * expr and 0 / expr and 0 ^ expr
                    if (operation->operation == Operation::op::mul ||
                        operation->operation == Operation::op::div ||
                        operation->operation == Operation::op::pow) {
                        do {
                            tokens.erase(tokens.begin() + i);

                            if (i == tokens.size()) {
                                tokens.pop_back();

                                break;
                            }

                            if (auto const op =
                                    std::dynamic_pointer_cast<Operation>(
                                        tokens[i]);
                                op && (op->operation == Operation::op::add ||
                                       op->operation == Operation::op::sub))
                                break;
                        } while (i < tokens.size());

                        if (i == tokens.size() - 1 &&
                            typeid(*tokens[i]) == typeid(Operation)) {
                            tokens.erase(tokens.begin() + i);
                        }

                        continue;
                    }

                    tokens.erase(tokens.begin() + i);

                    continue;
                }

                ++i;

                continue;
            }

            if (constant->value == 1) {
                // Simplifies 1 * expr and 1 ^ expr
                if (auto const operation =
                        std::dynamic_pointer_cast<Operation>(tokens[i + 1])) {
                    if (operation->operation == Operation::op::mul ||
                        operation->operation == Operation::op::pow) {
                        do {
                            tokens.erase(tokens.begin() + i);

                            if (i == tokens.size()) {
                                tokens.erase(tokens.end());

                                break;
                            }

                            if (auto const op =
                                    std::dynamic_pointer_cast<Operation>(
                                        tokens[i]);
                                op && (op->operation == Operation::op::add ||
                                       op->operation == Operation::op::sub))
                                break;
                        } while (i < tokens.size());

                        if (i == tokens.size() - 1 &&
                            typeid(*tokens[i]) == typeid(Operation)) {
                            tokens.erase(tokens.begin() + i);
                        }

                        continue;
                    }
                }
            }

            if (auto const operation =
                    std::dynamic_pointer_cast<Operation>(tokens[i + 1])) {
                if (operation->operation == Operation::op::pow) {
                    if (auto const c = std::dynamic_pointer_cast<Constant>(
                            tokens[i + 2])) {
                        if (c->value == 0) {
                            constant->value == 1;

                            tokens.erase(tokens.begin() + i + 1);
                            tokens.erase(tokens.begin() + i + 1);
                        } else if (constant->value > 0 ||
                                   c->value ==
                                       static_cast<long long>(c->value)) {
                            constant->value =
                                std::powl(constant->value, c->value);

                            tokens.erase(tokens.begin() + i + 1);
                            tokens.erase(tokens.begin() + i + 1);
                        }
                    }
                } else if (operation->operation == Operation::op::mul) {
                    if (auto const c = std::dynamic_pointer_cast<Constant>(
                            tokens[i + 2])) {
                        c->value *= constant->value;

                        tokens.erase(tokens.begin() + i);
                        tokens.erase(tokens.begin() + i);

                        continue;
                    }

                    if (auto const term =
                            std::dynamic_pointer_cast<Term>(tokens[i + 2])) {
                        term->coefficient->value *= constant->value;

                        tokens.erase(tokens.begin() + i);
                        tokens.erase(tokens.begin() + i);

                        continue;
                    }

                    if (auto const function =
                            std::dynamic_pointer_cast<Function>(
                                tokens[i + 2])) {
                        function->coefficient->value *= constant->value;

                        tokens.erase(tokens.begin() + i);
                        tokens.erase(tokens.begin() + i);

                        continue;
                    }

                    token = std::make_shared<Term>(
                        std::dynamic_pointer_cast<Constant>(tokens[i]),
                        tokens[i + 2], std::make_shared<Constant>(1));

                    tokens.erase(tokens.begin() + i + 1);
                    tokens.erase(tokens.begin() + i + 1);
                } else if (operation->operation == Operation::op::div) {
                    if (auto const c = std::dynamic_pointer_cast<Constant>(
                            tokens[i + 2])) {
                        c->value = constant->value / c->value;

                        tokens.erase(tokens.begin() + i);
                        tokens.erase(tokens.begin() + i);

                        continue;
                    }

                    if (auto const term =
                            std::dynamic_pointer_cast<Term>(tokens[i + 2])) {
                        term->coefficient->value /= constant->value;

                        if (auto power = std::dynamic_pointer_cast<Constant>(
                                term->power)) {
                            power->value = -power->value;
                        } else if (auto power = std::dynamic_pointer_cast<Term>(
                                       term->power)) {
                            power->coefficient->value =
                                -power->coefficient->value;
                        } else if (auto power =
                                       std::dynamic_pointer_cast<Function>(
                                           term->power)) {
                            power->coefficient->value =
                                -power->coefficient->value;
                        } else {
                            auto const p = std::make_shared<Term>();
                            p->coefficient = std::make_shared<Constant>(-1);
                            p->base = term->power;

                            term->power = p;
                        }

                        tokens.erase(tokens.begin() + i);
                        tokens.erase(tokens.begin() + i);

                        continue;
                    }

                    if (auto function = std::dynamic_pointer_cast<Function>(
                            tokens[i + 2])) {
                        function->coefficient->value /= constant->value;

                        if (auto power = std::dynamic_pointer_cast<Constant>(
                                function->power)) {
                            power->value = -power->value;
                        } else if (auto power = std::dynamic_pointer_cast<Term>(
                                       function->power)) {
                            power->coefficient->value =
                                -power->coefficient->value;
                        } else if (auto power =
                                       std::dynamic_pointer_cast<Function>(
                                           function->power)) {
                            power->coefficient->value =
                                -power->coefficient->value;
                        } else {
                            auto const p = std::make_shared<Term>();
                            p->coefficient = std::make_shared<Constant>(-1);
                            p->base = function->power;

                            function->power = p;
                        }

                        tokens.erase(tokens.begin() + i);
                        tokens.erase(tokens.begin() + i);

                        continue;
                    }

                    token = std::make_shared<Term>(
                        std::dynamic_pointer_cast<Constant>(tokens[i]),
                        tokens[i + 2], std::make_shared<Constant>(-1));

                    tokens.erase(tokens.begin() + i + 1);
                    tokens.erase(tokens.begin() + i + 1);
                }
            }
        }

        ++i;
    }

    if (tokens.size() == 1)
        return expression->pop_token();

    return expression;
}

Expression tokenise(std::string expression) {
    Expression result{};

    auto e = std::ranges::remove(expression, ' ');

    expression.erase(e.begin(), e.end());

    for (int i = 0; i < expression.size(); ++i) {
        std::shared_ptr<Token> token = get_next_token(expression, i);

        if (!token)
            continue;

        if (auto const function = std::dynamic_pointer_cast<Function>(token)) {
            if (std::vector<std::shared_ptr<Token>> &tokens = result.tokens();
                !tokens.empty())
                if (auto const last =
                        std::dynamic_pointer_cast<Constant>(tokens.back())) {
                    function->coefficient = last;
                    tokens.pop_back();
                } else if (tokens.size() == 2) {
                    if (auto const operation =
                            std::dynamic_pointer_cast<Operation>(tokens.back());
                        operation &&
                        operation->operation == Operation::op::mul) {
                        auto constant = std::dynamic_pointer_cast<Constant>(
                            *(tokens.rbegin() + 1));

                        if (constant) {
                            function->coefficient = constant;
                            tokens.pop_back();
                            tokens.pop_back();
                        }
                    }
                }

            if (!function->coefficient)
                function->coefficient = std::make_shared<Constant>(1);

            if (i < expression.size())
                if (Operation operation{expression[i + 1]};
                    operation.operation &&
                    operation.operation == Operation::op::pow) {
                    i += 2;

                    function->power = get_next_token(expression, i);
                }

            if (!function->power)
                function->power = std::make_shared<Constant>(1);
        }

        if (std::vector<std::shared_ptr<Token>> const &tokens = result.tokens();
            !tokens.empty() && typeid(*token) != typeid(Operation) &&
            typeid(*tokens.back()) == typeid(Constant))
            result.add_token(std::make_shared<Operation>('*'));

        result.add_token(token);
    }

    return result;
}

std::ostream &operator<<(std::ostream &os, Token const &token) {
    os << static_cast<std::string>(token);

    return os;
}