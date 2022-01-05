#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <optional>
#include <queue>
#include <regex>
#include <string>
#include <type_traits>
#include <utility>

using namespace std;

namespace parse {

// use sm[1]
const regex Lexer::number_re_(R"(^\s*(\d+))");
const regex Lexer::id_re_(R"(^\s*([A-Za-z_][A-Za-z0-9_]*))");
const regex Lexer::string_single_quote_re_(R"(^\s*'(.*?[^\\])?')");
const regex Lexer::string_double_quote_re_(R"(^\s*\"(.*?[^\\])?\")");
const regex Lexer::char_re_(R"(^\s*([<>=,():\.\+\-\*/]))");
// now sm[x] is not used
const regex Lexer::non_comment_re_(R"(([^#'\"]*(('[^']*')|(\"[^\"]*\"))*)*[^#]*)");
const regex Lexer::end_spaces_re_(R"(\s*$)");
const regex Lexer::start_spaces_re_(R"(^\s*)");
const regex Lexer::class_re_(R"(^\s*class\b)");
const regex Lexer::return_re_(R"(^\s*return\b)");
const regex Lexer::if_re_(R"(^\s*if\b)");
const regex Lexer::else_re_(R"(^\s*else\b)");
const regex Lexer::def_re_(R"(^\s*def\b)");
const regex Lexer::print_re_(R"(^\s*print\b)");
const regex Lexer::and_re_(R"(^\s*and\b)");
const regex Lexer::or_re_(R"(^\s*or\b)");
const regex Lexer::not_re_(R"(^\s*not\b)");
const regex Lexer::eq_re_(R"(^\s*==)");
const regex Lexer::not_eq_re_(R"(^\s*!=)");
const regex Lexer::less_or_eq_re_(R"(^\s*<=)");
const regex Lexer::greater_or_eq_re_(R"(^\s*>=)");
const regex Lexer::none_re_(R"(^\s*None\b)");
const regex Lexer::true_re_(R"(^\s*True\b)");
const regex Lexer::false_re_(R"(^\s*False\b)");

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

ostream& operator<<(ostream& os, const Token& rhs) {
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

Lexer::Lexer(istream& input) : input_(input) {
    NextToken();
}

const Token& Lexer::CurrentToken() const {
    return curr_token_;
}

Token Lexer::NextToken() {
    if (curr_token_.Is<token_type::Eof>()) {
        return curr_token_;
    }

    if (parsed_tokens_.empty()) {
        string line;
        if (getline(input_, line)) {
            ParseLine(move(line));
            // the line can be empty or only with a comment, so we must go through all "if" again
            return NextToken();
        }

        // we'll get here when getline reaches the end of file
        AddEOFAndPrecedingDedents();
        return NextToken();
    }

    curr_token_ = move(parsed_tokens_.front());
    parsed_tokens_.pop();

    return curr_token_;
}

void Lexer::ParseLine(string&& line) {
    using namespace std::literals;

    EraseCommentAndSpacesBefore(line);

    // empty line must not produce indent, dedent, newline
    if (line.empty()) {
        return;
    }

    ParseIndentDedent(line);

#define PARSE_TOKEN(type, re, has_value)                               \
    {                                                                  \
        smatch sm;                                                     \
        regex_search(line, sm, re);                                    \
                                                                       \
        if (sm.size()) {                                               \
            /* for valued tokens */                                    \
            optional<string> value_opt;                                \
            if (has_value) {                                           \
                string value = move(sm[1].str());                      \
                                                                       \
                /* for strings */                                      \
                if (is_same_v<type, token_type::String>) {             \
                    EraseEscapeSequences(value);                       \
                }                                                      \
                                                                       \
                value_opt = move(value);                               \
            }                                                          \
                                                                       \
            parsed_tokens_.push(GetConstructedToken<type>(value_opt)); \
                                                                       \
            line = move(sm.suffix().str());                            \
                                                                       \
            continue;                                                  \
        }                                                              \
    }

    // order is important: do not misparse keywords as identifiers or chars
    while (!line.empty()) {
        // keywords' start (they have not value -> "false")
        PARSE_TOKEN(token_type::Class, class_re_, false);
        PARSE_TOKEN(token_type::Return, return_re_, false);
        PARSE_TOKEN(token_type::If, if_re_, false);
        PARSE_TOKEN(token_type::Else, else_re_, false);
        PARSE_TOKEN(token_type::Def, def_re_, false);
        PARSE_TOKEN(token_type::Print, print_re_, false);
        PARSE_TOKEN(token_type::And, and_re_, false);
        PARSE_TOKEN(token_type::Or, or_re_, false);
        PARSE_TOKEN(token_type::Not, not_re_, false);
        PARSE_TOKEN(token_type::Eq, eq_re_, false);
        PARSE_TOKEN(token_type::NotEq, not_eq_re_, false);
        PARSE_TOKEN(token_type::LessOrEq, less_or_eq_re_, false);
        PARSE_TOKEN(token_type::GreaterOrEq, greater_or_eq_re_, false);
        PARSE_TOKEN(token_type::None, none_re_, false);
        PARSE_TOKEN(token_type::True, true_re_, false);
        PARSE_TOKEN(token_type::False, false_re_, false);
        // keywords' end

        // (they have value -> "true")
        PARSE_TOKEN(token_type::Number, number_re_, true);
        PARSE_TOKEN(token_type::Id, id_re_, true);
        PARSE_TOKEN(token_type::Char, char_re_, true);
        PARSE_TOKEN(token_type::String, string_single_quote_re_, true);
        PARSE_TOKEN(token_type::String, string_double_quote_re_, true);

        throw logic_error("Unknown token to parse"s);
    }
#undef PARSE_STRING_TOKEN
#undef PARSE_TOKEN

    parsed_tokens_.push(Token(token_type::Newline{}));
}

void Lexer::EraseCommentAndSpacesBefore(string& line) {
    // erase comment
    {
        smatch sm;
        regex_search(line, sm, non_comment_re_);

        if (sm.size()) {
            line = move(sm[0].str());
        }
    }

    // erase spaces that were before comment so that (when line became empty) not to parse them as indent or dedent
    {
        smatch sm;
        regex_search(line, sm, end_spaces_re_);

        if (sm.size()) {
            line = move(sm.prefix().str());
        }
    }
}

void Lexer::ParseIndentDedent(string& line) {
    smatch sm;
    regex_search(line, sm, start_spaces_re_);

    // according to start_spaces_re_, sm.size() is always not empty

    // we are supposed to take only an even number of spaces
    int new_indent_count = sm[0].str().size() / 2;

    if (indent_count_ == new_indent_count) {
        return;
    }

    bool should_indent = indent_count_ < new_indent_count;
    for (int i = should_indent ? indent_count_ : new_indent_count;
         i < (should_indent ? new_indent_count : indent_count_);
         ++i) {
        Token token = should_indent ? Token(token_type::Indent{}) : Token(token_type::Dedent{});
        parsed_tokens_.push(move(token));
    }
    indent_count_ = new_indent_count;

    line = move(sm.suffix().str());
}

void Lexer::AddEOFAndPrecedingDedents() {
    for (int i = indent_count_; i > 0; --i) {
        parsed_tokens_.push(Token(token_type::Dedent{}));
    }
    indent_count_ = 0;

    parsed_tokens_.push(Token(token_type::Eof{}));
}

void Lexer::EraseEscapeSequences(string& s) {
    using namespace std::literals;

    ReplaceOneEscapeSequence(s, "\\'"s, "'"s);
    ReplaceOneEscapeSequence(s, "\\\""s, "\""s);
    ReplaceOneEscapeSequence(s, "\\n"s, "\n"s);
    ReplaceOneEscapeSequence(s, "\\t"s, "\t"s);
}

void Lexer::ReplaceOneEscapeSequence(string& s, string&& old_s, string&& new_s) {
    while (s.find(old_s) != s.npos) {
        s.replace(s.find(old_s), old_s.size(), new_s);
    }
}

}  // namespace parse