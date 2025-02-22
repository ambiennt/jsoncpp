// Copyright 2007-2010 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

#if !defined(JSONCPP_IS_AMALGAMATION)
#include <json/value.h>
#include <json/writer.h>
#ifndef JSONCPP_USE_SIMPLE_INTERNAL_ALLOCATOR
#include "json_batchallocator.h"
#endif // #ifndef JSONCPP_USE_SIMPLE_INTERNAL_ALLOCATOR
#endif // if !defined(JSONCPP_IS_AMALGAMATION)
#include <iostream>
#include <utility>
#include <stdexcept>
#include <cstring>
#include <cassert>
#include <cstddef> // size_t
#include <cmath>   // std::nextafter

namespace Json {

    /// Unknown size marker
    static constexpr auto unknown = std::numeric_limits<unsigned int>::max();

    /** Duplicates the specified string value.
     * @param value Pointer to the string to duplicate. Must be zero-terminated if
     *              length is "unknown".
     * @param length Length of the value. if equals to unknown, then it will be
     *               computed using strlen(value).
     * @return Pointer on the duplicate instance of string.
     */
    static inline char* duplicateStringValue(const char* value, unsigned int length = unknown) {
        if (length == unknown) {
            length = (unsigned int)strlen(value);
        }
        else if (length == 0) {
            return nullptr;
        }
        char* newString = static_cast<char*>(malloc(length + 1));
        JSONCPP_ASSERT_MESSAGE(newString != 0, "Failed to allocate string value buffer");
        [[likely]] if (newString) {
            memcpy(newString, value, length);
            newString[length] = 0;
        }
        return newString;
    }

    /** Free the string duplicated by duplicateStringValue().
     */
    static inline void releaseStringValue(char* value) {
        if (value)
            free(value);
    }

} // namespace Json

// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// ValueInternals...
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
#if !defined(JSONCPP_IS_AMALGAMATION)
#include "json_valueiterator.inl"
#endif // if !defined(JSONCPP_IS_AMALGAMATION)

namespace Json {

    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // class Value::CZString
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
        // Notes: index_ indicates if the string was allocated when
        // a string is stored.

    Value::CZString::CZString(ArrayIndex index) : cstr_(nullptr), index_(index) {}

    Value::CZString::CZString(const char* cstr, DuplicationPolicy allocate) :
        cstr_(allocate == duplicate ? duplicateStringValue(cstr) : cstr), index_(allocate) {}

    Value::CZString::CZString(const CZString& other) :
        cstr_(other.index_ != noDuplication && other.cstr_ ? duplicateStringValue(other.cstr_) : other.cstr_),
        index_(other.cstr_ ? (other.index_ == noDuplication ? noDuplication : duplicate) : other.index_) {}

    Value::CZString::CZString(std::string_view str) :
        cstr_(str.data() ? duplicateStringValue(str.data(), static_cast<unsigned int>(str.length())) : nullptr), index_(str.data() ? duplicate : noDuplication) {}

    Value::CZString::~CZString() {
        if (cstr_ && index_ == duplicate)
            releaseStringValue(const_cast<char*>(cstr_));
    }

    void Value::CZString::swap(CZString& other) {
        std::swap(cstr_, other.cstr_);
        std::swap(index_, other.index_);
    }

    Value::CZString& Value::CZString::operator=(const CZString& other) {
        CZString temp(other);
        swap(temp);
        return *this;
    }

    bool Value::CZString::operator<(const CZString& other) const {
        if (cstr_ == other.cstr_) {
            return false;
        }
        if (cstr_ && other.cstr_) {
            return strcmp(cstr_, other.cstr_) < 0;
        }
        return false;
    }

    bool Value::CZString::operator==(const CZString& other) const {
        if (!cstr_ || !other.cstr_) {
            return cstr_ == other.cstr_;
        }
        if (cstr_ == other.cstr_) {
            return true;
        }
        return strcmp(cstr_, other.cstr_) == 0;
    }

    ArrayIndex Value::CZString::index() const {
        return index_;
    }

    const char* Value::CZString::c_str() const {
        return cstr_;
    }

