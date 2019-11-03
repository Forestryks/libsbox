/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_PLAIN_VECTOR_H
#define LIBSBOX_PLAIN_VECTOR_H

#include <stddef.h> // for size_t
#include <cstddef>  // for std::byte
#include <type_traits>
#include <stdexcept>

template<class T, size_t MaxSize>
class PlainVector {
public:
    using value_type = T;
    using size_type = size_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;

    PlainVector() = default;
    ~PlainVector();

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

    bool empty() const;
    size_type size() const;
    size_type max_size() const;

    void clear();

    void push_back(const_reference value);
    template<class... Args>
    reference emplace_back(Args &&... args);
    void pop_back();

protected:
    size_type size_ = 0;
    std::byte data_[MaxSize * sizeof(T)]{};
};

template<class T, size_t MaxSize>
PlainVector<T, MaxSize>::~PlainVector() {
    clear();
}

template<class T, size_t MaxSize>
typename PlainVector<T, MaxSize>::reference
PlainVector<T, MaxSize>::at(PlainVector::size_type pos) {
    if (pos >= size_) {
        throw std::out_of_range(format("PlainVector[%d] index out of bounds [%d, %d)", pos, 0, size_));
    }
    return reinterpret_cast<pointer>(data_)[pos];
}

template<class T, size_t MaxSize>
typename PlainVector<T, MaxSize>::const_reference
PlainVector<T, MaxSize>::at(PlainVector::size_type pos) const {
    if (pos >= size_) {
        throw std::out_of_range(format("PlainVector[%d] index out of bounds [%d, %d)", pos, 0, size_));
    }
    return reinterpret_cast<const_pointer>(data_)[pos];
}

template<class T, size_t MaxSize>
typename PlainVector<T, MaxSize>::reference
PlainVector<T, MaxSize>::operator[](PlainVector::size_type pos) {
    return reinterpret_cast<pointer>(data_)[pos];
}

template<class T, size_t MaxSize>
typename PlainVector<T, MaxSize>::const_reference
PlainVector<T, MaxSize>::operator[](PlainVector::size_type pos) const {
    return reinterpret_cast<const_pointer>(data_)[pos];
}

template<class T, size_t MaxSize>
typename PlainVector<T, MaxSize>::reference
PlainVector<T, MaxSize>::front() {
    return reinterpret_cast<pointer>(data_)[0];
}

template<class T, size_t MaxSize>
typename PlainVector<T, MaxSize>::const_reference
PlainVector<T, MaxSize>::front() const {
    return reinterpret_cast<const_pointer>(data_)[0];
}

template<class T, size_t MaxSize>
typename PlainVector<T, MaxSize>::reference
PlainVector<T, MaxSize>::back() {
    return reinterpret_cast<pointer>(data_)[size_ - 1];
}

template<class T, size_t MaxSize>
typename PlainVector<T, MaxSize>::const_reference
PlainVector<T, MaxSize>::back() const {
    return reinterpret_cast<const_pointer>(data_)[size_ - 1];
}

template<class T, size_t MaxSize>
typename PlainVector<T, MaxSize>::pointer
PlainVector<T, MaxSize>::data() {
    return reinterpret_cast<pointer>(data_);
}

template<class T, size_t MaxSize>
typename PlainVector<T, MaxSize>::const_pointer
PlainVector<T, MaxSize>::data() const {
    return reinterpret_cast<const_pointer>(data_);
}

template<class T, size_t MaxSize>
bool PlainVector<T, MaxSize>::empty() const {
    return (size_ == 0);
}

template<class T, size_t MaxSize>
typename PlainVector<T, MaxSize>::size_type
PlainVector<T, MaxSize>::size() const {
    return size_;
}

template<class T, size_t MaxSize>
typename PlainVector<T, MaxSize>::size_type
PlainVector<T, MaxSize>::max_size() const {
    return MaxSize;
}

template<class T, size_t MaxSize>
void PlainVector<T, MaxSize>::clear() {
    if constexpr (std::is_destructible_v<value_type>) {
        for (size_type i = 0; i < size_; ++i) {
            pointer ptr = reinterpret_cast<pointer>(data_) + i;
            ptr->~value_type();
        }
    }
    size_ = 0;
}

template<class T, size_t MaxSize>
void PlainVector<T, MaxSize>::push_back(const_reference value) {
    pointer ptr = reinterpret_cast<pointer>(data_) + size_;
    new(ptr) value_type(value);
    size_++;
}

template<class T, size_t MaxSize>
template<class... Args>
typename PlainVector<T, MaxSize>::reference
PlainVector<T, MaxSize>::emplace_back(Args &&... args) {
    pointer ptr = reinterpret_cast<pointer>(data_) + size_;
    new(ptr) value_type(args...);
    size_++;
    return back();
}

template<class T, size_t MaxSize>
void PlainVector<T, MaxSize>::pop_back() {
    size_--;
    pointer ptr = reinterpret_cast<pointer>(data_) + size_;
    if (std::is_destructible_v<value_type>) {
        ptr->~value_type();
    }
}

#endif //LIBSBOX_PLAIN_STRING_H
