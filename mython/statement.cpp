#include "statement.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>

using namespace std;

namespace ast {

using runtime::Closure;
using runtime::Context;
using runtime::ObjectHolder;

namespace {
const string ADD_METHOD = "__add__"s;
const string INIT_METHOD = "__init__"s;

const string NONE_S = "None";
}  // namespace

VariableValue::VariableValue(const std::string& var_name) : ids_({var_name}) {}

VariableValue::VariableValue(std::vector<std::string> dotted_ids) : ids_(move(dotted_ids)) {}

ObjectHolder VariableValue::Execute(Closure& closure, Context& /*context*/) {
    ObjectHolder obj;

    for (size_t i = 0; i < ids_.size(); ++i) {
        const string& var_name = ids_[i];

        if (i == 0) {
            if (closure.count(var_name) == 0u) {
                throw runtime_error("There is no such name in closure"s + var_name);
            }
            obj = closure.at(var_name);
            continue;
        }

        Closure& inst_closure = obj.TryAs<runtime::ClassInstance>()->Fields();
        if (inst_closure.count(var_name) == 0u) {
            throw runtime_error("There is no such name in closure: "s + var_name);
        }

        obj = inst_closure.at(var_name);
    }

    return obj;
}

Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv)
    : var_(move(var)),
      rv_(move(rv)) {}

ObjectHolder Assignment::Execute(Closure& closure, Context& context) {
    ObjectHolder res = rv_->Execute(closure, context);
    closure[var_] = res;

    return res;
}

FieldAssignment::FieldAssignment(VariableValue object, std::string field_name, std::unique_ptr<Statement> rv)
    : var_val_(move(object)),
      field_name_(move(field_name)),
      rv_(move(rv)) {}

ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context) {
    ObjectHolder obj_instance = var_val_.Execute(closure, context);
    runtime::ClassInstance* instance_ptr = obj_instance.TryAs<runtime::ClassInstance>();

    Closure& inst_closure = instance_ptr->Fields();
    inst_closure[field_name_] = rv_->Execute(closure, context);

    return inst_closure[field_name_];
}

Print::Print(unique_ptr<Statement> argument) {
    args_.push_back(move(argument));
}

Print::Print(vector<unique_ptr<Statement>> args)
    : args_{make_move_iterator(args.begin()), make_move_iterator(args.end())} {}

unique_ptr<Print> Print::Variable(const std::string& name) {
    return make_unique<Print>(make_unique<VariableValue>(name));
}

ObjectHolder Print::Execute(Closure& closure, Context& context) {
    ostream& os = context.GetOutputStream();

    for (size_t i = 0; i < args_.size(); ++i) {
        const unique_ptr<Statement>& arg = args_[i];
        ObjectHolder result = arg->Execute(closure, context);

        if (result) {
            result->Print(os, context);
        } else {
            // "None"
            runtime::String(NONE_S).Print(os, context);
        }

        // print " "
        if (i + 1 != args_.size()) {
            runtime::String(" "s).Print(os, context);
        }
    }

    // print "\n"
    runtime::String("\n"s).Print(os, context);

    return {};
}

MethodCall::MethodCall(unique_ptr<Statement> object, string method, vector<unique_ptr<Statement>> args)
    : object_(move(object)),
      method_(move(method)),
      args_{make_move_iterator(args.begin()), make_move_iterator(args.end())} {}

ObjectHolder MethodCall::Execute(Closure& closure, Context& context) {
    ObjectHolder obj = object_->Execute(closure, context);
    runtime::ClassInstance* cls_instance_ptr = obj.TryAs<runtime::ClassInstance>();

    vector<ObjectHolder> actual_args;
    for (const unique_ptr<Statement>& arg : args_) {
        actual_args.push_back(arg->Execute(closure, context));
    }

    return cls_instance_ptr->Call(method_, actual_args, context);
}

NewInstance::NewInstance(const runtime::Class& cls) : instance_(cls) {}

NewInstance::NewInstance(const runtime::Class& cls, vector<unique_ptr<Statement>> args)
    : instance_(cls),
      init_args_{make_move_iterator(args.begin()), make_move_iterator(args.end())} {}

ObjectHolder NewInstance::Execute(Closure& closure, Context& context) {
    if (!instance_.HasMethod(INIT_METHOD, init_args_.size())) {
        return ObjectHolder::Share(instance_);
    }

    // prepare args for "__init__()"
    vector<ObjectHolder> args;
    transform(
        init_args_.begin(), init_args_.end(),
        back_inserter(args),
        [&closure, &context](const unique_ptr<Statement>& arg_ptr) {
            return arg_ptr->Execute(closure, context);
        });

    instance_.Call(INIT_METHOD, args, context);

    return ObjectHolder::Share(instance_);
}

ObjectHolder Stringify::Execute(Closure& closure, Context& context) {
    ObjectHolder obj = arg_->Execute(closure, context);
    ostringstream oss;

    if (!obj) {
        oss << NONE_S;
    } else {
        obj->Print(oss, context);
    }

    return ObjectHolder::Own(runtime::String(oss.str()));
}