    size_t Value::CZString::length() const {
        return strlen(cstr_);
    }

    bool Value::CZString::isStaticString() const {
        return index_ == noDuplication;
    }

    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // class Value::Value
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////

    const Value Value::null;

    /*! \internal Default constructor initialization must be equivalent to:
     * memset( this, 0, sizeof(Value) )
     * This optimization is used in ValueInternalMap fast allocator.
     */
    Value::Value(ValueType type)
        : value_{}, type_{ type }, allocated_{ false }
    {
        switch (type) {
        case nullValue:
            break;
        case intValue:
        case uintValue:
            value_.int_ = 0;
            break;
        case realValue:
            value_.real_ = 0.0;
            break;
        case stringValue:
            value_.string_ = nullptr;
            break;
        case arrayValue:
        case objectValue:
            value_.map_ = new ObjectValues();
            break;
        case booleanValue:
            value_.bool_ = false;
            break;
        default:
            JSONCPP_ASSERT_UNREACHABLE;
        }
    }

#if defined(JSONCPP_HAS_INT64)
    Value::Value(UInt value) :
        value_{ .uint_ = value }, type_{ uintValue }, allocated_{ false } {}

    Value::Value(Int value) :
        value_{ .int_ = value }, type_{ intValue }, allocated_{ false } {}

#endif // if defined(JSONCPP_HAS_INT64)

    Value::Value(Int64 value) :
        value_{ .int_ = value }, type_{ intValue }, allocated_{ false } {}

    Value::Value(UInt64 value) :
        value_{ .uint_ = value }, type_{ uintValue }, allocated_{ false } {}

    Value::Value(double value) :
        value_{ .real_ = value }, type_{ realValue }, allocated_{ false } {}

    Value::Value(const char* value) :
        value_{ .string_ = duplicateStringValue(value) }, type_{ stringValue }, allocated_{ true } {}

    Value::Value(const StaticString& value) :
        value_{ .string_ = const_cast<char*>(value.c_str()) }, type_{ stringValue }, allocated_{ false } {}

    Value::Value(std::string_view value) :
        value_{ .string_ = duplicateStringValue(value.data(), (unsigned int)value.length()) }, type_{ stringValue }, allocated_{ true } {}

    Value::Value(const std::string& value) :
        Value{ std::string_view{ value } } {}

    Value::Value(bool value) :
        value_{ .bool_ = value }, type_{ booleanValue }, allocated_{ false } {}

    Value::Value(const Value& other) :
        value_{}, type_{ other.type_ }, allocated_{ false }
    {
        switch (type_) {
        case nullValue:
        case intValue:
        case uintValue:
        case realValue:
        case booleanValue:
            value_ = other.value_;
            break;
        case stringValue:
            if (other.value_.string_) {
                value_.string_ = duplicateStringValue(other.value_.string_);
                allocated_ = true;
            }
            else
                value_.string_ = nullptr;
            break;
        case arrayValue:
        case objectValue:
            value_.map_ = new ObjectValues(*other.value_.map_);
            break;
        default:
            JSONCPP_ASSERT_UNREACHABLE;
        }
    }

    Value::Value(Value&& other) noexcept : Value{} {
        if (this != &other) {
            this->swap(other);
        }
    }

    Value::~Value() {
        switch (type_) {
        case nullValue:
        case intValue:
        case uintValue:
        case realValue:
        case booleanValue:
            break;
        case stringValue:
            if (allocated_)
                releaseStringValue(value_.string_);
            break;
        case arrayValue:
        case objectValue:
            delete value_.map_;
            break;
        default:
            JSONCPP_ASSERT_UNREACHABLE;
        }
    }

    Value& Value::operator=(const Value& other) {
        if (this != &other) {
            Value temp{ other };
            swap(temp);
        }
        return *this;
    }

    Value& Value::operator=(Value&& other) noexcept {
        if (this != &other) {
            this->swap(other);
        }
        return *this;
    }

    void Value::swap(Value& other) {
        std::swap(type_, other.type_);
        std::swap(value_, other.value_);
    }

    ValueType Value::type() const {
        return type_;
    }

