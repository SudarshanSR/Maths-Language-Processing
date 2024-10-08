#include "token.h"

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>

namespace {
std::map<char, char> const k_parenthesis_map{
    {'(', ')'}, {'[', ']'}, {'{', '}'}};

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

            else if (character == k_parenthesis_map.at(original))
                --count;

            character = expression[++i];
        }

        --i;

        if (count)
            throw std::invalid_argument("Expression is not valid!");

        std::size_t const end = i - start;

        return tokenise(expression.substr(start, end));
    }

    for (int const offset : {4, 3, 2}) {
        if (i + offset >= expression.size())
            continue;

        std::string const &fn = expression.substr(i, offset);

        if (!k_functions.contains(fn))
            continue;

        i += offset;

        return std::make_shared<Function>(fn, get_next_token(expression, i));
    }

    if (character == 'e' && i < expression.size() - 1 &&
        expression[i + 1] == '^') {
        i += 2;

        return std::make_shared<Function>("e^", get_next_token(expression, i));
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

bool Constant::operator==(Constant const &constant) const {
    return this->value == constant.value;
}

Variable::Variable(char const var) : var(var) {}

Variable::operator std::string() const { return {this->var}; }

bool Variable::operator==(Variable const &variable) const {
    return this->var == variable.var;
}

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
                   std::shared_ptr<Token> const &parameter)
    : function(std::move(function)), parameter(parameter) {}

Function::operator std::string() const {
    std::stringstream result;

    result << this->function << '('
           << static_cast<std::string>(*this->parameter) << ')';

    return result.str();
}

void Function::simplify() {
    if (auto const &type_info = typeid(*this->parameter);
        type_info == typeid(Expression))
        this->parameter =
            ::simplify(std::dynamic_pointer_cast<Expression>(this->parameter));

    else if (type_info == typeid(Term))
        this->parameter =
            ::simplify(std::dynamic_pointer_cast<Term>(this->parameter));

    else if (type_info == typeid(Function))
        std::dynamic_pointer_cast<Function>(this->parameter)->simplify();
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
        if (typeid(*this->power) == typeid(Constant)) {
            if (std::dynamic_pointer_cast<Constant>(this->power)->value != 1) {
                result << '^';
                result << *this->power;
            }
        } else {
            result << '^';
            result << *this->power;
        }
    }

    return result.str();
}

Expression::operator std::string() const {
    std::stringstream result;

    if (this->tokens.size() == 1) {
        result << static_cast<std::string>(*this->tokens[0]);
    } else {
        result << '(';

        for (std::shared_ptr<Token> const &token : this->tokens)
            result << static_cast<std::string>(*token);

        result << ')';
    }

    return result.str();
}

void Expression::add_token(std::shared_ptr<Token> const &token) {
    this->tokens.push_back(token);
}

std::shared_ptr<Token> Expression::pop_token() {
    std::shared_ptr<Token> token = this->tokens.back();

    this->tokens.pop_back();

    return token;
}

std::shared_ptr<Token> simplify(std::shared_ptr<Token> const &token) {
    if (typeid(*token) == typeid(Expression))
        return simplify(std::dynamic_pointer_cast<Expression>(token));

    if (typeid(*token) == typeid(Term))
        return simplify(std::dynamic_pointer_cast<Term>(token));

    if (typeid(*token) == typeid(Function)) {
        auto const function = std::dynamic_pointer_cast<Function>(token);

        function->simplify();

        return function;
    }

    return token;
}

std::shared_ptr<Token> simplify(std::shared_ptr<Term> const &term) {
    if (!term->coefficient)
        term->coefficient = std::make_shared<Constant>(1);

    if (!term->power)
        term->power = std::make_shared<Constant>(1);

    if (typeid(*term->power) == typeid(Constant)) {
        if (term->coefficient->value == 1 &&
            std::dynamic_pointer_cast<Constant>(term->power)->value == 1)
            return simplify(term->base);

        if (std::dynamic_pointer_cast<Constant>(term->power)->value == 0)
            return term->coefficient;
    }

    if (typeid(*term->base) == typeid(Constant)) {
        auto const base = std::dynamic_pointer_cast<Constant>(term->base);

        if (typeid(*term->power) == typeid(Constant))
            return std::make_shared<Constant>(
                term->coefficient->value *
                std::powl(
                    base->value,
                    std::dynamic_pointer_cast<Constant>(term->power)->value));

        if (*term->coefficient == *base) {
            if (typeid(*term->power) == typeid(Expression)) {
                auto const power =
                    std::dynamic_pointer_cast<Expression>(term->power);
                power->add_token(std::make_shared<Operation>('+'));
                power->add_token(std::make_shared<Constant>(1));
                term->coefficient->value = 1;
            }
        }
    }

    term->base = simplify(term->base);
    term->power = simplify(term->power);

    return term;
}

