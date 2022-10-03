#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>

using namespace std;

namespace parse {

    bool operator==(const Token& lhs, const Token& rhs) {
        using namespace token_type;

        if (lhs.index() != rhs.index()) {
            return false;
        }
        if (lhs.Is<Char>()) {
            return lhs.As<Char>().value == rhs.As<Char>().value;
        }
        if (lhs.Is<Number>()) {
            return lhs.As<Number>().value == rhs.As<Number>().value;
        }
        if (lhs.Is<String>()) {
            return lhs.As<String>().value == rhs.As<String>().value;
        }
        if (lhs.Is<Id>()) {
            return lhs.As<Id>().value == rhs.As<Id>().value;
        }
        return true;
    }

    bool operator!=(const Token& lhs, const Token& rhs) {
        return !(lhs == rhs);
    }

    std::ostream& operator<<(std::ostream& os, const Token& rhs) {
        using namespace token_type;

#define VALUED_OUTPUT(type) \
    if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

        VALUED_OUTPUT(Number);
        VALUED_OUTPUT(Id);
        VALUED_OUTPUT(String);
        VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

        UNVALUED_OUTPUT(Class);
        UNVALUED_OUTPUT(Return);
        UNVALUED_OUTPUT(If);
        UNVALUED_OUTPUT(Else);
        UNVALUED_OUTPUT(Def);
        UNVALUED_OUTPUT(Newline);
        UNVALUED_OUTPUT(Print);
        UNVALUED_OUTPUT(Indent);
        UNVALUED_OUTPUT(Dedent);
        UNVALUED_OUTPUT(And);
        UNVALUED_OUTPUT(Or);
        UNVALUED_OUTPUT(Not);
        UNVALUED_OUTPUT(Eq);
        UNVALUED_OUTPUT(NotEq);
        UNVALUED_OUTPUT(LessOrEq);
        UNVALUED_OUTPUT(GreaterOrEq);
        UNVALUED_OUTPUT(None);
        UNVALUED_OUTPUT(True);
        UNVALUED_OUTPUT(False);
        UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

        return os << "Unknown token :("sv;
    }

    Lexer::Lexer(std::istream& input) 
        : input_(input)
        , new_line_flag_(true)
    {
        NextToken();
    }

    const Token& Lexer::CurrentToken() const {
        return current_token_;
        //throw std::logic_error("Not implemented"s);
    }

    Token Lexer::NextToken() {
        current_token_ = ParseToken();
        return CurrentToken();
        //throw std::logic_error("Not implemented"s);
    }

    Token Lexer::ParseToken() {
        char c;
        while (input_.get(c)) {
            if (c == '\n') {
                if (!new_line_flag_) {
                    return ParseNewLine();
                }
                else {
                    continue;
                }
            }
            if (c == ' ') {
                if (new_line_flag_) {
                    new_line_flag_ = false;
                    input_.putback(c);
                    Token token = ParseIndent();
                    if (token == token_type::None()) {
                        continue;
                    }
                    return token;
                }
                else {
                    continue;
                }
            }
            else {
                if (new_line_flag_ && (current_indent_ != 0)) {
                    input_.putback(c);
                    return ParseDedent();
                }
            }
            if (isalpha(c) || c == '_') {
                new_line_flag_ = false;
                input_.putback(c);
                return ParseId();
            }
            if (isdigit(c)) {
                new_line_flag_ = false;
                input_.putback(c);
                return ParseNumber();
            }
            if (ispunct(c)) {
                if (c == '\"' || c == '\'') {
                    new_line_flag_ = false;
                    input_.putback(c);
                    return ParseString();
                }
                else if (c == '#') {
                    Token token = ParseComment();
                    if (token == token_type::None()) {
                        continue;
                    }
                    else {
                        return token;
                    }
                }
                else {
                    new_line_flag_ = false;
                    input_.putback(c);
                    return ParseChar();
                }
            }
        }
        if (current_indent_ == 0) {
            if (new_line_flag_) {
                return token_type::Eof();
            }
            else {
                return ParseNewLine();
            }
        }
        else {
            return ParseDedent();
        }
    }