    int Value::compare(const Value& other) const {
        if (*this < other)
            return -1;
        if (*this > other)
            return 1;
        return 0;
    }

    bool Value::operator<(const Value& other) const {
        int typeDelta = type_ - other.type_;
        if (typeDelta)
            return typeDelta < 0 ? true : false;
        switch (type_) {
        case nullValue:
            return false;
        case intValue:
            return value_.int_ < other.value_.int_;
        case uintValue:
            return value_.uint_ < other.value_.uint_;
        case realValue:
            return value_.real_ < other.value_.real_;
        case booleanValue:
            return value_.bool_ < other.value_.bool_;
        case stringValue:
            return (!value_.string_ && other.value_.string_) ||
                (other.value_.string_ && value_.string_ && strcmp(value_.string_, other.value_.string_) < 0);
        case arrayValue:
        case objectValue: {
            int delta = int(value_.map_->size() - other.value_.map_->size());
            if (delta)
                return delta < 0;
            return (*value_.map_) < (*other.value_.map_);
        }
        default:
            JSONCPP_ASSERT_UNREACHABLE;
        }
        return false; // unreachable
    }

    bool Value::operator<=(const Value& other) const {
        return !(other < *this);
    }

    bool Value::operator>=(const Value& other) const {
        return !(*this < other);
    }

    bool Value::operator>(const Value& other) const {
        return other < *this;
    }

    bool Value::operator==(const Value& other) const {
        if (type_ != other.type_)
            return false;
        switch (type_) {
        case nullValue:
            return true;
        case intValue:
            return value_.int_ == other.value_.int_;
        case uintValue:
            return value_.uint_ == other.value_.uint_;
        case realValue:
            return value_.real_ == other.value_.real_;
        case booleanValue:
            return value_.bool_ == other.value_.bool_;
        case stringValue:
            return (value_.string_ == other.value_.string_) ||
                (other.value_.string_ && value_.string_ && strcmp(value_.string_, other.value_.string_) == 0);
        case arrayValue:
        case objectValue:
            return value_.map_->size() == other.value_.map_->size() && (*value_.map_) == (*other.value_.map_);
        default:
            JSONCPP_ASSERT_UNREACHABLE;
        }
        return false; // unreachable
    }

    bool Value::operator!=(const Value& other) const {
        return !(*this == other);
    }

    const char* Value::asCString() const {
        JSONCPP_ASSERT(type_ == stringValue);
        return value_.string_;
    }

    std::string Value::asString(const std::string& defaultValue) const {
        switch (type_) {
        case nullValue:
            return defaultValue;
        case stringValue:
            return value_.string_ ? value_.string_ : "";
        case booleanValue:
            return value_.bool_ ? "true" : "false";
        case intValue:
        case uintValue:
        case realValue:
        case arrayValue:
        case objectValue: {
            JSONCPP_FAIL_MESSAGE("Type is not convertible to string");
            break;
        }
        default:
            JSONCPP_ASSERT_UNREACHABLE;
        }
        return defaultValue; // unreachable
    }

    Value::Int Value::asInt(Value::Int defaultValue) const {
        switch (type_) {
        case nullValue:
            return defaultValue;
        case intValue:
            JSONCPP_ASSERT_MESSAGE(value_.int_ >= minInt && value_.int_ <= maxInt, "unsigned integer out of signed int range");
            return Int(value_.int_);
        case uintValue:
            JSONCPP_ASSERT_MESSAGE(value_.uint_ <= UInt(maxInt), "unsigned integer out of signed int range");
            return Int(value_.uint_);
        case realValue:
            JSONCPP_ASSERT_MESSAGE(value_.real_ >= minInt && value_.real_ <= maxInt, "Real out of signed integer range");
            return Int(value_.real_);
        case booleanValue:
            return value_.bool_ ? 1 : 0;
        case stringValue:
        case arrayValue:
        case objectValue: {
            JSONCPP_FAIL_MESSAGE("Type is not convertible to int");
            break;
        }
        default:
            JSONCPP_ASSERT_UNREACHABLE;
        }
        return defaultValue; // unreachable;
    }

