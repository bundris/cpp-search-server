#pragma once
#include <cassert>
#include <cstddef>
#include <string>
#include <utility>
#include <iterator>

template <typename Type>
class SingleLinkedList {
    // Узел списка
    struct Node {
        Node() = default;
        Node(const Type& val, Node* next)
                : value(val)
                , next_node(next) {
        }
        Type value;
        Node* next_node = nullptr;
    };

    // класс итератора
    template <typename ValueType>
    class BasicIterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = Type;
        using difference_type = std::ptrdiff_t;
        using pointer = ValueType*;
        using reference = ValueType&;

        BasicIterator() = default;
        BasicIterator(const BasicIterator<Type>& other) noexcept {
            node_ = other.node_;
        }

        // перегрузка операторов сравнения
        BasicIterator& operator=(const BasicIterator& rhs) = default;
        [[nodiscard]] bool operator==(const BasicIterator<const Type>& rhs) const noexcept {
            return node_ == rhs.node_;
        }
        [[nodiscard]] bool operator!=(const BasicIterator<const Type>& rhs) const noexcept {
            return node_ != rhs.node_;
        }

        [[nodiscard]] bool operator==(const BasicIterator<Type>& rhs) const noexcept {
            return node_ == rhs.node_;
        }
        [[nodiscard]] bool operator!=(const BasicIterator<Type>& rhs) const noexcept {
            return node_ != rhs.node_;
        }

        // инкременты
        BasicIterator& operator++() noexcept {
            if (this->node_ != nullptr) {
                this->node_ = this->node_->next_node;
            }
            return *this;
        }
        BasicIterator operator++(int) noexcept {
            BasicIterator<ValueType> old(BasicIterator<ValueType>(*this));
            if (this->node_ != nullptr) {
                this->node_ = this->node_->next_node;
            }
            return old;
        }

        // доступ к данным через итератор
        [[nodiscard]] reference operator*() const noexcept {
            return node_->value;
        }
        [[nodiscard]] pointer operator->() const noexcept {
            return pointer(node_);
        }
    private:
        friend class SingleLinkedList;
        explicit BasicIterator(Node* node) {
            node_ = node;
        }
        Node* node_ = nullptr;
    };

    //SingleLinkedList public
