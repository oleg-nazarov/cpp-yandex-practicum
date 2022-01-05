#pragma once

#include <iosfwd>
#include <optional>
#include <queue>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>

namespace parse {

namespace token_type {

struct Number {  // Лексема «число»
    int value;   // число
};

struct Id {             // Лексема «идентификатор»
    std::string value;  // Имя идентификатора
};

struct Char {    // Лексема «символ»
    char value;  // код символа
};

struct String {  // Лексема «строковая константа»
    std::string value;
};

struct Class {};        // Лексема «class»
struct Return {};       // Лексема «return»
struct If {};           // Лексема «if»
struct Else {};         // Лексема «else»
struct Def {};          // Лексема «def»
struct Newline {};      // Лексема «конец строки»
struct Print {};        // Лексема «print»
struct Indent {};       // Лексема «увеличение отступа», соответствует двум пробелам
struct Dedent {};       // Лексема «уменьшение отступа»
struct Eof {};          // Лексема «конец файла»
struct And {};          // Лексема «and»
struct Or {};           // Лексема «or»
struct Not {};          // Лексема «not»
struct Eq {};           // Лексема «==»
struct NotEq {};        // Лексема «!=»
struct LessOrEq {};     // Лексема «<=»
struct GreaterOrEq {};  // Лексема «>=»
struct None {};         // Лексема «None»
struct True {};         // Лексема «True»
struct False {};        // Лексема «False»

}  // namespace token_type

using TokenBase = std::variant<token_type::Number, token_type::Id, token_type::Char, token_type::String,
                               token_type::Class, token_type::Return, token_type::If, token_type::Else,
                               token_type::Def, token_type::Newline, token_type::Print, token_type::Indent,
                               token_type::Dedent, token_type::And, token_type::Or, token_type::Not,
                               token_type::Eq, token_type::NotEq, token_type::LessOrEq, token_type::GreaterOrEq,
                               token_type::None, token_type::True, token_type::False, token_type::Eof>;

struct Token : TokenBase {
    using TokenBase::TokenBase;

    template <typename T>
    [[nodiscard]] bool Is() const {
        return std::holds_alternative<T>(*this);
    }

    template <typename T>
    [[nodiscard]] const T& As() const {
        return std::get<T>(*this);
    }

    template <typename T>
    [[nodiscard]] const T* TryAs() const {
        return std::get_if<T>(this);
    }
};

bool operator==(const Token& lhs, const Token& rhs);
bool operator!=(const Token& lhs, const Token& rhs);

std::ostream& operator<<(std::ostream& os, const Token& rhs);

class LexerError : public std::runtime_error {
   public:
    using std::runtime_error::runtime_error;
};

class Lexer {
   public:
    explicit Lexer(std::istream& input);

    [[nodiscard]] const Token& CurrentToken() const;

    Token NextToken();

    template <typename T>
    const T& Expect() const;
    template <typename T, typename U>
    void Expect(const U& value) const;

    template <typename T>
    const T& ExpectNext();
    template <typename T, typename U>
    void ExpectNext(const U& value);

   private:
    int indent_count_ = 0;
    std::istream& input_;
    std::queue<Token> parsed_tokens_;
    Token curr_token_;

    const static std::regex number_re_;
    const static std::regex id_re_;
    const static std::regex string_single_quote_re_;
    const static std::regex string_double_quote_re_;
    const static std::regex char_re_;
    const static std::regex non_comment_re_;
    const static std::regex end_spaces_re_;
    const static std::regex start_spaces_re_;
    const static std::regex class_re_;
    const static std::regex return_re_;
    const static std::regex if_re_;
    const static std::regex else_re_;
    const static std::regex def_re_;
    const static std::regex print_re_;
    const static std::regex and_re_;
    const static std::regex or_re_;
    const static std::regex not_re_;
    const static std::regex eq_re_;
    const static std::regex not_eq_re_;
    const static std::regex less_or_eq_re_;
    const static std::regex greater_or_eq_re_;
    const static std::regex none_re_;
    const static std::regex true_re_;
    const static std::regex false_re_;

    void ParseLine(std::string&& line);

    static void EraseCommentAndSpacesBefore(std::string& line);
    void ParseIndentDedent(std::string& line);
    void AddEOFAndPrecedingDedents();

    void EraseEscapeSequences(std::string& s);
    void ReplaceOneEscapeSequence(std::string& s, std::string&& old_s, std::string&& new_s);

    template <typename T>
    static Token GetConstructedToken(const std::optional<std::string>& value_opt);
};

template <typename T>
const T& Lexer::Expect() const {
    using namespace std::literals;

    if (auto p = curr_token_.TryAs<T>()) {
        return *p;
    }

    std::ostringstream oss;
    oss << curr_token_;
    throw LexerError("Unexpected token type: "s + oss.str());
}

template <typename T, typename U>
void Lexer::Expect(const U& value) const {
    using namespace std::literals;

    const T& token = Expect<T>();

    if (token.value != value) {
        throw LexerError("Unexpected token value"s);
    }
}

template <typename T>
const T& Lexer::ExpectNext() {
    using namespace std::literals;

    NextToken();

    return Expect<T>();
}

template <typename T, typename U>
void Lexer::ExpectNext(const U& value) {
    using namespace std::literals;

    NextToken();

    return Expect<T>(value);
}

template <typename T>
Token Lexer::GetConstructedToken(const std::optional<std::string>& value_opt) {
    using namespace std::literals;

    // valued tokens
    if (std::is_same_v<T, token_type::Number>) {
        return Token(token_type::Number{std::stoi(*value_opt)});
    }
    if (std::is_same_v<T, token_type::Id>) {
        return Token(token_type::Id{std::move(*value_opt)});
    }
    if (std::is_same_v<T, token_type::String>) {
        return Token(token_type::String{std::move(*value_opt)});
    }
    if (std::is_same_v<T, token_type::Char>) {
        return Token(token_type::Char{(*value_opt)[0]});
    }

    // unvalued token
    return Token(T{});
}

}  // namespace parse