    Value::UInt Value::asUInt(Value::UInt defaultValue) const {
        switch (type_) {
        case nullValue:
            return defaultValue;
        case intValue:
            JSONCPP_ASSERT_MESSAGE(value_.int_ >= 0, "Negative integer can not be converted to unsigned integer");
            JSONCPP_ASSERT_MESSAGE(value_.int_ <= maxUInt, "signed integer out of UInt range");
            return UInt(value_.int_);
        case uintValue:
            JSONCPP_ASSERT_MESSAGE(value_.uint_ <= maxUInt, "unsigned integer out of UInt range");
            return UInt(value_.uint_);
        case realValue:
            JSONCPP_ASSERT_MESSAGE(value_.real_ >= 0 && value_.real_ <= maxUInt, "Real out of unsigned integer range");
            return UInt(value_.real_);
        case booleanValue:
            return value_.bool_ ? 1 : 0;
        case stringValue:
        case arrayValue:
        case objectValue: {
            JSONCPP_FAIL_MESSAGE("Type is not convertible to uint");
            break;
        }
        default:
            JSONCPP_ASSERT_UNREACHABLE;
        }
        return defaultValue; // unreachable;
    }

#if defined(JSONCPP_HAS_INT64)

    Value::Int64 Value::asInt64(Value::Int64 defaultValue) const {
        switch (type_) {
        case nullValue:
            return defaultValue;
        case intValue:
            return value_.int_;
        case uintValue:
            JSONCPP_ASSERT_MESSAGE(value_.uint_ <= UInt64(maxInt64), "unsigned integer out of Int64 range");
            return value_.uint_;
        case realValue:
            JSONCPP_ASSERT_MESSAGE(
                value_.real_ >= std::nextafter(static_cast<double>(minInt64), 0.0) && value_.real_ <= std::nextafter(static_cast<double>(maxInt64), 0.0),
                "Real out of Int64 range"
            );
            return Int(value_.real_);
        case booleanValue:
            return value_.bool_ ? 1 : 0;
        case stringValue:
        case arrayValue:
        case objectValue: {
            JSONCPP_FAIL_MESSAGE("Type is not convertible to Int64");
            break;
        }
        default:
            JSONCPP_ASSERT_UNREACHABLE;
        }
        return defaultValue; // unreachable;
    }

    Value::UInt64 Value::asUInt64(Value::UInt64 defaultValue) const {
        switch (type_) {
        case nullValue:
            return defaultValue;
        case intValue:
            JSONCPP_ASSERT_MESSAGE(value_.int_ >= 0, "Negative integer can not be converted to UInt64");
            return value_.int_;
        case uintValue:
            return value_.uint_;
        case realValue:
            JSONCPP_ASSERT_MESSAGE(value_.real_ >= 0 && value_.real_ <= std::nextafter(static_cast<double>(maxUInt64), 0.0), "Real out of UInt64 range");
            return UInt(value_.real_);
        case booleanValue:
            return value_.bool_ ? 1 : 0;
        case stringValue:
        case arrayValue:
        case objectValue: {
            JSONCPP_FAIL_MESSAGE("Type is not convertible to UInt64");
            break;
        }
        default:
            JSONCPP_ASSERT_UNREACHABLE;
        }
        return defaultValue; // unreachable;
    }
#endif // if defined(JSONCPP_HAS_INT64)

    LargestInt Value::asLargestInt() const {
#if defined(JSONCPP_NO_INT64)
        return asInt(0);
#else
        return asInt64(0);
#endif
    }

    LargestUInt Value::asLargestUInt() const {
#if defined(JSONCPP_NO_INT64)
        return asUInt(0);
#else
        return asUInt64(0);
#endif
    }

    double Value::asDouble(double defaultValue) const {
        switch (type_) {
        case nullValue:
            return defaultValue;
        case intValue:
            return static_cast<double>(value_.int_);
        case uintValue:
#if !defined(JSONCPP_USE_INT64_DOUBLE_CONVERSION)
            return static_cast<double>(value_.uint_);
#else  // if !defined(JSONCPP_USE_INT64_DOUBLE_CONVERSION)
            return static_cast<double>(Int(value_.uint_ / 2)) * 2 + Int(value_.uint_ & 1);
#endif // if !defined(JSONCPP_USE_INT64_DOUBLE_CONVERSION)
        case realValue:
            return value_.real_;
        case booleanValue:
            return value_.bool_ ? 1.0 : 0.0;
        case stringValue:
        case arrayValue:
        case objectValue: {
            JSONCPP_FAIL_MESSAGE("Type is not convertible to double");
            break;
        }
        default:
            JSONCPP_ASSERT_UNREACHABLE;
        }
        return defaultValue; // unreachable;
    }

