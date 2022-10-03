#include "statement.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace ast {

    using runtime::Closure;
    using runtime::Context;
    using runtime::ObjectHolder;

    namespace {
        const string ADD_METHOD = "__add__"s;
        const string INIT_METHOD = "__init__"s;
    }  // namespace

    ObjectHolder Assignment::Execute(Closure& closure, Context& context) {
        ObjectHolder obj = rv_->Execute(closure, context);
        closure[var_] = std::move(obj);
        return closure.at(var_);
    }

    Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv)
        : var_(var)
        , rv_(std::move(rv)) {
    }

    VariableValue::VariableValue(const std::string& var_name) {
        ids_.push_back(var_name);
    }

    VariableValue::VariableValue(std::vector<std::string> dotted_ids) 
        : ids_(std::move(dotted_ids)) {
    }

    ObjectHolder VariableValue::Execute(Closure& closure, [[maybe_unused]] Context& context) {
        Closure* curr_closure = &closure;
        std::string last_id;
        for (auto it = ids_.begin(); it != ids_.end(); ++it) {
            auto object = curr_closure->find(*it);
            if (object != curr_closure->end()) {
                last_id = object->first;
                if (object->second.TryAs<runtime::ClassInstance>()) {
                    if (std::next(it) != ids_.end()) {
                        curr_closure = &object->second.TryAs<runtime::ClassInstance>()->Fields();
                    }
                }
            }
        }
        if (curr_closure->find(last_id) != curr_closure->end()) {
            return curr_closure->at(last_id);
        }
        throw std::runtime_error("Wrong variable!"s);
    }

    unique_ptr<Print> Print::Variable(const std::string& name) {
        auto result = std::make_unique<Print>(std::make_unique<VariableValue>(VariableValue(name)));
        return result;
   }

    Print::Print(unique_ptr<Statement> argument) {
        args_.push_back(std::move(argument));
    }

    Print::Print(vector<unique_ptr<Statement>> args) 
        : args_(std::move(args)) {
    }

    ObjectHolder Print::Execute(Closure& closure, Context& context) {
        std::ostream& os = context.GetOutputStream();
        bool first_iter = true;
        for (auto& ptr : args_) {
            auto obj = ptr->Execute(closure, context);
            if (!first_iter) {
                os << ' ';
            }
            first_iter = false;
            if (obj.Get()) {
                obj->Print(os, context);
            }
            else {
                os << "None"s;
            }
        }
        os << '\n';
        return {};
    }

    MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method,
        std::vector<std::unique_ptr<Statement>> args) 
        : object_(std::move(object))
        , method_(method) 
        , args_(std::move(args)) {
    }

    ObjectHolder MethodCall::Execute(Closure& closure, Context& context) {
        ObjectHolder obj = object_->Execute(closure, context);
        if (obj.TryAs<runtime::ClassInstance>()->HasMethod(method_, args_.size())) {
            std::vector<ObjectHolder> fields;
            for (const auto& arg : args_) {
                fields.push_back(arg->Execute(closure, context));
            }
            return obj.TryAs<runtime::ClassInstance>()->Call(method_, fields, context);
        }
        return ObjectHolder::None();
    }

    ObjectHolder Stringify::Execute(Closure& closure, Context& context) {
        if (arg_.get()) {
            ObjectHolder obj = arg_.get()->Execute(closure, context);
            if (obj.Get()) {
                std::ostringstream os;
                obj.Get()->Print(os, context);
                return ObjectHolder::Own(runtime::String(os.str()));
            }
        }
        return ObjectHolder::Own(runtime::String("None"s));
    }

    ObjectHolder Add::Execute(Closure& closure, Context& context) {
        ObjectHolder lhs = lhs_->Execute(closure, context);
        ObjectHolder rhs = rhs_->Execute(closure, context);
        if (lhs.TryAs<runtime::Number>() && rhs.TryAs<runtime::Number>()) {
            return ObjectHolder::Own<runtime::Number>(lhs.TryAs<runtime::Number>()->GetValue() 
                + rhs.TryAs<runtime::Number>()->GetValue());
        }
        if (lhs.TryAs<runtime::String>() && rhs.TryAs<runtime::String>()) {
            return ObjectHolder::Own<runtime::String>(lhs.TryAs<runtime::String>()->GetValue()
                + rhs.TryAs<runtime::String>()->GetValue());
        }
        if (lhs.TryAs<runtime::ClassInstance>()) {
            if (lhs.TryAs<runtime::ClassInstance>()->HasMethod(ADD_METHOD, 1)) {
                return lhs.TryAs<runtime::ClassInstance>()->Call(ADD_METHOD, { rhs }, context);
            }
        }
        throw std::runtime_error("Addition error"s);
    }

    ObjectHolder Sub::Execute(Closure& closure, Context& context) {
        ObjectHolder lhs = lhs_->Execute(closure, context);
        ObjectHolder rhs = rhs_->Execute(closure, context);
        if (lhs.TryAs<runtime::Number>() && rhs.TryAs<runtime::Number>()) {
            return ObjectHolder::Own<runtime::Number>(lhs.TryAs<runtime::Number>()->GetValue()
                - rhs.TryAs<runtime::Number>()->GetValue());
        }
        throw std::runtime_error("Substraction error"s);
    }

    ObjectHolder Mult::Execute(Closure& closure, Context& context) {
        ObjectHolder lhs = lhs_->Execute(closure, context);
        ObjectHolder rhs = rhs_->Execute(closure, context);
        if (lhs.TryAs<runtime::Number>() && rhs.TryAs<runtime::Number>()) {
            return ObjectHolder::Own<runtime::Number>(lhs.TryAs<runtime::Number>()->GetValue()
                * rhs.TryAs<runtime::Number>()->GetValue());
        }
        throw std::runtime_error("Multiplication error"s);
    }

    ObjectHolder Div::Execute(Closure& closure, Context& context) {
        ObjectHolder lhs = lhs_->Execute(closure, context);
        ObjectHolder rhs = rhs_->Execute(closure, context);
        if (lhs.TryAs<runtime::Number>() && rhs.TryAs<runtime::Number>()) {
            if (rhs.TryAs<runtime::Number>()->GetValue() != 0) {
                return ObjectHolder::Own<runtime::Number>(lhs.TryAs<runtime::Number>()->GetValue()
                    / rhs.TryAs<runtime::Number>()->GetValue());
            }
            else {
                throw std::runtime_error("Error. Division by zero"s);
            }
        }
        throw std::runtime_error("Division error"s);
    }

    ObjectHolder Compound::Execute(Closure& closure, Context& context) {
        for (const auto& arg : args_) {
            arg->Execute(closure, context);
        }
        return ObjectHolder::None();
    }

    ObjectHolder Return::Execute(Closure& closure, Context& context) {
        throw(statement_->Execute(closure, context));
    }

    ClassDefinition::ClassDefinition(ObjectHolder cls) 
        : cls_(cls) {
    }

    ObjectHolder ClassDefinition::Execute(Closure& closure, Context& /*context*/) {
        closure[cls_.TryAs<runtime::Class>()->GetName()] = cls_;
        return {};
    }

    FieldAssignment::FieldAssignment(VariableValue object, std::string field_name,
        std::unique_ptr<Statement> rv) 
        : object_(object)
        , field_name_(field_name)
        , rv_(std::move(rv)) {
    }

    ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context) {
        ObjectHolder obj = object_.Execute(closure, context);
        ObjectHolder statement = rv_->Execute(closure, context);
        obj.TryAs<runtime::ClassInstance>()->Fields()[field_name_] = statement;
        return obj.TryAs<runtime::ClassInstance>()->Fields().at(field_name_);
    }

    IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
        std::unique_ptr<Statement> else_body) 
        : condition_(std::move(condition))
        , if_body_(std::move(if_body))
        , else_body_(std::move(else_body)) {
    }

    ObjectHolder IfElse::Execute(Closure& closure, Context& context) {
        if (runtime::IsTrue(condition_->Execute(closure, context))) {
            try {
                if_body_->Execute(closure, context);
            }
            catch (ObjectHolder obj) {
                throw obj;
            }
        }
        else {
            if (else_body_ != nullptr) {
                try {
                    else_body_->Execute(closure, context);
                }
                catch (ObjectHolder obj) {
                    throw obj;
                }
            }
        }
        return ObjectHolder::None();
    }

    ObjectHolder Or::Execute(Closure& closure, Context& context) {
        ObjectHolder lhs = lhs_->Execute(closure, context);
        if (runtime::IsTrue(lhs)) {
            return ObjectHolder::Own<runtime::Bool>(true);
        }
        else {
            ObjectHolder rhs = rhs_->Execute(closure, context);
            if (runtime::IsTrue(rhs)) {
                return ObjectHolder::Own<runtime::Bool>(true);
            }
        }
        return ObjectHolder::Own<runtime::Bool>(false);
    }

    ObjectHolder And::Execute(Closure& closure, Context& context) {
        ObjectHolder lhs = lhs_->Execute(closure, context);
        if (runtime::IsTrue(lhs)) {
            ObjectHolder rhs = rhs_->Execute(closure, context);
            if (runtime::IsTrue(rhs)) {
                return ObjectHolder::Own<runtime::Bool>(true);
            }
        }
        return ObjectHolder::Own<runtime::Bool>(false);
    }

    ObjectHolder Not::Execute(Closure& closure, Context& context) {
        ObjectHolder obj = arg_->Execute(closure, context);
        return ObjectHolder::Own<runtime::Bool>(!runtime::IsTrue(obj));
    }

    Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
        : BinaryOperation(std::move(lhs), std::move(rhs))
        , cmp_(cmp){
    }

    ObjectHolder Comparison::Execute(Closure& closure, Context& context) {
        ObjectHolder lhs = lhs_->Execute(closure, context);
        ObjectHolder rhs = rhs_->Execute(closure, context);
        auto result = cmp_(lhs, rhs, context);
        return ObjectHolder::Own<runtime::Bool>(result);
    }

    NewInstance::NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args) 
        : cls_inst_(runtime::ClassInstance(class_))
        , args_(std::move(args)) {
    }

    NewInstance::NewInstance(const runtime::Class& class_) 
        : cls_inst_(runtime::ClassInstance(class_)) {
    }

    ObjectHolder NewInstance::Execute(Closure& closure, Context& context) {
        if (cls_inst_.HasMethod(INIT_METHOD, args_.size())) {
            std::vector<ObjectHolder> fields;
            for (const auto& arg : args_) {
                fields.push_back(arg->Execute(closure, context));
            }
            cls_inst_.Call(INIT_METHOD, fields, context);
        }
        return ObjectHolder::Share(cls_inst_);
    }

    MethodBody::MethodBody(std::unique_ptr<Statement>&& body) 
        : body_(std::move(body)) {
    }

    ObjectHolder MethodBody::Execute(Closure& closure, Context& context) {
        try {
            body_->Execute(closure, context);
        }
        catch (ObjectHolder obj) {
            return obj;
        }
        return ObjectHolder::None();
    }

}  // namespace ast