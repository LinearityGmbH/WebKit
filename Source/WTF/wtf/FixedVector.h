/*
 * Copyright (C) 2021 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <wtf/EmbeddedFixedVector.h>
#include <wtf/RefCountedArray.h>

namespace WTF {

// Wrapper around RefCountedArray, fixed-sized, memory compact Vector.
// Copy constructor / assignment operator work differently from RefCountedArray: works as like a Vector.
template<typename T>
class FixedVector {
public:
    using Storage = EmbeddedFixedVector<T>;
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    FixedVector() = default;
    FixedVector(const FixedVector& other)
        : m_storage(other.m_storage ? other.m_storage->clone().moveToUniquePtr() : nullptr)
    { }
    FixedVector(FixedVector&& other) = default;

    FixedVector& operator=(const FixedVector& other)
    {
        FixedVector tmp(other);
        swap(tmp);
        return *this;
    }

    FixedVector& operator=(FixedVector&& other)
    {
        FixedVector tmp(WTFMove(other));
        swap(tmp);
        return *this;
    }

    explicit FixedVector(size_t size)
        : m_storage(size ? Storage::create(size).moveToUniquePtr() : nullptr)
    { }

    template<size_t inlineCapacity, typename OverflowHandler>
    explicit FixedVector(const Vector<T, inlineCapacity, OverflowHandler>& other)
        : m_storage(other.isEmpty() ? nullptr : Storage::createFromVector(other).moveToUniquePtr())
    { }

    template<size_t inlineCapacity, typename OverflowHandler>
    FixedVector& operator=(const Vector<T, inlineCapacity, OverflowHandler>& other)
    {
        m_storage = other.isEmpty() ? nullptr : Storage::createFromVector(other).moveToUniquePtr();
        return *this;
    }

    template<size_t inlineCapacity, typename OverflowHandler>
    explicit FixedVector(Vector<T, inlineCapacity, OverflowHandler>&& other)
    {
        Vector<T, inlineCapacity, OverflowHandler> target = WTFMove(other);
        m_storage = target.isEmpty() ? nullptr : Storage::createFromVector(WTFMove(target)).moveToUniquePtr();
    }

    template<size_t inlineCapacity, typename OverflowHandler>
    FixedVector& operator=(Vector<T, inlineCapacity, OverflowHandler>&& other)
    {
        Vector<T, inlineCapacity, OverflowHandler> target = WTFMove(other);
        m_storage = target.isEmpty() ? nullptr : Storage::createFromVector(WTFMove(target)).moveToUniquePtr();
        return *this;
    }

    size_t size() const { return m_storage ? m_storage->size() : 0; }
    bool isEmpty() const { return m_storage ? m_storage->isEmpty() : true; }
    size_t byteSize() const { return m_storage ? m_storage->byteSize() : 0; }

    T* data() { return m_storage ? m_storage->data() : nullptr; }
    iterator begin() { return m_storage ? m_storage->begin() : nullptr; }
    iterator end() { return m_storage ? m_storage->end() : nullptr; }

    const T* data() const { return const_cast<FixedVector*>(this)->data(); }
    const_iterator begin() const { return const_cast<FixedVector*>(this)->begin(); }
    const_iterator end() const { return const_cast<FixedVector*>(this)->end(); }

    reverse_iterator rbegin() { return m_storage ? m_storage->rbegin() : reverse_iterator(nullptr); }
    reverse_iterator rend() { return m_storage ? m_storage->rend() : reverse_iterator(nullptr); }
    const_reverse_iterator rbegin() const { return m_storage ? m_storage->rbegin() : const_reverse_iterator(nullptr); }
    const_reverse_iterator rend() const { return m_storage ? m_storage->rend() : const_reverse_iterator(nullptr); }

    T& at(size_t i) { return m_storage->at(i); }
    const T& at(size_t i) const { return m_storage->at(i); }

    T& operator[](size_t i) { return m_storage->at(i); }
    const T& operator[](size_t i) const { return m_storage->at(i); }

    T& first() { return (*this)[0]; }
    const T& first() const { return (*this)[0]; }
    T& last() { return (*this)[size() - 1]; }
    const T& last() const { return (*this)[size() - 1]; }

    void fill(const T& val)
    {
        if (!m_storage)
            return;
        m_storage->fill(val);
    }

    bool operator!=(const FixedVector<T>& other) const
    {
        return !(*this == other);
    }

    bool operator==(const FixedVector<T>& other) const
    {
        if (!m_storage) {
            if (!other.m_storage)
                return true;
            return other.m_storage->isEmpty();
        }
        if (!other.m_storage)
            return m_storage->isEmpty();
        return *m_storage == *other.m_storage;
    }

    void swap(FixedVector<T>& other)
    {
        using std::swap;
        swap(m_storage, other.m_storage);
    }

    static ptrdiff_t offsetOfStorage() { return OBJECT_OFFSETOF(FixedVector, m_storage); }

    Storage* storage() { return m_storage.get(); }

private:
    friend class JSC::LLIntOffsetsExtractor;

    std::unique_ptr<Storage> m_storage;
};
static_assert(sizeof(FixedVector<int>) == sizeof(int*));

template<typename T>
inline void swap(FixedVector<T>& a, FixedVector<T>& b)
{
    a.swap(b);
}

} // namespace WTF

using WTF::FixedVector;