    float Value::asFloat(float defaultValue) const {
        switch (type_) {
        case nullValue:
            return defaultValue;
        case intValue:
            return static_cast<float>(value_.int_);
        case uintValue:
#if !defined(JSONCPP_USE_INT64_DOUBLE_CONVERSION)
            return static_cast<float>(value_.uint_);
#else  // if !defined(JSONCPP_USE_INT64_DOUBLE_CONVERSION)
            return static_cast<float>(Int(value_.uint_ / 2)) * 2 + Int(value_.uint_ & 1);
#endif // if !defined(JSONCPP_USE_INT64_DOUBLE_CONVERSION)
        case realValue:
            return static_cast<float>(value_.real_);
        case booleanValue:
            return value_.bool_ ? 1.0f : 0.0f;
        case stringValue:
        case arrayValue:
        case objectValue: {
            JSONCPP_FAIL_MESSAGE("Type is not convertible to float");
            break;
        }
        default:
            JSONCPP_ASSERT_UNREACHABLE;
        }
        return defaultValue; // unreachable;
    }

    bool Value::asBool(bool defaultValue) const {
        switch (type_) {
        case nullValue:
            return defaultValue;
        case intValue:
        case uintValue:
            return value_.int_ != 0;
        case realValue:
            return value_.real_ != 0.0;
        case booleanValue:
            return value_.bool_;
        case stringValue:
            return value_.string_ && value_.string_[0] != '\0';
        case arrayValue:
        case objectValue:
            return !value_.map_->empty();
        default:
            JSONCPP_ASSERT_UNREACHABLE;
        }
        return defaultValue; // unreachable;
    }

    bool Value::isConvertibleTo(ValueType other) const {
        switch (type_) {
        case nullValue:
            return true;
        case intValue:
            return (other == nullValue && value_.int_ == 0) || other == intValue || (other == uintValue && value_.int_ >= 0) || other == realValue ||
                other == stringValue || other == booleanValue;
        case uintValue:
            return (other == nullValue && value_.uint_ == 0) || (other == intValue && value_.uint_ <= (unsigned)maxInt) || other == uintValue ||
                other == realValue || other == stringValue || other == booleanValue;
        case realValue:
            return (other == nullValue && value_.real_ == 0.0) || (other == intValue && value_.real_ >= minInt && value_.real_ <= maxInt) ||
                (other == uintValue && value_.real_ >= 0 && value_.real_ <= maxUInt) || other == realValue || other == stringValue ||
                other == booleanValue;
        case booleanValue:
            return (other == nullValue && value_.bool_ == false) || other == intValue || other == uintValue || other == realValue || other == stringValue ||
                other == booleanValue;
        case stringValue:
            return other == stringValue || (other == nullValue && (!value_.string_ || value_.string_[0] == '\0'));
        case arrayValue:
            return other == arrayValue || (other == nullValue && value_.map_->empty());
        case objectValue:
            return other == objectValue || (other == nullValue && value_.map_->empty());
        default:
            JSONCPP_ASSERT_UNREACHABLE;
        }
        return false; // unreachable;
    }

    /// Number of values in array or object
    ArrayIndex Value::size() const {
        switch (type_) {
        case nullValue:
        case intValue:
        case uintValue:
        case realValue:
        case booleanValue:
        case stringValue:
            return 0;
        case arrayValue: // size of the array is highest index + 1
            if (!value_.map_->empty()) {
                ObjectValues::const_iterator itLast = value_.map_->end();
                --itLast;
                return (*itLast).first.index() + 1;
            }
            return 0;
        case objectValue:
            return ArrayIndex(value_.map_->size());
        default:
            JSONCPP_ASSERT_UNREACHABLE;
        }
        return 0; // unreachable;
    }

