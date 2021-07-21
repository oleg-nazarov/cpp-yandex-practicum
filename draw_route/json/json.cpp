#include "json.h"

using namespace std;

namespace json {

Node LoadNode(istream& input);

Node LoadNull(istream& input) {
    const static string NULL_S = "null";

    // we start from 1 because we'd extracted first char before we called this function
    for (size_t i = 1; i < NULL_S.size(); ++i) {
        if (input.peek() == EOF || input.get() != NULL_S[i]) {
            throw ParsingError("A null is expected");
        }
    }

    return Node{};
}

void CheckBoolParsingOrThrow(istream& input, const string& bool_s) {
    for (size_t i = 0; i < bool_s.size(); ++i) {
        if (input.peek() == EOF || input.get() != bool_s[i]) {
            throw ParsingError("A bool is expected");
        }
    }
}

Node LoadBool(istream& input) {
    const static string TRUE_S = "true";
    const static string FALSE_S = "false";

    bool bool_value = input.peek() == TRUE_S.front();
    CheckBoolParsingOrThrow(input, bool_value ? TRUE_S : FALSE_S);

    return Node{bool_value};
}

Node LoadArray(istream& input) {
    Array result;

    char c;
    for (; input >> c && c != ']';) {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }

    if (c != ']') {
        throw ParsingError("An array is expected"s);
    }

    return Node(move(result));
}

Node LoadNumber(istream& input) {
    using namespace literals;

    string parsed_num;

    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());

        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    auto read_digits = [&input, read_char] {
        if (!isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }

        while (isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }

    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    } else {
        read_digits();
    }

    bool is_int = true;

    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();

        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }

        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                return Node(stoi(parsed_num));
            } catch (...) {
                // В случае неудачи, например, при переполнении
                // код ниже попробует преобразовать строку в double
            }
        }
        return Node(stod(parsed_num));
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

Node LoadString(istream& input) {
    string line;

    char c;
    input >> noskipws;
    for (; input >> c && c != '"';) {
        if (c == '\\') {
            c = input.get();

            switch (c) {
                case 'n':
                    line += '\n';
                    break;
                case 'r':
                    line += '\r';
                    break;
                case '"':
                    line += '"';
                    break;
                case 't':
                    line += '\t';
                    break;
                case '\\':
                    line += '\\';
                    break;
                default:
                    break;
            }

            continue;
        }

        line += c;
    }
    input >> skipws;

    if (c != '"') {
        throw ParsingError("A string is expected");
    }

    return Node(move(line));
}

Node LoadDict(istream& input) {
    Dict result;

    char c;
    for (; input >> c && c != '}';) {
        if (c == ',') {
            input >> c;
        }

        string key = LoadString(input).AsString();
        input >> c;
        result.insert({move(key), LoadNode(input)});
    }

    if (c != '}') {
        throw ParsingError("A dict is expected");
    }

    return Node(move(result));
}

Node LoadNode(istream& input) {
    char c;
    input >> c;

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (c == 'n') {
        return LoadNull(input);
    } else if (c == 't' || c == 'f') {
        input.putback(c);
        return LoadBool(input);
    } else if (c == '-' || isdigit(c)) {
        input.putback(c);
        return LoadNumber(input);
    } else {
        throw ParsingError("Failed to parse");
    }
}

Node::Node() = default;
Node::Node(nullptr_t) : value_(nullptr) {}
Node::Node(Array array) : value_(move(array)) {}
Node::Node(Dict dict) : value_(move(dict)) {}
Node::Node(bool value) : value_(move(value)) {}
Node::Node(int value) : value_(move(value)) {}
Node::Node(double value) : value_(move(value)) {}
Node::Node(string value) : value_(move(value)) {}

