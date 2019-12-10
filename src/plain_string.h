/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_PLAIN_STRING_H
#define LIBSBOX_PLAIN_STRING_H

#include "utils.h"

#include <stddef.h>
#include <string>
#include <stdexcept>
#include <cstring>

template<size_t MaxSize>
class PlainString {
public:
    using value_type = char;
    using size_type = size_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;

    PlainString() = default;
    PlainString(const std::basic_string<value_type> &str);
    ~PlainString() = default;

    reference at(size_type pos);
    const_reference at(size_type pos) const;

    reference operator[](size_type pos);
    const_reference operator[](size_type pos) const;

    reference front();
    const_reference front() const;

    reference back();
    const_reference back() const;

    pointer data();
    const_pointer data() const;

    pointer c_str();
    const_pointer c_str() const;

    bool empty() const;
    size_type size() const;
    size_type max_size() const;

    void clear();

    void push_back(value_type value);
    void pop_back();

    PlainString<MaxSize> &operator=(const std::basic_string<value_type> &str);
    PlainString<MaxSize> &operator=(const_pointer str);

    PlainString<MaxSize> &operator+=(const std::basic_string<value_type> &str);
    PlainString<MaxSize> &operator+=(const_pointer str);
    PlainString<MaxSize> &operator+=(const_reference ch);

    bool operator==(const std::basic_string<value_type> &str);
    bool operator==(const_pointer str);

    std::string substr(size_type pos);
    std::string substr(size_type pos, size_type len);

private:
    size_type size_ = 0;
    value_type data_[MaxSize + 1]{};

    void set(const_pointer str, size_type len);
    void append(const_pointer str, size_type len);
    bool equals(const_pointer str, size_type len);
};

template<size_t MaxSize>
PlainString<MaxSize>::PlainString(const std::basic_string<value_type> &str) {
    this->set(str.c_str(), str.size());
}

template<size_t MaxSize>
typename PlainString<MaxSize>::reference
PlainString<MaxSize>::at(size_type pos) {
    if (pos >= size_) {
        throw std::out_of_range(format("PlainString[%d] index out of bounds [%d, %d)", pos, 0, size_));
    }
    return data_[pos];
}

template<size_t MaxSize>
typename PlainString<MaxSize>::const_reference
PlainString<MaxSize>::at(size_type pos) const {
    if (pos >= size_) {
        throw std::out_of_range(format("PlainString[%d] index out of bounds [%d, %d)", pos, 0, size_));
    }
    return data_[pos];
}

template<size_t MaxSize>
typename PlainString<MaxSize>::reference
PlainString<MaxSize>::operator[](size_type pos) {
    return data_[pos];
}

template<size_t MaxSize>
typename PlainString<MaxSize>::const_reference
PlainString<MaxSize>::operator[](size_type pos) const {
    return data_[pos];
}

template<size_t MaxSize>
typename PlainString<MaxSize>::reference
PlainString<MaxSize>::front() {
    return data_[0];
}

template<size_t MaxSize>
typename PlainString<MaxSize>::const_reference
PlainString<MaxSize>::front() const {
    return data_[0];
}

template<size_t MaxSize>
typename PlainString<MaxSize>::reference
PlainString<MaxSize>::back() {
    return data_[size_ - 1];
}

template<size_t MaxSize>
typename PlainString<MaxSize>::const_reference
PlainString<MaxSize>::back() const {
    return data_[size_ - 1];
}

template<size_t MaxSize>
typename PlainString<MaxSize>::pointer
PlainString<MaxSize>::data() {
    return data_;
}

template<size_t MaxSize>
typename PlainString<MaxSize>::const_pointer
PlainString<MaxSize>::data() const {
    return data_;
}

template<size_t MaxSize>
typename PlainString<MaxSize>::pointer
PlainString<MaxSize>::c_str() {
    return data_;
}

template<size_t MaxSize>
typename PlainString<MaxSize>::const_pointer
PlainString<MaxSize>::c_str() const {
    return data_;
}

template<size_t MaxSize>
bool PlainString<MaxSize>::empty() const {
    return (size_ == 0);
}

template<size_t MaxSize>
typename PlainString<MaxSize>::size_type
PlainString<MaxSize>::size() const {
    return size_;
}

template<size_t MaxSize>
typename PlainString<MaxSize>::size_type
PlainString<MaxSize>::max_size() const {
    return MaxSize;
}

template<size_t MaxSize>
void PlainString<MaxSize>::clear() {
    size_ = 0;
    data_[0] = 0;
}

template<size_t MaxSize>
void PlainString<MaxSize>::push_back(value_type value) {
    data_[size_] = value;
    size_++;
    data_[size_] = 0;
}

template<size_t MaxSize>
void PlainString<MaxSize>::pop_back() {
    size_--;
    data_[size_] = 0;
}

template<size_t MaxSize>
PlainString<MaxSize> &PlainString<MaxSize>::operator=(const std::basic_string<value_type> &str) {
    set(str.c_str(), str.size());
    return (*this);
}

template<size_t MaxSize>
PlainString<MaxSize> &PlainString<MaxSize>::operator=(const_pointer str) {
    set(str, strlen(str));
    return (*this);
}

template<size_t MaxSize>
PlainString<MaxSize> &PlainString<MaxSize>::operator+=(const std::basic_string<value_type> &str) {
    append(str.c_str(), str.size());
    return (*this);
}

template<size_t MaxSize>
PlainString<MaxSize> &PlainString<MaxSize>::operator+=(const_pointer str) {
    append(str, strlen(str));
    return (*this);
}

template<size_t MaxSize>
PlainString<MaxSize> &PlainString<MaxSize>::operator+=(const_reference ch) {
    push_back(ch);
    return (*this);
}

template<size_t MaxSize>
bool PlainString<MaxSize>::operator==(const std::basic_string<value_type> &str) {
    return equals(str.c_str(), str.size());
}

template<size_t MaxSize>
bool PlainString<MaxSize>::operator==(const_pointer str) {
    return equals(str, strlen(str));
}

template<size_t MaxSize>
std::string PlainString<MaxSize>::substr(size_type pos) {
    return std::string(data_ + pos);
}

template<size_t MaxSize>
std::string PlainString<MaxSize>::substr(size_type pos, size_type len) {
    return std::string(data_ + pos, len);
}

template<size_t MaxSize>
void PlainString<MaxSize>::set(const_pointer str, size_type len) {
    size_ = len;
    for (size_type i = 0; i < len; ++i) {
        data_[i] = str[i];
    }
    data_[len] = 0;
}

template<size_t MaxSize>
void PlainString<MaxSize>::append(const_pointer str, size_type len) {
    for (size_type i = 0; i < len; ++i) {
        data_[size_] = str[i];
        size_++;
    }
    data_[size_] = 0;
}

template<size_t MaxSize>
bool PlainString<MaxSize>::equals(const_pointer str, size_type len) {
    if (size_ != len) return false;
    for (size_t i = 0; i < size_; ++i) {
        if (str[i] != data_[i]) {
            return false;
        }
    }
    return true;
}

#endif //LIBSBOX_PLAIN_STRING_H