    bool Value::empty() const {
        if (isNull() || isArray() || isObject())
            return size() == 0u;
        else
            return false;
    }

    bool Value::operator!() const {
        return isNull();
    }

    void Value::clear() {
        JSONCPP_ASSERT(type_ == nullValue || type_ == arrayValue || type_ == objectValue);

        switch (type_) {
        case arrayValue:
        case objectValue:
            value_.map_->clear();
            break;
        default:
            break;
        }
    }

    void Value::resize(ArrayIndex newSize) {
        JSONCPP_ASSERT(type_ == nullValue || type_ == arrayValue);
        if (type_ == nullValue)
            *this = Value(arrayValue);
        ArrayIndex oldSize = size();
        if (newSize == 0)
            clear();
        else if (newSize > oldSize)
            (*this)[newSize - 1];
        else {
            for (ArrayIndex index = newSize; index < oldSize; ++index) {
                value_.map_->erase(index);
            }
            assert(size() == newSize);
        }
    }

    Value& Value::get(ArrayIndex index) {
        auto value = this->tryGet(index);
        JSONCPP_ASSERT(value != nullptr);
        return *value;
    }

    const Value& Value::get(ArrayIndex index) const {
        auto value = this->tryGet(index);
        JSONCPP_ASSERT(value != nullptr);
        return *value;
    }

    Value& Value::get(std::string_view key) {
        auto value = this->tryGet(key);
        JSONCPP_ASSERT(value != nullptr);
        return *value;
    }

    const Value& Value::get(std::string_view key) const {
        auto value = this->tryGet(key);
        JSONCPP_ASSERT(value != nullptr);
        return *value;
    }

    Value& Value::get(const CZString& key) {
        auto value = this->tryGet(key);
        JSONCPP_ASSERT(value != nullptr);
        return *value;
    }

    const Value& Value::get(const CZString& key) const {
        auto value = this->tryGet(key);
        JSONCPP_ASSERT(value != nullptr);
        return *value;
    }

    Value* Value::tryGet(ArrayIndex index) {
        return const_cast<Value*>(std::as_const(*this).tryGet(index));
    }

    const Value* Value::tryGet(ArrayIndex index) const {
        JSONCPP_ASSERT(type_ == nullValue || type_ == arrayValue);
        if (type_ == nullValue)
            return nullptr;

        const auto it = value_.map_->find(index);
        if (it != value_.map_->end()) {
            return &it->second;
        }
        return nullptr;
    }

    Value* Value::tryGet(std::string_view key) {
        return const_cast<Value*>(std::as_const(*this).tryGet(key));
    }

    const Value* Value::tryGet(std::string_view key) const {
        JSONCPP_ASSERT(type_ == nullValue || type_ == objectValue);
        if (type_ == nullValue)
            return nullptr;

        const auto it = value_.map_->find(key);
        if (it != value_.map_->end()) {
            return &it->second;
        }
        return nullptr;
    }

    Value* Value::tryGet(const CZString& key) {
        return const_cast<Value*>(std::as_const(*this).tryGet(key));
    }

    const Value* Value::tryGet(const CZString& key) const {
        JSONCPP_ASSERT(type_ == nullValue || type_ == objectValue);
        if (type_ == nullValue)
            return nullptr;

        const auto it = value_.map_->find(key);
        if (it != value_.map_->end()) {
            return &it->second;
        }
        return nullptr;
    }

    Value& Value::operator[](ArrayIndex index) {
        JSONCPP_ASSERT(type_ == nullValue || type_ == arrayValue);
        if (type_ == nullValue)
            *this = Value(arrayValue);

        auto ret = this->tryGet(index);
        if (ret) {
            return *ret;
        }
        return value_.map_->emplace(index, null).first->second;
    }

    Value& Value::operator[](std::string_view key) {
        JSONCPP_ASSERT(type_ == nullValue || type_ == objectValue);
        if (type_ == nullValue)
            *this = Value(objectValue);

        auto ret = this->tryGet(key);
        if (ret) {
            return *ret;
        }
        return value_.map_->emplace(key, null).first->second;
    }