std::shared_ptr<Token> simplify(std::shared_ptr<Expression> expression) {
    std::vector<std::shared_ptr<Token>> &tokens = expression->tokens;

    if (tokens.size() == 1)
        return simplify(expression->pop_token());

    int i = 0;

    while (i < tokens.size()) {
        std::shared_ptr<Token> &token = tokens[i];

        if (auto const &token_Type = typeid(*token);
            token_Type == typeid(Expression) || token_Type == typeid(Term) ||
            token_Type == typeid(Function)) {
            simplify(token);
        }

        ++i;
    }

    for (i = 1; i < tokens.size(); ++i) {
        std::shared_ptr<Token> &token = tokens[i];

        if (typeid(*token) != typeid(Operation))
            continue;

        if (i == tokens.size())
            throw std::invalid_argument("Expression is not valid!");

        if (auto operation = std::dynamic_pointer_cast<Operation>(token);
            operation->operation != Operation::op::pow)
            continue;

        std::shared_ptr<Token> left = tokens[i - 1];
        std::shared_ptr<Token> right = tokens[i + 1];

        if (typeid(*left) == typeid(Constant)) {
            auto left_constant = std::dynamic_pointer_cast<Constant>(left);

            if (left_constant->value == 0 || left_constant->value == 1) {
                tokens.erase(tokens.begin() + i);
                tokens.erase(tokens.begin() + i);

                --i;

                continue;
            }

            if (typeid(*right) == typeid(Constant)) {
                auto right_constant =
                    std::dynamic_pointer_cast<Constant>(right);

                left_constant->value =
                    std::powl(left_constant->value, right_constant->value);

                tokens.erase(tokens.begin() + i);
                tokens.erase(tokens.begin() + i);

                --i;

                continue;
            }
        }

        if (typeid(*right) == typeid(Constant)) {
            auto right_constant = std::dynamic_pointer_cast<Constant>(right);

            if (right_constant->value == 1) {
                tokens.erase(tokens.begin() + i);
                tokens.erase(tokens.begin() + i);

                --i;

                continue;
            }

            if (right_constant->value == 0) {
                --i;

                tokens.erase(tokens.begin() + i);
                tokens.erase(tokens.begin() + i);

                right_constant->value = 1;

                continue;
            }
        }

        auto term =
            std::make_shared<Term>(std::make_shared<Constant>(1), left, right);

        tokens[i - 1] = term;

        tokens.erase(tokens.begin() + i);
        tokens.erase(tokens.begin() + i);

        --i;
    }

    for (i = 1; i < tokens.size(); ++i) {
        std::shared_ptr<Token> &token = tokens[i];

        if (typeid(*token) != typeid(Operation))
            continue;

        if (i == tokens.size())
            throw std::invalid_argument("Expression is not valid!");

        auto operation = std::dynamic_pointer_cast<Operation>(token);

        if (operation->operation == Operation::op::add ||
            operation->operation == Operation::op::sub ||
            operation->operation == Operation::op::div)
            continue;

        std::shared_ptr<Token> left = tokens[i - 1];
        std::shared_ptr<Token> right = tokens[i + 1];

        if (operation->operation == Operation::op::mul) {
            if (typeid(*left) == typeid(Constant)) {
                auto left_constant = std::dynamic_pointer_cast<Constant>(left);

                if (left_constant->value == 0) {
                    tokens.erase(tokens.begin() + i);
                    tokens.erase(tokens.begin() + i);

                    --i;

                    continue;
                }

                if (left_constant->value == 1) {
                    --i;

                    tokens.erase(tokens.begin() + i);
                    tokens.erase(tokens.begin() + i);

                    continue;
                }

                if (typeid(*right) == typeid(Constant)) {
                    auto right_constant =
                        std::dynamic_pointer_cast<Constant>(right);
                    right_constant->value *= left_constant->value;
                } else if (typeid(*right) == typeid(Term)) {
                    auto right_term = std::dynamic_pointer_cast<Term>(right);

                    right_term->coefficient->value *= left_constant->value;
                } else if (typeid(*right) == typeid(Variable)) {
                    auto right_variable =
                        std::dynamic_pointer_cast<Variable>(right);

                    tokens[i + 1] =
                        std::make_shared<Term>(left_constant, right_variable,
                                               std::make_shared<Constant>(1));
                } else {
                    tokens[i + 1] = std::make_shared<Term>(
                        left_constant, right, std::make_shared<Constant>(1));
                }
            } else if (typeid(*right) == typeid(Constant)) {
                auto right_constant =
                    std::dynamic_pointer_cast<Constant>(right);

                if (right_constant->value == 0) {
                    --i;

                    tokens.erase(tokens.begin() + i);
                    tokens.erase(tokens.begin() + i);

                    continue;
                }

                if (right_constant->value == 1) {
                    tokens.erase(tokens.begin() + i);
                    tokens.erase(tokens.begin() + i);

                    --i;

                    continue;
                }

                if (typeid(*left) == typeid(Term)) {
                    auto left_term = std::dynamic_pointer_cast<Term>(left);

                    left_term->coefficient->value *= right_constant->value;

                    tokens[i + 1] = left;
                } else if (typeid(*left) == typeid(Variable)) {
                    auto left_variable =
                        std::dynamic_pointer_cast<Variable>(left);

                    tokens[i + 1] =
                        std::make_shared<Term>(right_constant, left_variable,
                                               std::make_shared<Constant>(1));
                } else {
                    tokens[i + 1] = std::make_shared<Term>(
                        right_constant, left, std::make_shared<Constant>(1));
                }
            } else {
                if (tokens.size() == 3)
                    continue;

                auto result = std::make_shared<Expression>();
                result->add_token(left);
                result->add_token(operation);
                result->add_token(right);

                tokens[i + 1] = result;
            }
        } else if (operation->operation == Operation::op::div) {
            if (typeid(*left) == typeid(Constant)) {
                auto left_constant = std::dynamic_pointer_cast<Constant>(left);

                if (left_constant->value == 0) {
                    tokens.erase(tokens.begin() + i);
                    tokens.erase(tokens.begin() + i);

                    --i;

                    continue;
                }

                tokens[i + 1] = std::make_shared<Term>(
                    left_constant, right, std::make_shared<Constant>(-1));

                --i;

                tokens.erase(tokens.begin() + i);
                tokens.erase(tokens.begin() + i);

                --i;

                continue;
            }

            auto result = std::make_shared<Expression>();
            result->add_token(left);
            result->add_token(std::make_shared<Operation>('*'));
            result->add_token(
                std::make_shared<Term>(std::make_shared<Constant>(1), right,
                                       std::make_shared<Constant>(-1)));

            tokens[i + 1] = result;
        }

        --i;

        tokens.erase(tokens.begin() + i);
        tokens.erase(tokens.begin() + i);
    }

    for (i = 1; i < tokens.size(); ++i) {
        std::shared_ptr<Token> &token = tokens[i];

        if (typeid(*token) != typeid(Operation))
            continue;

        if (i == tokens.size())
            throw std::invalid_argument("Expression is not valid!");

        std::shared_ptr<Token> left = tokens[i - 1];
        std::shared_ptr<Token> right = tokens[i + 1];

        auto operation = std::dynamic_pointer_cast<Operation>(token);

        if (!operation)
            continue;

        if (operation->operation == Operation::op::add) {
            if (typeid(*left) == typeid(Constant)) {
                if (auto left_constant =
                        std::dynamic_pointer_cast<Constant>(left);
                    left_constant->value == 0) {
                    --i;

                    tokens.erase(tokens.begin() + i);
                    tokens.erase(tokens.begin() + i);
                } else if (typeid(*right) == typeid(Constant)) {
                    auto right_constant =
                        std::dynamic_pointer_cast<Constant>(right);

                    right_constant->value += left_constant->value;

                    --i;

                    tokens.erase(tokens.begin() + i);
                    tokens.erase(tokens.begin() + i);
                }

                continue;
            }

            if (typeid(*right) == typeid(Constant)) {
                auto right_constant =
                    std::dynamic_pointer_cast<Constant>(right);

                if (right_constant->value != 0)
                    continue;

                tokens[i + 1] = left;

                --i;

                tokens.erase(tokens.begin() + i);
                tokens.erase(tokens.begin() + i);
            }
        } else if (operation->operation == Operation::op::sub) {
            if (typeid(*left) == typeid(Constant)) {
                if (auto left_constant =
                        std::dynamic_pointer_cast<Constant>(left);
                    left_constant->value == 0) {
                    tokens.erase(tokens.begin() + i - 1);
                } else if (typeid(*right) == typeid(Constant)) {
                    auto right_constant =
                        std::dynamic_pointer_cast<Constant>(right);

                    right_constant->value =
                        left_constant->value - right_constant->value;

                    --i;

                    tokens.erase(tokens.begin() + i);
                    tokens.erase(tokens.begin() + i);
                }

                continue;
            }

            if (typeid(*right) == typeid(Constant)) {
                auto right_constant =
                    std::dynamic_pointer_cast<Constant>(right);
                if (right_constant->value != 0)
                    continue;

                tokens[i + 1] = left;

                --i;

                tokens.erase(tokens.begin() + i);
                tokens.erase(tokens.begin() + i);
            }
        }
    }

    i = 0;

    while (i < tokens.size()) {
        std::shared_ptr<Token> &token = tokens[i];

        if (auto const &token_Type = typeid(*token);
            token_Type == typeid(Expression) || token_Type == typeid(Term) ||
            token_Type == typeid(Function)) {
            simplify(token);
        }

        ++i;
    }

    if (tokens.size() == 1)
        return expression->pop_token();

    i = 0;

    while (i < tokens.size()) {
        std::shared_ptr<Token> &token = tokens[i];

        if (auto const &token_type = typeid(*token);
            token_type == typeid(Operation)) {
            auto const operation = std::dynamic_pointer_cast<Operation>(token);

            if (i == tokens.size() - 1)
                throw std::invalid_argument("Expression is not valid!");

            if (operation->operation == Operation::op::sub) {
                if (std::shared_ptr<Token> next = tokens[i + 1];
                    typeid(*next) == typeid(Constant)) {
                    auto const constant =
                        std::dynamic_pointer_cast<Constant>(next);

                    constant->value = -constant->value;

                    operation->operation = Operation::op::add;
                } else if (typeid(*next) == typeid(Term)) {
                    auto const term = std::dynamic_pointer_cast<Term>(next);

                    term->coefficient->value = -term->coefficient->value;

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
        } else if (token_type == typeid(Constant)) {
            auto const constant = std::dynamic_pointer_cast<Constant>(token);

            if (i == tokens.size() - 1) {
                ++i;

                continue;
            }

            if (constant->value == 0) {
                if (typeid(*tokens[i + 1]) != typeid(Operation)) {
                    ++i;

                    continue;
                }

                auto const operation =
                    std::dynamic_pointer_cast<Operation>(tokens[i + 1]);

                // Simplifies 0 / expr

                if (operation->operation != Operation::op::div) {
                    tokens.erase(tokens.begin() + i);

                    continue;
                }

                do {
                    tokens.erase(tokens.begin() + i);

                    if (tokens.empty())
                        break;

                    if (i == tokens.size()) {
                        tokens.pop_back();

                        break;
                    }

                    if (typeid(*tokens[i]) != typeid(Operation))
                        continue;

                    if (auto const op =
                            std::dynamic_pointer_cast<Operation>(tokens[i]);
                        op->operation == Operation::op::add ||
                        op->operation == Operation::op::sub)
                        break;
                } while (i < tokens.size());

                if (i == tokens.size() - 1 &&
                    typeid(*tokens[i]) == typeid(Operation)) {
                    tokens.erase(tokens.begin() + i);
                }

                continue;
            }

            if (typeid(*tokens[i + 1]) == typeid(Operation)) {
                auto const operation =
                    std::dynamic_pointer_cast<Operation>(tokens[i + 1]);

                if (operation->operation == Operation::op::div) {
                    if (typeid(*tokens[i + 2]) == typeid(Constant)) {
                        auto const c =
                            std::dynamic_pointer_cast<Constant>(tokens[i + 2]);

                        c->value = constant->value / c->value;

                        tokens.erase(tokens.begin() + i);
                        tokens.erase(tokens.begin() + i);

                        continue;
                    }

                    if (typeid(*tokens[i + 2]) == typeid(Term)) {
                        auto const term =
                            std::dynamic_pointer_cast<Term>(tokens[i + 2]);

                        term->coefficient->value /= constant->value;

                        if (auto const &power_type = typeid(*term->power);
                            power_type == typeid(Constant)) {
                            auto power = std::dynamic_pointer_cast<Constant>(
                                term->power);

                            power->value = -power->value;
                        } else if (power_type == typeid(Term)) {
                            auto power =
                                std::dynamic_pointer_cast<Term>(term->power);

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

                    token = std::make_shared<Term>(
                        std::dynamic_pointer_cast<Constant>(tokens[i]),
                        tokens[i + 2], std::make_shared<Constant>(-1));

                    tokens.erase(tokens.begin() + i + 1);
                    tokens.erase(tokens.begin() + i + 1);
                }
            }
        } else if (token_type == typeid(Term)) {
            auto const term = std::dynamic_pointer_cast<Term>(token);

            if (i == tokens.size() - 1 ||
                typeid(*tokens[i + 1]) != typeid(Operation)) {
                ++i;

                continue;
            }

            auto operation =
                std::dynamic_pointer_cast<Operation>(tokens[i + 1]);

            if (operation->operation == Operation::op::pow) {
                if (!term->power) {
                    term->power = tokens[i + 2];
                } else {
                    if (auto const &power_type = typeid(*term->power);
                        power_type == typeid(Constant) &&
                        typeid(*tokens[i + 1]) == typeid(Constant)) {
                        std::dynamic_pointer_cast<Constant>(term->power)
                            ->value *=
                            std::dynamic_pointer_cast<Constant>(tokens[i + 2])
                                ->value;
                    } else if (power_type == typeid(Constant)) {
                        auto power = std::make_shared<Term>(
                            std::dynamic_pointer_cast<Constant>(term->power),
                            tokens[i + 2], std::make_shared<Constant>(1));

                        term->power = power;
                    } else if (typeid(*tokens[i + 1]) == typeid(Constant)) {
                        auto power = std::make_shared<Term>(
                            std::dynamic_pointer_cast<Constant>(tokens[i + 2]),
                            term->power, std::make_shared<Constant>(1));

                        term->power = power;
                    } else {
                        auto power = std::make_shared<Expression>();
                        power->add_token(term->power);
                        power->add_token(std::make_shared<Operation>('*'));
                        power->add_token(tokens[i + 2]);

                        term->power = power;
                    }
                }

                tokens.erase(tokens.begin() + i + 1);
                tokens.erase(tokens.begin() + i + 1);
            } else if (operation->operation == Operation::op::mul &&
                       typeid(*term->coefficient) == typeid(Constant) &&
                       typeid(*tokens[i + 2]) == typeid(Constant)) {
                std::dynamic_pointer_cast<Constant>(term->coefficient)->value *=
                    std::dynamic_pointer_cast<Constant>(tokens[i + 2])->value;

                tokens.erase(tokens.begin() + i + 1);
                tokens.erase(tokens.begin() + i + 1);
            } else if (operation->operation == Operation::op::div &&
                       typeid(*term->coefficient) == typeid(Constant) &&
                       typeid(*tokens[i + 2]) == typeid(Constant)) {
                std::dynamic_pointer_cast<Constant>(term->coefficient)->value /=
                    std::dynamic_pointer_cast<Constant>(tokens[i + 2])->value;

                tokens.erase(tokens.begin() + i + 1);
                tokens.erase(tokens.begin() + i + 1);
            }
        }

        ++i;
    }

    if (tokens.size() == 1)
        return expression->pop_token();

    return expression;
}

std::shared_ptr<Token> tokenise(std::string expression) {
    auto const result = std::make_shared<Expression>();

    auto e = std::ranges::remove(expression, ' ');

    expression.erase(e.begin(), e.end());

    for (int i = 0; i < expression.size(); ++i) {
        std::shared_ptr<Token> token = get_next_token(expression, i);

        if (!token)
            continue;

        if (std::vector<std::shared_ptr<Token>> const &tokens = result->tokens;
            !tokens.empty() && typeid(*token) != typeid(Operation) &&
            typeid(*tokens.back()) != typeid(Operation))
            result->add_token(std::make_shared<Operation>('*'));

        result->add_token(token);
    }

    return simplify(result);
}

std::ostream &operator<<(std::ostream &os, Token const &token) {
    os << static_cast<std::string>(token);

    return os;
}