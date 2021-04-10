#pragma once
#include "array_ptr.h"

#include <algorithm>
#include <stdexcept>
#include <cassert>
#include <utility>

struct ReserveProxyObj {
    size_t capacity;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj{capacity_to_reserve};
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;
    using ItemsPtr = ArrayPtr<Type>;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size)
        :SimpleVector(size, Type{}){
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) 
        : items_(size)
        , size_(size)
        , capacity_(size){
        std::fill(items_.Get(), items_.Get() + size_, value);
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) 
        : items_(init.size())
        , size_(init.size())
        , capacity_(init.size()){
        std::copy(init.begin(), init.end(), items_.Get());
    }

    SimpleVector(ReserveProxyObj reserved)
            : items_(reserved.capacity)
            , size_(0)
            , capacity_(reserved.capacity) {
    }
    
    SimpleVector(const SimpleVector& other) 
        : items_(other.size_)
        , size_(other.size_)
        , capacity_(other.size_){
        std::copy(other.items_.Get(), other.items_.Get() + size_, items_.Get());
    }
    
    SimpleVector(SimpleVector&& other) 
        : items_(std::move(other.items_)){
        size_ = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (&rhs != this){
            if (rhs.IsEmpty())
                Clear(); 
                
            auto rhs_copy(rhs);
                swap(rhs_copy);
        }
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& rhs){
		items_ = std::move(rhs.items_);
        size_ = std::exchange(rhs.size_, 0);
        capacity_ = std::exchange(rhs.capacity_, 0);
        return *this;
	}
    
    void Reserve(size_t new_capacity){
        if (capacity_ < new_capacity){
            auto new_items = ReallocateCopy(new_capacity);
            items_.swap(new_items);
            capacity_ = new_capacity;
        }
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_) 
            throw std::out_of_range{"index >= size"};
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_) 
            throw std::out_of_range{"index >= size"};
        return items_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        
        if (new_size > capacity_){
            const size_t new_capacity = std::max(capacity_ * 2, new_size);
            auto new_items = ReallocateCopy(new_capacity);
            std::fill(new_items.Get() + size_, new_items.Get() + new_size, Type{});
            items_.swap(new_items);
            capacity_ = new_capacity;
        } else
            if (new_size > size_){
                std::fill(items_.Get() + size_, items_.Get() + new_size, Type{});
            }
        size_ = new_size;
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return items_.Get()+size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return items_.Get()+size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return items_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return items_.Get()+size_;
    }
    
    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        size_t new_size = size_ + 1;
        if (capacity_ < new_size){
            capacity_ = std::max(capacity_*2, new_size);
            auto new_items = ReallocateCopy(capacity_);
            new_items.Get()[size_] = item;
            items_.swap(new_items);
        } else{
            items_.Get()[size_] = item;
            }
        size_ = new_size;
    }
    
    void PushBack(Type && item){
        size_t new_size = size_ + 1;
        if (capacity_ < new_size){
            capacity_ = std::max(capacity_*2, new_size);
            auto new_items = ReallocateCopy(capacity_);
            new_items.Get()[size_] = std::move(item);
            items_.swap(new_items);
        } else {
            items_.Get()[size_] = std::move(item);
            }
        size_ = new_size;
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(begin() <= pos && pos <= end());
        size_t new_pos = pos - cbegin();
        size_t new_size = size_ + 1;
        if (new_size <= capacity_) {
            Iterator change_pos = begin() + new_pos;
            std::copy_backward(pos, cend(), end() + 1);
            *change_pos = value;
        } else {
            capacity_ = std::max(capacity_ * 2, new_size);
            ItemsPtr items(capacity_);
            Iterator items_pos = items.Get() + new_pos;
            std::copy(cbegin(), pos, items.Get());
            *items_pos = value;
            std::copy(pos, cend(), items_pos+1);
            items_.swap(items);
        }            
        size_ = new_size;
        return begin() + new_pos;
    }
    
    Iterator Insert(ConstIterator pos, Type&& value){
        assert(begin() <= pos && pos <= end());
        size_t new_pos = pos - cbegin();
        size_t new_size = size_ + 1;
        Iterator change_pos = begin() + new_pos;
        if (new_size <= capacity_){
            std::move_backward(change_pos, end(), end() + 1);
            *change_pos = std::move(value);
        } else {
            capacity_ = std::max(capacity_ * 2, new_size);
            ItemsPtr items(capacity_);
            Iterator items_pos = items.Get() + new_pos;
            std::move(begin(), change_pos, items.Get());
            *items_pos = std::move(value);
            std::move(change_pos, end(), items_pos + 1);
            items_.swap(items);
        }
        size_ = new_size;
        return begin() + new_pos;
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        assert(!IsEmpty());
        size_--;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(begin() <= pos && pos < end());
        Iterator change_pos = begin() + (pos - cbegin());
        std::move(change_pos + 1, end(), change_pos);
        --size_;
        return change_pos;
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        items_.swap(other.items_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }
        
private:
    ItemsPtr ReallocateCopy(size_t new_capacity) const {
        ItemsPtr new_items(new_capacity);
        size_t copy_size = std::min(new_capacity, size_);
        std::move(items_.Get(), items_.Get() + copy_size, new_items.Get());
        return ItemsPtr(new_items.Release());
    }
    
    ItemsPtr items_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (&lhs == &rhs) || (lhs.GetSize() == rhs.GetSize() && std::equal(lhs.begin(), lhs.end(), rhs.begin()));
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs==rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs<lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (rhs<lhs);
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs>rhs);
}