    Value& Value::operator[](const CZString& key) {
        JSONCPP_ASSERT(type_ == nullValue || type_ == objectValue);
        if (type_ == nullValue)
            *this = Value(objectValue);

        auto ret = this->tryGet(key);
        if (ret) {
            return *ret;
        }
        return value_.map_->emplace(key, null).first->second;
    }

    Value& Value::operator[](const StaticString& key) {
        return resolveReference(key, true);
    }

    Value& Value::resolveReference(const char* key, bool isStatic) {
        JSONCPP_ASSERT(type_ == nullValue || type_ == objectValue);
        if (type_ == nullValue)
            *this = Value(objectValue);

        CZString actualKey(key, isStatic ? CZString::noDuplication : CZString::duplicateOnCopy);
        auto ret = this->tryGet(actualKey);
        if (ret) {
            return *ret;
        }
        return value_.map_->emplace(actualKey, null).first->second;
    }

    bool Value::isValidIndex(ArrayIndex index) const {
        return index < size();
    }

    Value& Value::append(const Value& value) {
        return (*this)[size()] = value;
    }

    Value Value::removeMember(std::string_view key) {
        JSONCPP_ASSERT(type_ == nullValue || type_ == objectValue);
        if (type_ == nullValue)
            return null;

        const auto it = value_.map_->find(key);
        if (it != value_.map_->end()) {
            Value old(it->second);
            value_.map_->erase(it);
            return old;
        }
        return null;
    }

    bool Value::isMember(std::string_view key) const {
        return this->tryGet(key) != nullptr;
    }

    bool Value::isMember(const CZString& key) const {
        return this->tryGet(key) != nullptr;
    }

    const Value::ObjectValues& Value::items() const {
        JSONCPP_ASSERT(type_ == objectValue);
        return *value_.map_;
    }

    bool Value::isNull() const {
        return type_ == nullValue;
    }

    bool Value::isBool() const {
        return type_ == booleanValue;
    }

    bool Value::isInt() const {
        return type_ == intValue;
    }

    bool Value::isUInt() const {
        return type_ == uintValue;
    }

    bool Value::isIntegral() const {
        return type_ == intValue || type_ == uintValue || type_ == booleanValue;
    }

    bool Value::isDouble() const {
        return type_ == realValue;
    }

    bool Value::isNumeric() const {
        return isIntegral() || isDouble();
    }

    bool Value::isString() const {
        return type_ == stringValue;
    }

    bool Value::isArray() const {
        return type_ == nullValue || type_ == arrayValue;
    }

    bool Value::isObject() const {
        return type_ == nullValue || type_ == objectValue;
    }

    void Value::setComment([[maybe_unused]] const char* comment, [[maybe_unused]] CommentPlacement placement) {}

    void Value::setComment(const std::string& comment, CommentPlacement placement) {
        setComment(comment.c_str(), placement);
    }

    bool Value::hasComment([[maybe_unused]] CommentPlacement placement) const {
        return false;
    }

    std::string Value::getComment([[maybe_unused]] CommentPlacement placement) const {
        return "";
    }

    std::string Value::toStyledString() const {
        StyledWriter writer;
        return writer.write(*this);
    }

    Value::const_iterator Value::begin() const {
        switch (type_) {
        case arrayValue:
        case objectValue:
            if (value_.map_)
                return const_iterator(value_.map_->begin());
            break;
        default:
            break;
        }
        return {};
    }

    Value::const_iterator Value::end() const {
        switch (type_) {
        case arrayValue:
        case objectValue:
            if (value_.map_)
                return const_iterator(value_.map_->end());
            break;
        default:
            break;
        }
        return {};
    }

    Value::iterator Value::begin() {
        switch (type_) {
        case arrayValue:
        case objectValue:
            if (value_.map_)
                return iterator(value_.map_->begin());
            break;
        default:
            break;
        }
        return {};
    }

    Value::iterator Value::end() {
        switch (type_) {
        case arrayValue:
        case objectValue:
            if (value_.map_)
                return iterator(value_.map_->end());
            break;
        default:
            break;
        }
        return {};
    }