const Array& Node::AsArray() const {
    try {
        return get<Array>(value_);
    } catch (bad_variant_access& e) {
        throw logic_error("Wrong type");
    }
}
bool Node::AsBool() const {
    try {
        return get<bool>(value_);
    } catch (bad_variant_access& e) {
        throw logic_error("Wrong type");
    }
}
double Node::AsDouble() const {
    try {
        return IsPureDouble() ? get<double>(value_) : static_cast<double>(get<int>(value_));
    } catch (bad_variant_access& e) {
        throw logic_error("Wrong type");
    }
}
int Node::AsInt() const {
    try {
        return get<int>(value_);
    } catch (bad_variant_access& e) {
        throw logic_error("Wrong type");
    }
}
const Dict& Node::AsMap() const {
    try {
        return get<Dict>(value_);
    } catch (bad_variant_access& e) {
        throw logic_error("Wrong type");
    }
}
const string& Node::AsString() const {
    try {
        return get<string>(value_);
    } catch (bad_variant_access& e) {
        throw logic_error("Wrong type");
    }
}

bool Node::IsArray() const {
    return holds_alternative<Array>(value_);
}
bool Node::IsBool() const {
    return holds_alternative<bool>(value_);
}
bool Node::IsDouble() const {
    return holds_alternative<double>(value_) || holds_alternative<int>(value_);
}
bool Node::IsInt() const {
    return holds_alternative<int>(value_);
}
bool Node::IsMap() const {
    return holds_alternative<Dict>(value_);
}
bool Node::IsNull() const {
    return holds_alternative<nullptr_t>(value_);
}
bool Node::IsPureDouble() const {
    return holds_alternative<double>(value_);
}
bool Node::IsString() const {
    return holds_alternative<string>(value_);
}

bool Node::operator==(const Node& other) const {
    return value_ == other.value_;
}
bool Node::operator!=(const Node& other) const {
    return !(*this == other);
}

Document::Document(Node root) : root_(move(root)) {}

const Node& Document::GetRoot() const {
    return root_;
}

bool Document::operator==(const Document& other) const {
    return this->GetRoot() == other.GetRoot();
}
bool Document::operator!=(const Document& other) const {
    return !(*this == other);
}

Document Load(istream& input) {
    return Document{LoadNode(input)};
}

string EscapeSequences(const string& s) {
    string result;

    for (size_t i = 0; i < s.size(); ++i) {
        switch (s[i]) {
            case '\n':
                result += "\\n"s;
                break;
            case '\r':
                result += "\\r"s;
                break;
            case '"':
                result += "\\\""s;
                break;
            case '\t':
                result += "\\t"s;
                break;
            case '\\':
                result += "\\"s;
                break;
            default:
                result += s[i];
                break;
        }
    }

    return result;
}

void PrintNode(const Node& node, ostream& output) {
    using namespace literals;

    if (node.IsArray()) {
        output << "["s;
        for (size_t i = 0; i < node.AsArray().size(); ++i) {
            PrintNode(node.AsArray()[i], output);

            if (i + 1 < node.AsArray().size()) {
                output << ",";
            }
        }
        output << "]"s;
    } else if (node.IsBool()) {
        output << boolalpha << node.AsBool() << noboolalpha;
    } else if (node.IsPureDouble()) {
        output << node.AsDouble();
    } else if (node.IsInt()) {
        output << node.AsInt();
    } else if (node.IsMap()) {
        output << "{";
        for (auto it = node.AsMap().begin(); it != node.AsMap().end(); /* "++it" is inside */) {
            const auto& [key_s, value_node] = *it;

            PrintNode(Node(key_s), output);
            output << ":";
            PrintNode(value_node, output);

            ++it;
            if (it != node.AsMap().end()) {
                output << ",";
            }
        }
        output << "}";
    } else if (node.IsNull()) {
        output << "null"s;
    } else if (node.IsString()) {
        output << "\"" << EscapeSequences(node.AsString()) << "\"";
    }
}

void Print(const Document& doc, ostream& output) {
    const Node& node = doc.GetRoot();

    PrintNode(node, output);
}

}  // namespace json
