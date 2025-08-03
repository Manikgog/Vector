#pragma once
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <new>
#include <ostream>
#include <utility>
#include <memory>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    RawMemory(const RawMemory &) = delete;
    RawMemory &operator=(const RawMemory &) = delete;

    RawMemory(RawMemory&& other) noexcept
    : buffer_(std::move(other.buffer_))
    , capacity_(other.Capacity()){}

    RawMemory& operator=(RawMemory&& other) noexcept {
        if (this != &other) {
            Deallocate(buffer_); // Освобождаем текущие ресурсы
            buffer_ = other.buffer_;
            capacity_ = other.capacity_;
            other.buffer_ = nullptr;
            other.capacity_ = 0;
        }
        return *this; // Явно возвращаем *this
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};







template <typename T>
class Vector {
public:

    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }

    Vector(Vector&& other) noexcept {
        Swap(other);
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        } else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {

            if (rhs.size_ > data_.Capacity()) {
                /* Применить copy-and-swap */
                // Если новый размер больше текущей capacity - создаем новый буфер
                Vector tmp(rhs);
                Swap(tmp);
            } else {
                /* Скопировать элементы из rhs, создав при необходимости новые
                   или удалив существующие */
                // Если места достаточно - копируем на место
                if (rhs.size_ <= size_) {
                    // Уничтожаем лишние элементы
                    std::destroy_n(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
                }

                // Копируем элементы
                size_t to_copy = std::min(size_, rhs.size_);
                std::copy_n(rhs.data_.GetAddress(), to_copy, data_.GetAddress());

                // Если rhs больше - создаем новые элементы
                if (rhs.size_ > size_) {
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + size_,
                                            rhs.size_ - size_,
                                            data_.GetAddress() + size_);
                }

                size_ = rhs.size_;
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& rhs) {
        if (this != &rhs) {
             Swap(rhs);
        }
        return *this;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;
};