    // class Path
    // //////////////////////////////////////////////////////////////////

    Path::Path(
        const std::string& path, const PathArgument& a1, const PathArgument& a2, const PathArgument& a3, const PathArgument& a4, const PathArgument& a5
    ) {
        InArgs in;
        in.push_back(&a1);
        in.push_back(&a2);
        in.push_back(&a3);
        in.push_back(&a4);
        in.push_back(&a5);
        makePath(path, in);
    }

    void Path::makePath(const std::string& path, const InArgs& in) {
        const char* current = path.c_str();
        const char* end = current + path.length();
        InArgs::const_iterator itInArg = in.begin();
        while (current != end) {
            if (*current == '[') {
                ++current;
                if (*current == '%')
                    addPathInArg(path, in, itInArg, PathArgument::kindIndex);
                else {
                    ArrayIndex index = 0;
                    for (; current != end && *current >= '0' && *current <= '9'; ++current)
                        index = index * 10 + ArrayIndex(*current - '0');
                    args_.emplace_back(index);
                }
                if (current == end || *current++ != ']')
                    invalidPath(path, int(current - path.c_str()));
            } else if (*current == '%') {
                addPathInArg(path, in, itInArg, PathArgument::kindKey);
                ++current;
            } else if (*current == '.') {
                ++current;
            } else {
                const char* beginName = current;
                while (current != end && !strchr("[.", *current))
                    ++current;
                args_.push_back(std::string(beginName, current));
            }
        }
    }

    void Path::addPathInArg([[maybe_unused]] const std::string& path, const InArgs& in, InArgs::const_iterator& itInArg, PathArgument::Kind kind) {
        if (itInArg == in.end()) {
            // Error: missing argument %d
        } else if ((*itInArg)->kind_ != kind) {
            // Error: bad argument type
        } else {
            args_.push_back(**itInArg);
        }
    }

    void Path::invalidPath([[maybe_unused]] const std::string& path, [[maybe_unused]] int location) {
        // Error: invalid path.
    }

    const Value& Path::resolve(const Value& root) const {
        const Value* node = &root;
        for (const auto& arg : args_) {
            if (arg.kind_ == PathArgument::kindIndex) {
                if (!node->isArray() || node->isValidIndex(arg.index_)) {
                    // Error: unable to resolve path (array value expected at position...
                }

                {
                    auto temp = node->tryGet(arg.index_);
                    node = (temp ? temp : &Value::null);
                }
            } else if (arg.kind_ == PathArgument::kindKey) {
                if (!node->isObject()) {
                    // Error: unable to resolve path (object value expected at position...)
                }

                {
                    auto temp = node->tryGet(arg.key_);
                    node = (temp ? temp : &Value::null);
                }
                if (node == &Value::null) {
                    // Error: unable to resolve path (object has no member named '' at position...)
                }
            }
        }
        return *node;
    }

    Value Path::resolve(const Value& root, const Value& defaultValue) const {
        const Value* node = &root;
        for (const auto& arg : args_) {
            if (arg.kind_ == PathArgument::kindIndex) {
                if (!node->isArray() || node->isValidIndex(arg.index_))
                    return defaultValue;

                {
                    auto temp = node->tryGet(arg.index_);
                    node = (temp ? temp : &Value::null);
                }
            } else if (arg.kind_ == PathArgument::kindKey) {
                if (!node->isObject())
                    return defaultValue;
                
                {
                    auto temp = node->tryGet(arg.key_);
                    node = (temp ? temp : &Value::null);
                }
                if (node == &Value::null)
                    return defaultValue;
            }
        }
        return *node;
    }

    Value& Path::make(Value& root) const {
        Value* node = &root;
        for (const auto& arg : args_) {
            if (arg.kind_ == PathArgument::kindIndex) {
                if (!node->isArray()) {
                    // Error: node is not an array at position ...
                }
                node = &((*node)[arg.index_]);
            } else if (arg.kind_ == PathArgument::kindKey) {
                if (!node->isObject()) {
                    // Error: node is not an object at position...
                }
                node = &((*node)[arg.key_]);
            }
        }
        return *node;
    }

} // namespace Json
