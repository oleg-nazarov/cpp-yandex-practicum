#pragma once

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace json {

class Node;
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

class ParsingError : public std::runtime_error {
   public:
    using runtime_error::runtime_error;
};

class Node {
   public:
    Node();
    Node(std::nullptr_t);
    Node(Array array);
    Node(Dict dict);
    Node(bool value);
    Node(int value);
    Node(double value);
    Node(std::string value);

    const Array& AsArray() const;
    bool AsBool() const;
    double AsDouble() const;
    int AsInt() const;
    const Dict& AsMap() const;
    const std::string& AsString() const;

    bool IsArray() const;
    bool IsBool() const;
    bool IsDouble() const;
    bool IsInt() const;
    bool IsMap() const;
    bool IsNull() const;
    bool IsPureDouble() const;
    bool IsString() const;

    bool operator==(const Node& other) const;
    bool operator!=(const Node& other) const;

   private:
    std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string> value_;
};

class Document {
   public:
    explicit Document(Node root);

    const Node& GetRoot() const;

    bool operator==(const Document& other) const;
    bool operator!=(const Document& other) const;

   private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

}  // namespace json