    Token Lexer::ParseString() {
        char open_quote = input_.get();
        string str;
        while (true) {
            char c = input_.get();
            if (c == '\\') {
                const char escaped_char = input_.get();
                switch (escaped_char) {
                case 'n':
                    str.push_back('\n');
                    break;
                case 't':
                    str.push_back('\t');
                    break;
                case 'r':
                    str.push_back('\r');
                    break;
                case '"':
                    str.push_back('\"');
                    break;
                case '\'':
                    str.push_back('\'');
                    break;
                case '\\':
                    str.push_back('\\');
                    break;
                default:
                    str.push_back(escaped_char);
                }
            }
            else if (c == '\"' || c == '\'') {
                if (c == open_quote) {
                    break;
                }
                str.push_back(c);
            }
            else {
                str.push_back(c);
            }
            
        }
        return token_type::String{ str };
    }

    Token Lexer::ParseNumber() {
        string parse_num;
        while (isdigit(input_.peek())) {
            parse_num.push_back(static_cast<char>(input_.get()));
        }
        return token_type::Number{ stoi(parse_num) };
    }

    Token Lexer::ParseId() {
        string str;
        while (std::isalpha(input_.peek()) || std::isdigit(input_.peek()) || input_.peek() == '_') {
            str.push_back(static_cast<char>(input_.get()));
        }
        if (key_words_.count(str)) {
            return ParseKeyWord(str);
        }
        else {
            return token_type::Id{ str };
        }
    }

    Token Lexer::ParseChar() {
        char c = input_.get();
        if (c == '<' || c == '>' || c == '!' || c == '=') {
            if (input_.peek() == '=') {
                string str;
                str.push_back(c);
                str.push_back(static_cast<char>(input_.get()));
                return ParseKeyWord(str);
            }
        }
        if (chars_.count(c)) {
            return token_type::Char{ c };
        }
        else {
            throw LexerError("Invalid char"s);
        }
    }

    Token Lexer::ParseKeyWord(string str) {
        if (str == "class"s) {
            return token_type::Class();
        }
        if (str == "return"s) {
            return token_type::Return();
        }
        if (str == "if"s) {
            return token_type::If();
        }
        if (str == "else"s) {
            return token_type::Else();
        }
        if (str == "def"s) {
            return token_type::Def();
        }
        if (str == "print"s) {
            return token_type::Print();
        }
        if (str == "and"s) {
            return token_type::And();
        }
        if (str == "or"s) {
            return token_type::Or();
        }
        if (str == "not"s) {
            return token_type::Not();
        }
        if (str == "=="s) {
            return token_type::Eq();
        }
        if (str == "!="s) {
            return token_type::NotEq();
        }
        if (str == "<="s) {
            return token_type::LessOrEq();
        }
        if (str == ">="s) {
            return token_type::GreaterOrEq();
        }
        if (str == "None"s) {
            return token_type::None();
        }
        if (str == "True"s) {
            return token_type::True();
        }
        if (str == "False"s) {
            return token_type::False();
        }
        throw LexerError("Invalid Key Word"s);
    }

    Token Lexer::ParseIndent() {
        size_t space_counter = 0;
        char c;
        while (input_.get(c)) {
            if (c == '\n') {
                return token_type::None();
            }
            if (c == ' ') {
                ++space_counter;
            }
            else {
                input_.putback(c);
                if (space_counter % 2 == 0) {
                    if (space_counter / 2 > current_indent_) {
                        ++current_indent_;
                        return token_type::Indent();
                    }
                    if (space_counter / 2 < current_indent_) {
                        if ((space_counter / 2) < (current_indent_ - 1)) {
                            new_line_flag_ = true;
                            input_.putback(' ');
                            input_.putback(' ');
                        }
                        return ParseDedent();
                    }
                    return token_type::None();
                }
                else {
                    throw LexerError("Invalid number of spaces"s);
                }
            }
        }
        return token_type::None();
    }

    Token Lexer::ParseDedent() {
        --current_indent_;
        return token_type::Dedent();
    }

    Token Lexer::ParseNewLine() {
        new_line_flag_ = true;
        return token_type::Newline();
    }

    Token Lexer::ParseComment() {
        char c;
        while (input_.get(c) && c != '\n') {
            continue;
        }
        if (new_line_flag_) {
            return token_type::None();
        }
        else {
            return ParseNewLine();
        }
    }
}  // namespace parse