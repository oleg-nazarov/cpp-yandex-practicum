#pragma once
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <string>
#include <utility>

template <typename Type>
class SingleLinkedList {
    struct Node {
        Node() = default;
        Node(const Type& val, Node* next) : value(val), next_node(next) {}

        Type value;
        Node* next_node = nullptr;
    };

    template <typename ValueType>
    class BasicIterator {
       public:
        using value_type = Type;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using pointer = ValueType*;
        using reference = ValueType&;

        BasicIterator() = default;
        BasicIterator(const BasicIterator<Type>& other) {
            iter_ = other.iter_;
        }

        BasicIterator& operator=(const BasicIterator& rhs) = default;

        [[nodiscard]] bool operator==(const BasicIterator<Type>& rhs) const noexcept {
            return iter_ == rhs.iter_;
        }

        [[nodiscard]] bool operator==(const BasicIterator<const Type>& rhs) const noexcept {
            return iter_ == rhs.iter_;
        }

        [[nodiscard]] bool operator!=(const BasicIterator<Type>& rhs) const noexcept {
            return iter_ != rhs.iter_;
        }

        [[nodiscard]] bool operator!=(const BasicIterator<const Type>& rhs) const noexcept {
            return iter_ != rhs.iter_;
        }

        [[nodiscard]] reference operator*() const {
            return iter_->value;
        }

        [[nodiscard]] pointer operator->() const {
            return &iter_->value;
        }

        BasicIterator& operator++() {
            if (iter_ != nullptr) {
                iter_ = iter_->next_node;
            }

            return *this;
        }

        BasicIterator operator++(int) {
            BasicIterator<ValueType> copied_iter(iter_);

            ++(*this);

            return copied_iter;
        }

       private:
        friend class SingleLinkedList;
        Node* iter_ = nullptr;
        explicit BasicIterator(Node* head) {
            iter_ = head;
        }
    };

   public:
    using Iterator = BasicIterator<Type>;
    using ConstIterator = BasicIterator<const Type>;

    SingleLinkedList() : size_(0u) {}
    SingleLinkedList(std::initializer_list<Type> other) {
        SingleLinkedList temp = get_temp(other.begin(), other.end());

        this->swap(temp);
    }

    SingleLinkedList(const SingleLinkedList& other) noexcept {
        SingleLinkedList temp = get_temp(other.begin(), other.end());

        this->swap(temp);
    }

    ~SingleLinkedList() {
        Clear();
    }

    SingleLinkedList& operator=(const SingleLinkedList& other) {
        SingleLinkedList temp = get_temp(other.begin(), other.end());

        this->swap(temp);

        return *this;
    }

    bool operator==(const SingleLinkedList& rhs) const noexcept {
        return std::equal(this->begin(), this->end(), rhs.begin());
    }

    bool operator!=(const SingleLinkedList& rhs) const noexcept {
        return !(*this == rhs);
    }

    bool operator<(const SingleLinkedList& rhs) const noexcept {
        return std::lexicographical_compare(this->begin(), this->end(), rhs.begin(), rhs.end());
    }

    bool operator>(const SingleLinkedList& rhs) const noexcept {
        return rhs < *this;
    }

    bool operator<=(const SingleLinkedList& rhs) const noexcept {
        return *this < rhs || *this == rhs;
    }

    bool operator>=(const SingleLinkedList& rhs) const noexcept {
        return *this > rhs || *this == rhs;
    }

    void swap(SingleLinkedList& rhs) noexcept {
        std::swap(this->head_.next_node, rhs.head_.next_node);
        std::swap(this->size_, rhs.size_);
    }

    Iterator begin() noexcept {
        return Iterator(head_.next_node);
    }

    Iterator end() noexcept {
        return Iterator(nullptr);
    }

    ConstIterator begin() const noexcept {
        return ConstIterator(head_.next_node);
    }

    ConstIterator end() const noexcept {
        return ConstIterator(nullptr);
    }

    ConstIterator cbegin() const noexcept {
        return ConstIterator(head_.next_node);
    }

    ConstIterator cend() const noexcept {
        return ConstIterator(nullptr);
    }

    Iterator before_begin() noexcept {
        return Iterator(&head_);
    }

    ConstIterator before_begin() const noexcept {
        return ConstIterator(&head_);
    }

    ConstIterator cbefore_begin() const noexcept {
        return ConstIterator(const_cast<Node*>(&head_));
    }

    [[nodiscard]] size_t GetSize() const noexcept {
        return size_;
    }

    [[nodiscard]] bool IsEmpty() const noexcept {
        return size_ == 0u;
    }

    Iterator InsertAfter(ConstIterator pos, const Type& value) {
        Node* new_node = new Node(value, nullptr);

        for (auto it = this->before_begin(); it != this->end(); ++it) {
            if (it == pos) {
                new_node->next_node = it.iter_->next_node;
                it.iter_->next_node = new_node;

                ++size_;

                break;
            }
        }

        return Iterator(new_node);
    }

    Iterator EraseAfter(ConstIterator pos) noexcept {
        Node* node_after = nullptr;

        for (auto it = this->before_begin(); it != this->end(); ++it) {
            if (it == pos) {
                Node* node_to_delete = it.iter_->next_node;
                it.iter_->next_node = it.iter_->next_node->next_node;
                delete node_to_delete;

                --size_;

                node_after = it.iter_->next_node;
                break;
            }
        }

        return Iterator(node_after);
    }

    void PushFront(const Type& value) {
        head_.next_node = new Node(value, head_.next_node);

        ++size_;
    }

    void PopFront() noexcept {
        if (head_.next_node != nullptr) {
            Node* node_to_delete = head_.next_node;
            head_.next_node = node_to_delete->next_node;
            delete node_to_delete;

            --size_;
        }
    }

    void Clear() noexcept {
        while (head_.next_node != nullptr) {
            Node* to_delete = head_.next_node;
            head_.next_node = to_delete->next_node;
            delete to_delete;

            --size_;
        }
    }

   private:
    Node head_;
    size_t size_;

    template <typename Iterator>
    SingleLinkedList get_temp(Iterator it_begin, Iterator it_end) {
        SingleLinkedList temp;

        Node* last_node = &temp.head_;
        while (it_begin != it_end) {
            last_node->next_node = new Node(*it_begin, nullptr);
            last_node = last_node->next_node;

            ++temp.size_;

            ++it_begin;
        }

        return temp;
    }
};

template <typename Type>
void swap(SingleLinkedList<Type>& lhs, SingleLinkedList<Type>& rhs) noexcept {
    lhs.swap(rhs);
}
