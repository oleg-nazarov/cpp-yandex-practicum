#pragma once
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <utility>

template <typename Type>
class SinglyLinkedList {
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
            assert(iter_);

            return iter_->value;
        }

        [[nodiscard]] pointer operator->() const {
            assert(iter_);

            return &iter_->value;
        }

        BasicIterator& operator++() {
            assert(iter_);

            iter_ = iter_->next_node;

            return *this;
        }

        BasicIterator operator++(int) {
            BasicIterator<ValueType> copied_iter(iter_);

            ++(*this);

            return copied_iter;
        }

       private:
        friend class SinglyLinkedList;
        Node* iter_ = nullptr;
        explicit BasicIterator(Node* head) {
            iter_ = head;
        }
    };

   public:
    using Iterator = BasicIterator<Type>;
    using ConstIterator = BasicIterator<const Type>;

    SinglyLinkedList() : size_(0u) {}
    SinglyLinkedList(std::initializer_list<Type> other) {
        SinglyLinkedList temp;
        fill_temp_list(temp, other.begin(), other.end());

        this->swap(temp);
    }

    explicit SinglyLinkedList(const SinglyLinkedList& other) {
        SinglyLinkedList temp;
        fill_temp_list(temp, other.begin(), other.end());

        this->swap(temp);
    }

    ~SinglyLinkedList() {
        Clear();
    }

    SinglyLinkedList& operator=(const SinglyLinkedList& other) {
        if (this != &other) {
            SinglyLinkedList temp(other);
            this->swap(temp);
        }

        return *this;
    }

    bool operator==(const SinglyLinkedList& rhs) const noexcept {
        return std::equal(this->begin(), this->end(), rhs.begin());
    }

    bool operator!=(const SinglyLinkedList& rhs) const noexcept {
        return !(*this == rhs);
    }

    bool operator<(const SinglyLinkedList& rhs) const noexcept {
        return std::lexicographical_compare(this->begin(), this->end(), rhs.begin(), rhs.end());
    }

    bool operator>(const SinglyLinkedList& rhs) const noexcept {
        return rhs < *this;
    }

    bool operator<=(const SinglyLinkedList& rhs) const noexcept {
        return *this < rhs || *this == rhs;
    }

    bool operator>=(const SinglyLinkedList& rhs) const noexcept {
        return *this > rhs || *this == rhs;
    }

    void swap(SinglyLinkedList& rhs) noexcept {
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
        return cbegin();
    }

    ConstIterator end() const noexcept {
        return cend();
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
    void fill_temp_list(SinglyLinkedList& temp_list, Iterator other_begin, Iterator other_end) {
        Node* last_filled_node = &temp_list.head_;

        for (Iterator it = other_begin; it != other_end; ++it) {
            last_filled_node->next_node = new Node(*it, nullptr);
            last_filled_node = last_filled_node->next_node;

            ++temp_list.size_;
        }
    }
};

template <typename Type>
void swap(SinglyLinkedList<Type>& lhs, SinglyLinkedList<Type>& rhs) noexcept {
    lhs.swap(rhs);
}
