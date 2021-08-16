#include "json_builder.h"

#include <cassert>
#include <iterator>

namespace json {
namespace detail {

Context::Context(Builder& builder) : builder_(builder) {}

// ---------- ValueContext ----------

ValueContext::ValueContext(Builder& builder) : Context(builder) {}

ArrayContext ValueContext::StartArray() {
    return builder_.StartArray();
}
DictContext ValueContext::StartDict() {
    return builder_.StartDict();
}

ValueOfArrayContext::ValueOfArrayContext(Builder& builder) : ValueContext(builder) {}

ArrayContext ValueOfArrayContext::Value(Node node) {
    return builder_.ArrayValue(std::move(node));
}

ValueOfDictContext::ValueOfDictContext(Builder& builder) : ValueContext(builder) {}

DictContext ValueOfDictContext::Value(Node node) {
    return builder_.DictValue(std::move(node));
}

// ---------- ArrayContext ----------

ArrayContext::ArrayContext(Builder& builder) : ValueOfArrayContext(builder) {}

Builder& ArrayContext::EndArray() {
    return builder_.EndArray();
}

// ---------- DictContext ----------

DictContext::DictContext(Builder& builder) : Context(builder) {}

Builder& DictContext::EndDict() {
    return builder_.EndDict();
}
ValueOfDictContext DictContext::Key(std::string key) {
    return builder_.Key(std::move(key));
}

}  // namespace detail

detail::ArrayContext Builder::StartArray() {
    CheckCallCorrectnessOrThrow(detail::BuilderMethodType::START_ARRAY);

    nodes_stack_.push(Node(Array{}));

    return detail::ArrayContext(*this);
}
Builder& Builder::EndArray() {
    CheckCallCorrectnessOrThrow(detail::BuilderMethodType::END_ARRAY);

    Node ready_node = nodes_stack_.top();
    nodes_stack_.pop();

    ForwardNode(std::move(ready_node));

    return *this;
}

detail::DictContext Builder::StartDict() {
    CheckCallCorrectnessOrThrow(detail::BuilderMethodType::START_DICT);

    nodes_stack_.push(Node(Dict{}));

    return detail::DictContext(*this);
}
Builder& Builder::EndDict() {
    CheckCallCorrectnessOrThrow(detail::BuilderMethodType::END_DICT);

    Node ready_node = nodes_stack_.top();
    nodes_stack_.pop();

    ForwardNode(std::move(ready_node));

    return *this;
}

detail::ValueOfDictContext Builder::Key(std::string key) {
    CheckCallCorrectnessOrThrow(detail::BuilderMethodType::KEY);

    nodes_stack_.push(Node(std::move(key)));

    return detail::ValueOfDictContext(*this);
}

Builder& Builder::Value(Node value) {
    CheckCallCorrectnessOrThrow(detail::BuilderMethodType::VALUE);

    if (nodes_stack_.empty()) {
        ForwardNode(std::move(value));
    } else if (nodes_stack_.top().IsString()) {
        DictValue(std::move(value));
    } else if (nodes_stack_.top().IsArray()) {
        ArrayValue(std::move(value));
    }

    return *this;
}

Node Builder::Build() const {
    CheckCallCorrectnessOrThrow(detail::BuilderMethodType::BUILD);

    return root_;
}

void Builder::ForwardNode(Node&& node) {
    if (!root_.IsNull()) {
        throw std::logic_error("JSON is already builded");
    }

    if (nodes_stack_.empty()) {
        root_ = std::move(node);
    } else if (nodes_stack_.top().IsString() || nodes_stack_.top().IsArray()) {
        Value(std::move(node));
    } else {
        throw std::logic_error("Something went wrong");
    }
}

void Builder::CheckCallCorrectnessOrThrow(const detail::BuilderMethodType& field_type) const {
    using namespace detail;

    if (field_type == BuilderMethodType::START_ARRAY ||
        field_type == BuilderMethodType::START_DICT ||
        field_type == BuilderMethodType::VALUE) {
        if (!nodes_stack_.empty() &&
            !nodes_stack_.top().IsString() &&
            !nodes_stack_.top().IsArray()) {
            throw std::logic_error("wrong attempt to add array/dict/value");
        }
    } else if (field_type == BuilderMethodType::END_ARRAY) {
        if (nodes_stack_.empty() || !nodes_stack_.top().IsArray()) {
            throw std::logic_error("array is not ready");
        }
    } else if (field_type == BuilderMethodType::END_DICT) {
        if (nodes_stack_.empty() || !nodes_stack_.top().IsDict()) {
            throw std::logic_error("dict is not ready");
        }
    } else if (field_type == BuilderMethodType::KEY) {
        if (nodes_stack_.empty() || !nodes_stack_.top().IsDict()) {
            throw std::logic_error("this key must be only the \"key\" for map");
        }
    } else if (field_type == BuilderMethodType::BUILD) {
        if (root_.IsNull()) {
            throw std::logic_error("could not build empty JSON");
        }

        if (!nodes_stack_.empty()) {
            throw std::logic_error("complete unfilled JSON node");
        }
    }
}

detail::ArrayContext Builder::ArrayValue(Node value) {
    Array& arr = nodes_stack_.top().AsArray();
    arr.push_back(std::move(value));

    detail::ArrayContext ctx(*this);
    return ctx;
}

detail::DictContext Builder::DictValue(Node value) {
    Node key_node = std::move(nodes_stack_.top());
    nodes_stack_.pop();

    Dict& dict = nodes_stack_.top().AsDict();
    dict[std::move(key_node.AsString())] = std::move(value);

    detail::DictContext ctx(*this);
    return ctx;
}

}  // namespace json
