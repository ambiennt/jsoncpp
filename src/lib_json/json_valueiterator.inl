// Copyright 2007-2010 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

// included by json_value.cpp

namespace Json {

    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // class ValueIteratorBase
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////

    ValueIteratorBase::ValueIteratorBase()
        : current_(), isNull_(true){}

          ValueIteratorBase::ValueIteratorBase(const Value::ObjectValues::iterator& current)
        : current_(current), isNull_(false) {
    }

    Value& ValueIteratorBase::deref() const {
        return current_->second;
    }

    void ValueIteratorBase::increment() {
        ++current_;
    }

    void ValueIteratorBase::decrement() {
        --current_;
    }

    ValueIteratorBase::difference_type ValueIteratorBase::computeDistance(const SelfType& other) const {
        // Iterator for null value are initialized using the default
        // constructor, which initialize current_ to the default
        // std::map::iterator. As begin() and end() are two instance
        // of the default std::map::iterator, they can not be compared.
        // To allow this, we handle this comparison specifically.
        if (isNull_ && other.isNull_) {
            return 0;
        }

        // Usage of std::distance is not portable (does not compile with Sun Studio 12 RogueWave STL,
        // which is the one used by default).
        // Using a portable hand-made version for non random iterator instead:
        //   return difference_type( std::distance( current_, other.current_ ) );
        difference_type myDistance = 0;
        for (Value::ObjectValues::iterator it = current_; it != other.current_; ++it) {
            ++myDistance;
        }
        return myDistance;
    }

    bool ValueIteratorBase::isEqual(const SelfType& other) const {
        if (isNull_) {
            return other.isNull_;
        }
        return current_ == other.current_;
    }

    void ValueIteratorBase::copy(const SelfType& other) {
        current_ = other.current_;
    }

    Value ValueIteratorBase::key() const {
        const Value::CZString czstring = (*current_).first;
        if (czstring.c_str()) {
            if (czstring.isStaticString())
                return Value(StaticString(czstring.c_str()));
            return Value(czstring.c_str());
        }
        return Value(czstring.index());
    }

    UInt ValueIteratorBase::index() const {
        const Value::CZString czstring = (*current_).first;
        if (!czstring.c_str())
            return czstring.index();
        return Value::UInt(-1);
    }

    const char* ValueIteratorBase::memberName() const {
        const char* name = (*current_).first.c_str();
        return name ? name : "";
    }

    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // class ValueConstIterator
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////

    ValueConstIterator::ValueConstIterator() {}

    ValueConstIterator::ValueConstIterator(const Value::ObjectValues::iterator& current) : ValueIteratorBase(current) {}

    ValueConstIterator& ValueConstIterator::operator=(const ValueIteratorBase& other) {
        copy(other);
        return *this;
    }

    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // class ValueIterator
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////

    ValueIterator::ValueIterator() {}

    ValueIterator::ValueIterator(const Value::ObjectValues::iterator& current) : ValueIteratorBase(current) {}

    ValueIterator::ValueIterator(const ValueConstIterator& other) : ValueIteratorBase(other) {}

    ValueIterator::ValueIterator(const ValueIterator& other) : ValueIteratorBase(other) {}

    ValueIterator& ValueIterator::operator=(const SelfType& other) {
        copy(other);
        return *this;
    }

} // namespace Json
