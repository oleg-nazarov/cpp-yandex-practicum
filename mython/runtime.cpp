#include "runtime.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <optional>
#include <sstream>

using namespace std;

namespace runtime {

ObjectHolder ObjectHolder::Share(Object& object) {
    // Возвращаем невладеющий shared_ptr (его deleter ничего не делает)
    return ObjectHolder(shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
}

ObjectHolder ObjectHolder::None() {
    return ObjectHolder();
}

Object& ObjectHolder::operator*() const {
    AssertIsValid();
    return *Get();
}

Object* ObjectHolder::operator->() const {
    AssertIsValid();
    return Get();
}

Object* ObjectHolder::Get() const {
    return data_.get();
}

ObjectHolder::operator bool() const {
    return Get() != nullptr;
}

ObjectHolder::ObjectHolder(shared_ptr<Object> data)
    : data_(move(data)) {
}

void ObjectHolder::AssertIsValid() const {
    assert(data_ != nullptr);
}

bool IsTrue(const ObjectHolder& object) {
    // "None"
    if (!object) {
        return false;
    }

    if (const String* p = object.TryAs<String>(); p != nullptr) {
        return !p->GetValue().empty();
    } else if (const Number* p = object.TryAs<Number>(); p != nullptr) {
        return p->GetValue();
    } else if (const Bool* p = object.TryAs<Bool>(); p != nullptr) {
        return p->GetValue();
    }

    return false;
}

void Bool::Print(ostream& os, [[maybe_unused]] Context& context) {
    os << (GetValue() ? "True"sv : "False"sv);
}

// const std::string None::NONE_S = "None";

Class::Class(string name, vector<Method> methods, const Class* parent)
    : name_(move(name)), methods_(move(methods)), parent_(parent) {}

const Method* Class::GetMethod(const string& name) const {
    auto method_it = find_if(methods_.begin(), methods_.end(), [&name](const Method& m) {
        return m.name == name;
    });
    if (method_it != methods_.end()) {
        return &*method_it;
    }

    if (parent_ == nullptr) {
        return nullptr;
    }

    // looking for method in parent (recursion)
    return parent_->GetMethod(name);
}

[[nodiscard]] const string& Class::GetName() const {
    return name_;
}

void Class::Print(ostream& os, [[maybe_unused]] Context& context) {
    static const string CLASS_S = "Class";

    os << CLASS_S << ' ' << GetName();
}

ClassInstance::ClassInstance(const Class& cls) : cls_(cls) {}

void ClassInstance::Print(ostream& os, Context& context) {
    static const string __STR__S = "__str__";

    if (!HasMethod(__STR__S, 0)) {
        os << this;
        return;
    }

    ObjectHolder result = Call(__STR__S, {}, context);
    result.Get()->Print(os, context);
}

bool ClassInstance::HasMethod(const string& method, size_t argument_count) const {
    if (auto m = cls_.GetMethod(method); m != nullptr) {
        return m->formal_params.size() == argument_count;
    }

    return false;
}

ObjectHolder ClassInstance::Call(const string& method, const vector<ObjectHolder>& actual_args, Context& context) {
    if (!HasMethod(method, actual_args.size())) {
        throw runtime_error("No such method in class"s);
    }

    Closure closure;
    closure["self"s] = ObjectHolder::Share(*this);

    const Method* m = cls_.GetMethod(method);
    for (size_t i = 0; i < actual_args.size(); ++i) {
        string param_name = m->formal_params[i];
        ObjectHolder arg = actual_args[i];

        closure[move(param_name)] = move(arg);
    }

    Executable& exec = *(m->body);
    ObjectHolder result = exec.Execute(closure, context);

    return result;
}

Closure& ClassInstance::Fields() {
    return fields_;
}

const Closure& ClassInstance::Fields() const {
    return fields_;
}

bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    static const string __EQ__S = "__eq__";

    // ClassInstance
    if (auto lhs_p = lhs.TryAs<ClassInstance>(); lhs_p != nullptr) {
        if (lhs_p->HasMethod(__EQ__S, 1)) {
            Bool* result = lhs_p->Call(__EQ__S, {rhs}, context).TryAs<Bool>();
            return result->GetValue();
        }
    }

    // None
    if (!lhs && !rhs) {
        return true;
    }

    // String, Number, Bool
    if (auto lhs_p = lhs.TryAs<String>(); lhs_p != nullptr) {
        if (auto rhs_p = rhs.TryAs<String>(); rhs_p != nullptr) {
            return lhs_p->GetValue() == rhs_p->GetValue();
        }
    } else if (auto lhs_p = lhs.TryAs<Number>(); lhs_p != nullptr) {
        if (auto rhs_p = rhs.TryAs<Number>(); rhs_p != nullptr) {
            return lhs_p->GetValue() == rhs_p->GetValue();
        }
    } else if (auto lhs_p = lhs.TryAs<Bool>(); lhs_p != nullptr) {
        if (auto rhs_p = rhs.TryAs<Bool>(); rhs_p != nullptr) {
            return lhs_p->GetValue() == rhs_p->GetValue();
        }
    }

    throw runtime_error("Cannot compare objects for equality"s);
}

bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    static const string __LT__S = "__lt__";

    // ClassInstance
    if (auto lhs_p = lhs.TryAs<ClassInstance>(); lhs_p != nullptr) {
        if (lhs_p->HasMethod(__LT__S, 1)) {
            Bool* result = lhs_p->Call(__LT__S, {rhs}, context).TryAs<Bool>();
            return result->GetValue();
        }
    }

    // String, Number, Bool
    if (auto lhs_p = lhs.TryAs<String>(); lhs_p != nullptr) {
        if (auto rhs_p = rhs.TryAs<String>(); rhs_p != nullptr) {
            return lhs_p->GetValue() < rhs_p->GetValue();
        }
    } else if (auto lhs_p = lhs.TryAs<Number>(); lhs_p != nullptr) {
        if (auto rhs_p = rhs.TryAs<Number>(); rhs_p != nullptr) {
            return lhs_p->GetValue() < rhs_p->GetValue();
        }
    } else if (auto lhs_p = lhs.TryAs<Bool>(); lhs_p != nullptr) {
        if (auto rhs_p = rhs.TryAs<Bool>(); rhs_p != nullptr) {
            return lhs_p->GetValue() < rhs_p->GetValue();
        }
    }

    throw runtime_error("Cannot compare objects for less"s);
}

bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Equal(lhs, rhs, context);
}

bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Less(lhs, rhs, context) && !Equal(lhs, rhs, context);
}

bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return Less(lhs, rhs, context) || Equal(lhs, rhs, context);
}

bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return !Less(lhs, rhs, context);
}

}  // namespace runtime