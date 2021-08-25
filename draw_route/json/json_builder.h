#pragma once

#include <stack>
#include <string>

#include "json.h"

namespace json {

class Builder;

namespace detail {

enum class BuilderMethodType {
    BUILD,
    END_ARRAY,
    END_DICT,
    START_ARRAY,
    START_DICT,
    KEY,
    VALUE,
};

}  // namespace detail

class Builder {
   private:
    class Context {
       public:
        Context(Builder& builder);

       protected:
        ~Context() = default;

        Builder& builder_;
    };

    class ArrayContext;
    class DictContext;

    class ValueContext : public Context {
       public:
        ValueContext(Builder& builder);

        ArrayContext StartArray();
        DictContext StartDict();
    };

    class ValueOfArrayContext : public ValueContext {
       public:
        ValueOfArrayContext(Builder& builder);

        ArrayContext Value(Node node);
    };

    class ValueOfDictContext : public ValueContext {
       public:
        ValueOfDictContext(Builder& builder);

        DictContext Value(Node node);
    };

    class ArrayContext : public ValueOfArrayContext {
       public:
        ArrayContext(Builder& builder);

        Builder& EndArray();
    };

    class DictContext : public Context {
       public:
        DictContext(Builder& builder);

        Builder& EndDict();

        ValueOfDictContext Key(std::string key);
    };

   public:
    ArrayContext StartArray();
    Builder& EndArray();

    DictContext StartDict();
    Builder& EndDict();

    ValueOfDictContext Key(std::string key);

    Builder& Value(Node value);

    Node Build() const;

   private:
    /* 1. using this stack to track yet not fulfilled structures;
    ** 2. if a Node:
    **    - IsString() - got a key, awaiting for value now (the single case for being a string in the stack)
    **    - IsArray() - filling vector now
    **    - IsDict() - filling map now */
    std::stack<Node> nodes_stack_;
    Node root_;

    void ForwardNode(Node&& node);

    void CheckCallCorrectnessOrThrow(const detail::BuilderMethodType& field_type) const;

    ArrayContext ArrayValue(Node value);
    DictContext DictValue(Node value);
};

}  // namespace json
