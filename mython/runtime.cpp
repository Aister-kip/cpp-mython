#include "runtime.h"

#include <cassert>
#include <optional>
#include <sstream>

using namespace std;

namespace runtime {

    ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
        : data_(std::move(data)) {
    }

    void ObjectHolder::AssertIsValid() const {
        assert(data_ != nullptr);
    }

    ObjectHolder ObjectHolder::Share(Object& object) {
        // Возвращаем невладеющий shared_ptr (его deleter ничего не делает)
        return ObjectHolder(std::shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
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

    bool IsTrue(const ObjectHolder& object) {
        if (object.TryAs<Number>()) {
            return (object.TryAs<Number>()->GetValue() != 0);
        }
        if (object.TryAs<Bool>()) {
            return object.TryAs<Bool>()->GetValue();
        }
        if (object.TryAs<String>()) {
            return !object.TryAs<String>()->GetValue().empty();
        }

        return false;
    }

    void ClassInstance::Print(std::ostream& os, Context& context) {
        if (HasMethod("__str__"s, 0)) {
            auto res = Call("__str__"s, {}, context);
            res->Print(os, context);
        }
        else {
            os << this;
        }
    }

    bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
        auto temp_method = cls_.GetMethod(method);
        if (temp_method != nullptr) {
            return (temp_method->formal_params.size() == argument_count);
        }
        return false;
    }

    Closure& ClassInstance::Fields() {
        return fields_;
    }

    const Closure& ClassInstance::Fields() const {
        return fields_;
    }

    ClassInstance::ClassInstance(const Class& cls)
        : cls_(cls) {
    }

    ObjectHolder ClassInstance::Call(const std::string& method,
        const std::vector<ObjectHolder>& actual_args,
        Context& context) {
        if (!HasMethod(method, actual_args.size())) {
            throw std::runtime_error("Cannot call method"s);
        }
        const Method* temp_method = cls_.GetMethod(method);
        Closure closure;
        closure["self"s] = ObjectHolder::Share(*this);
        size_t args_counter = 0;
        for (const auto i : actual_args) {
            closure[temp_method->formal_params[args_counter]] = i;
            ++args_counter;
        }
        return temp_method->body->Execute(closure, context);
    }

    Class::Class(std::string name, std::vector<Method> methods, const Class* parent)
        : name_(name)
        , methods_(std::move(methods))
        , parent_(parent) {
    }

    const Method* Class::GetMethod(const std::string& name) const {
        for (const auto& i : methods_) {
            if (i.name == name) {
                return &i;
            }
        }
        if (parent_) {
            return parent_->GetMethod(name);
        }
        return nullptr;
    }

    const std::string& Class::GetName() const {
        return name_;
    }

    void Class::Print(ostream& os, [[maybe_unused]] Context& context) {
        os << "Class "s << name_;
    }

    void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
        os << (GetValue() ? "True"sv : "False"sv);
    }

    bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        if (!lhs && !rhs) {
            return true;
        }
        if (lhs.TryAs<Number>() && rhs.TryAs<Number>()) {
            return lhs.TryAs<Number>()->GetValue() == rhs.TryAs<Number>()->GetValue();
        }
        if (lhs.TryAs<String>() && rhs.TryAs<String>()) {
            return lhs.TryAs<String>()->GetValue() == rhs.TryAs<String>()->GetValue();
        }
        if (lhs.TryAs<Bool>() && rhs.TryAs<Bool>()) {
            return lhs.TryAs<Bool>()->GetValue() == rhs.TryAs<Bool>()->GetValue();
        }
        if (lhs.TryAs<ClassInstance>()) {
            if (lhs.TryAs<ClassInstance>()->HasMethod("__eq__"s, 1)) {
                return (bool)lhs.TryAs<ClassInstance>()->Call("__eq__"s, { rhs }, context).TryAs<Bool>()->GetValue();
            }
        }
        throw std::runtime_error("Cannot compare objects for equality"s);
    }

    bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        if (lhs.TryAs<Number>() && rhs.TryAs<Number>()) {
            return lhs.TryAs<Number>()->GetValue() < rhs.TryAs<Number>()->GetValue();
        }
        if (lhs.TryAs<String>() && rhs.TryAs<String>()) {
            return lhs.TryAs<String>()->GetValue() < rhs.TryAs<String>()->GetValue();
        }
        if (lhs.TryAs<Bool>() && rhs.TryAs<Bool>()) {
            return lhs.TryAs<Bool>()->GetValue() < rhs.TryAs<Bool>()->GetValue();
        }
        if (lhs.TryAs<ClassInstance>()) {
            if (lhs.TryAs<ClassInstance>()->HasMethod("__lt__"s, 1)) {
                return lhs.TryAs<ClassInstance>()->Call("__lt__"s, { rhs }, context).TryAs<Bool>()->GetValue();
            }
        }
        throw std::runtime_error("Cannot compare objects for less"s);
    }

    bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return !Equal(lhs, rhs, context);
    }

    bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return (!Less(lhs, rhs, context) && NotEqual(lhs, rhs, context));
    }

    bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return !Greater(lhs, rhs, context);
    }

    bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return !Less(lhs, rhs, context);
    }

}  // namespace runtime