ObjectHolder Add::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs = lhs_arg_->Execute(closure, context);
    ObjectHolder rhs = rhs_arg_->Execute(closure, context);

    if (auto lhs_inst = lhs.TryAs<runtime::ClassInstance>(); lhs_inst != nullptr) {
        return lhs_inst->Call(ADD_METHOD, {rhs}, context);
    } else if (auto lhs_num = lhs.TryAs<runtime::Number>(); lhs_num != nullptr) {
        if (auto rhs_num = rhs.TryAs<runtime::Number>(); rhs_num != nullptr) {
            int result = lhs_num->GetValue() + rhs_num->GetValue();
            return ObjectHolder::Own(runtime::Number(result));
        }
    } else if (auto lhs_str = lhs.TryAs<runtime::String>(); lhs_str != nullptr) {
        if (auto rhs_str = rhs.TryAs<runtime::String>(); rhs_str != nullptr) {
            string result = lhs_str->GetValue() + rhs_str->GetValue();
            return ObjectHolder::Own(runtime::String(result));
        }
    }

    throw runtime_error("Bad addition operands' type"s);
}

ObjectHolder Sub::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs = lhs_arg_->Execute(closure, context);
    ObjectHolder rhs = rhs_arg_->Execute(closure, context);

    if (auto lhs_num = lhs.TryAs<runtime::Number>(); lhs_num != nullptr) {
        if (auto rhs_num = rhs.TryAs<runtime::Number>(); rhs_num != nullptr) {
            int result = lhs_num->GetValue() - rhs_num->GetValue();
            return ObjectHolder::Own(runtime::Number(result));
        }
    }

    throw runtime_error("Bad subtraction operands' type"s);
}

ObjectHolder Mult::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs = lhs_arg_->Execute(closure, context);
    ObjectHolder rhs = rhs_arg_->Execute(closure, context);

    if (auto lhs_num = lhs.TryAs<runtime::Number>(); lhs_num != nullptr) {
        if (auto rhs_num = rhs.TryAs<runtime::Number>(); rhs_num != nullptr) {
            int result = lhs_num->GetValue() * rhs_num->GetValue();
            return ObjectHolder::Own(runtime::Number(result));
        }
    }

    throw runtime_error("Bad multiplication operands' type"s);
}

ObjectHolder Div::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs = lhs_arg_->Execute(closure, context);
    ObjectHolder rhs = rhs_arg_->Execute(closure, context);

    if (auto lhs_num = lhs.TryAs<runtime::Number>(); lhs_num != nullptr) {
        if (auto rhs_num = rhs.TryAs<runtime::Number>(); rhs_num != nullptr) {
            if (rhs_num->GetValue() == 0) {
                throw runtime_error("Division by zero"s);
            }

            int result = lhs_num->GetValue() / rhs_num->GetValue();
            return ObjectHolder::Own(runtime::Number(result));
        }
    }

    throw runtime_error("Bad division operands' type"s);
}

ObjectHolder Compound::Execute(Closure& closure, Context& context) {
    for (const unique_ptr<Statement>& stmt : statements_) {
        stmt->Execute(closure, context);
    }

    return ObjectHolder::None();
}

Return::Return(unique_ptr<Statement> statement) : statement_(move(statement)) {}

ObjectHolder Return::Execute(Closure& closure, Context& context) {
    throw statement_->Execute(closure, context);
}

MethodBody::MethodBody(std::unique_ptr<Statement>&& body) : body_(move(body)) {}

ObjectHolder MethodBody::Execute(Closure& closure, Context& context) {
    try {
        body_->Execute(closure, context);
    } catch (const ObjectHolder& return_obj) {
        return return_obj;
    }

    return ObjectHolder::None();
}

ClassDefinition::ClassDefinition(ObjectHolder cls) : cls_(move(cls)) {}

ObjectHolder ClassDefinition::Execute(Closure& closure, [[maybe_unused]] Context& context) {
    runtime::Class* cls_ptr = cls_.TryAs<runtime::Class>();

    closure[cls_ptr->GetName()] = cls_;

    return closure[cls_ptr->GetName()];
}

IfElse::IfElse(unique_ptr<Statement> condition, unique_ptr<Statement> if_body, unique_ptr<Statement> else_body)
    : condition_(move(condition)),
      if_body_(move(if_body)),
      else_body_(move(else_body)) {}

ObjectHolder IfElse::Execute(Closure& closure, Context& context) {
    if (runtime::IsTrue(condition_->Execute(closure, context))) {
        return if_body_->Execute(closure, context);
    }

    if (else_body_) {
        return else_body_->Execute(closure, context);
    }

    return {};
}

ObjectHolder Or::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs = lhs_arg_->Execute(closure, context);

    if (runtime::IsTrue(lhs)) {
        return ObjectHolder::Own(runtime::Bool{true});
    }

    ObjectHolder rhs = rhs_arg_->Execute(closure, context);

    return ObjectHolder::Own(runtime::Bool{runtime::IsTrue(rhs)});
}

ObjectHolder And::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs = lhs_arg_->Execute(closure, context);

    if (runtime::IsTrue(lhs)) {
        ObjectHolder rhs = rhs_arg_->Execute(closure, context);

        return ObjectHolder::Own(runtime::Bool{runtime::IsTrue(rhs)});
    }

    return ObjectHolder::Own(runtime::Bool{false});
}

ObjectHolder Not::Execute(Closure& closure, Context& context) {
    ObjectHolder obj = arg_->Execute(closure, context);
    bool bool_value = runtime::IsTrue(obj);

    return ObjectHolder::Own(runtime::Bool{!bool_value});
}

Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
    : BinaryOperation(std::move(lhs), std::move(rhs)),
      cmp_(move(cmp)) {}

ObjectHolder Comparison::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs = lhs_arg_->Execute(closure, context);
    ObjectHolder rhs = rhs_arg_->Execute(closure, context);

    bool bool_value = cmp_(lhs, rhs, context);

    return ObjectHolder::Own(runtime::Bool(bool_value));
}

}  // namespace ast