public:
    using Iterator = BasicIterator<Type>;
    using ConstIterator = BasicIterator<const Type>;

    //конструкторы
    SingleLinkedList():
            size_(0){}
    SingleLinkedList(std::initializer_list<Type> values) {
        if (values.size() == 0){
            return;
        }
        SingleLinkedList<Type> tmp, tmp2;
        // заполняем tmp, получаем обратный порядок (тк добавляем в начало)
        for (auto it = values.begin(); it != values.end(); ++it) {
            tmp.PushFront(*it);
        }
        // заполняем tmp2, чтобы получить исходный порядок элементов
        for (auto it = tmp.begin(); it != tmp.end(); ++it) {
            tmp2.PushFront(*it);
        }
        swap(tmp2);
    }
    SingleLinkedList(const SingleLinkedList& other) {
        if (this == &other) {
            return;
        }
        SingleLinkedList<Type> tmp, tmp2;
        // заполняем tmp, добавляя элементы в начало (получаем обратный порядок)
        for (auto it = other.begin(); it != other.end(); ++it) {
            tmp.PushFront(*it);
        }
        // заполняем tmp2, чтобы восстановить исходный порядок элементов
        for (auto it = tmp.begin(); it != tmp.end(); ++it) {
            tmp2.PushFront(*it);
        }
        swap(tmp2);
    }

    // методы проверки размера списка
    [[nodiscard]] size_t GetSize() const noexcept {
        return size_;
    }
    [[nodiscard]] bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // методы изменения списка (вставка/удаление первого элемента или по итератору)
    void PushFront(const Type& val) {
        head_.next_node = new Node(val, head_.next_node);
        ++size_;
    }
    void Clear() noexcept {
        while (head_.next_node != nullptr) {
            Node* temp = head_.next_node;
            head_.next_node = temp->next_node;
            delete temp;
        }
        size_ = 0;
    }
    Iterator InsertAfter(ConstIterator pos, const Type& value) {
        auto after = pos.node_->next_node;
        auto t = new Node(value, after);
        pos.node_->next_node = t;
        ++size_;
        return Iterator{t};
    }
    void PopFront() noexcept {
        auto first_el = head_.next_node;
        if (first_el == nullptr) {
            return;
        }
        head_.next_node = first_el->next_node;
        delete first_el;
    }
    Iterator EraseAfter(ConstIterator pos) noexcept {
        auto after_pos = pos.node_->next_node; //берем pos + 1 элемент
        if (after_pos == nullptr) {
            return end();
        }
        auto next_in_seq = after_pos->next_node; // если pos + 1 есть, запоминаем следующую ноду
        pos.node_->next_node = next_in_seq; // ссылаемся с pos на найденную ноду (исключаем after_pos)
        delete after_pos;
        --size_;
        return Iterator{next_in_seq};
    }

    ~SingleLinkedList(){
        Clear();
    }

    // методы для получения итераторов начала и конца
    [[nodiscard]] Iterator before_begin() noexcept {
        return Iterator{&head_};
    }
    [[nodiscard]] ConstIterator cbefore_begin() const noexcept {
        return ConstIterator{const_cast<Node*>(&head_)};
    }
    [[nodiscard]] ConstIterator before_begin() const noexcept {
        auto temp = new Node(head_);
        return ConstIterator{temp};
    }
    [[nodiscard]] Iterator begin() noexcept {
        return Iterator{head_.next_node};
    }
    [[nodiscard]] Iterator end() noexcept {
        Node* temp = head_.next_node;
        if (temp != nullptr) {
            while (temp->next_node != nullptr) {
                temp = temp->next_node;
            }
            temp = temp->next_node;
        }
        return Iterator{temp};
    }
    [[nodiscard]] ConstIterator begin() const noexcept {
        return ConstIterator{head_.next_node};
    }
    [[nodiscard]] ConstIterator end() const noexcept {
        Node* temp = head_.next_node;
        if (temp != nullptr) {
            while (temp->next_node != nullptr) {
                temp = temp->next_node;
            }
            temp = temp->next_node;
        }
        return ConstIterator{temp};
    }
    [[nodiscard]] ConstIterator cbegin() const noexcept {
        auto temp = new Node(head_);
        return ConstIterator{temp->next_node};
    }
    [[nodiscard]] ConstIterator cend() const noexcept {
        Node* temp = new Node(head_);
        temp = temp->next_node;
        if (temp != nullptr) {
            while (temp->next_node != nullptr) {
                temp = temp->next_node;
            }
            temp = temp->next_node;
        }
        Node* const en(temp);
        delete temp;
        return ConstIterator{en};
    }

    // перегрузка оператора присваивания
    SingleLinkedList& operator=(const SingleLinkedList& rhs) {
        if (this == &rhs) {
            return *this;
        }
        auto temp(rhs);
        swap(temp);
        return *this;
    }

    // обмен местами с другим списком
    void swap(SingleLinkedList& other) noexcept {
        Node* temp = other.head_.next_node;
        other.head_.next_node = this->head_.next_node;
        this->head_.next_node = temp;
        size_t tempsize = other.size_;
        other.size_ = this->size_;
        this->size_ = tempsize;
    }

private:
    // Фиктивный узел, используется для вставки "перед первым элементом"
    Node head_;
    size_t size_;
};

template <typename Type>
void swap(SingleLinkedList<Type>& lhs, SingleLinkedList<Type>& rhs) noexcept {
    lhs.swap(rhs);
}

template <typename Type>
bool operator==(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename Type>
bool operator!=(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
bool operator<(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
bool operator<=(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
bool operator>(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
bool operator>=(const SingleLinkedList<Type>& lhs, const SingleLinkedList<Type>& rhs) {
    return !(rhs < lhs);
}
