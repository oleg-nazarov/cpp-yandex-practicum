#include "json_builder.h"

#include <cassert>
#include <iterator>

namespace json {

Builder::Context::Context(Builder& builder) : builder_(builder) {}

// ---------- ValueContext ----------

Builder::ValueContext::ValueContext(Builder& builder) : Context(builder) {}

Builder::ArrayContext Builder::ValueContext::StartArray() {
    return builder_.StartArray();
}
Builder::DictContext Builder::ValueContext::StartDict() {
    return builder_.StartDict();
}

Builder::ValueOfArrayContext::ValueOfArrayContext(Builder& builder) : ValueContext(builder) {}

Builder::ArrayContext Builder::ValueOfArrayContext::Value(Node node) {
    return builder_.ArrayValue(std::move(node));
}

Builder::ValueOfDictContext::ValueOfDictContext(Builder& builder) : ValueContext(builder) {}

Builder::DictContext Builder::ValueOfDictContext::Value(Node node) {
    return builder_.DictValue(std::move(node));
}

// ---------- ArrayContext ----------

Builder::ArrayContext::ArrayContext(Builder& builder) : ValueOfArrayContext(builder) {}

Builder& Builder::ArrayContext::EndArray() {
    return builder_.EndArray();
}

// ---------- DictContext ----------

Builder::DictContext::DictContext(Builder& builder) : Context(builder) {}

Builder& Builder::DictContext::EndDict() {
    return builder_.EndDict();
}
Builder::ValueOfDictContext Builder::DictContext::Key(std::string key) {
    return builder_.Key(std::move(key));
}

Builder::ArrayContext Builder::StartArray() {
    CheckCallCorrectnessOrThrow(detail::BuilderMethodType::START_ARRAY);

    nodes_stack_.push(Node(Array{}));

    return Builder::ArrayContext(*this);
}
Builder& Builder::EndArray() {
    CheckCallCorrectnessOrThrow(detail::BuilderMethodType::END_ARRAY);

    Node ready_node = nodes_stack_.top();
    nodes_stack_.pop();

    ForwardNode(std::move(ready_node));

    return *this;
}

Builder::DictContext Builder::StartDict() {
    CheckCallCorrectnessOrThrow(detail::BuilderMethodType::START_DICT);

    nodes_stack_.push(Node(Dict{}));

    return Builder::DictContext(*this);
}
Builder& Builder::EndDict() {
    CheckCallCorrectnessOrThrow(detail::BuilderMethodType::END_DICT);

    Node ready_node = nodes_stack_.top();
    nodes_stack_.pop();

    ForwardNode(std::move(ready_node));

    return *this;
}

Builder::ValueOfDictContext Builder::Key(std::string key) {
    CheckCallCorrectnessOrThrow(detail::BuilderMethodType::KEY);

    nodes_stack_.push(Node(std::move(key)));

    return Builder::ValueOfDictContext(*this);
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

    switch (field_type) {
        case BuilderMethodType::START_ARRAY:
        case BuilderMethodType::START_DICT:
        case BuilderMethodType::VALUE:
            if (!nodes_stack_.empty() &&
                !nodes_stack_.top().IsString() &&
                !nodes_stack_.top().IsArray()) {
                throw std::logic_error("wrong attempt to add array/dict/value");
            }
            break;

        case BuilderMethodType::END_ARRAY:
            if (nodes_stack_.empty() || !nodes_stack_.top().IsArray()) {
                throw std::logic_error("array is not ready");
            }
            break;

        case BuilderMethodType::END_DICT:
            if (nodes_stack_.empty() || !nodes_stack_.top().IsDict()) {
                throw std::logic_error("dict is not ready");
            }
            break;

        case BuilderMethodType::KEY:
            if (nodes_stack_.empty() || !nodes_stack_.top().IsDict()) {
                throw std::logic_error("this key must be only the \"key\" for map");
            }
            break;

        case BuilderMethodType::BUILD:
            if (root_.IsNull()) {
                throw std::logic_error("could not build empty JSON");
            }

            if (!nodes_stack_.empty()) {
                throw std::logic_error("complete unfilled JSON node");
            }
            break;

        default:
            break;
    }
}

Builder::ArrayContext Builder::ArrayValue(Node value) {
    Array& arr = nodes_stack_.top().AsArray();
    arr.push_back(std::move(value));

    Builder::ArrayContext ctx(*this);
    return ctx;
}

Builder::DictContext Builder::DictValue(Node value) {
    Node key_node = std::move(nodes_stack_.top());
    nodes_stack_.pop();

    Dict& dict = nodes_stack_.top().AsDict();
    dict[std::move(key_node.AsString())] = std::move(value);

    Builder::DictContext ctx(*this);
    return ctx;
}

}  // namespace json
