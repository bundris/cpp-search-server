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

// Тест
void Test4() {
    struct DeletionSpy {
        ~DeletionSpy() {
            if (deletion_counter_ptr) {
                ++(*deletion_counter_ptr);
            }
        }
        int* deletion_counter_ptr = nullptr;
    };

    // Проверка PopFront
    {
        SingleLinkedList<int> numbers{3, 14, 15, 92, 6};
        numbers.PopFront();
        assert((numbers == SingleLinkedList<int>{14, 15, 92, 6}));

        SingleLinkedList<DeletionSpy> list;
        list.PushFront(DeletionSpy{});
        int deletion_counter = 0;
        list.begin()->deletion_counter_ptr = &deletion_counter;
        assert(deletion_counter == 0);
        list.PopFront();
        assert(deletion_counter == 1);
    }

    // Доступ к позиции, предшествующей begin
    {
        SingleLinkedList<int> empty_list;
        const auto& const_empty_list = empty_list;
        assert(empty_list.before_begin() == empty_list.cbefore_begin());
        assert(++empty_list.before_begin() == empty_list.begin());
        assert(++empty_list.cbefore_begin() == const_empty_list.begin());

        SingleLinkedList<int> numbers{1, 2, 3, 4};
        const auto& const_numbers = numbers;
        assert(numbers.before_begin() == numbers.cbefore_begin());
        assert(++numbers.before_begin() == numbers.begin());
        assert(++numbers.cbefore_begin() == const_numbers.begin());
    }

    // Вставка элемента после указанной позиции
    {  // Вставка в пустой список
        {
            SingleLinkedList<int> lst;
            const auto inserted_item_pos = lst.InsertAfter(lst.before_begin(), 123);
            assert((lst == SingleLinkedList<int>{123}));
            assert(inserted_item_pos == lst.begin());
            assert(*inserted_item_pos == 123);
        }

        // Вставка в непустой список
        {
            SingleLinkedList<int> lst{1, 2, 3};
            auto inserted_item_pos = lst.InsertAfter(lst.before_begin(), 123);

            assert(inserted_item_pos == lst.begin());
            assert(inserted_item_pos != lst.end());
            assert(*inserted_item_pos == 123);
            assert((lst == SingleLinkedList<int>{123, 1, 2, 3}));

            inserted_item_pos = lst.InsertAfter(lst.begin(), 555);
            assert(++SingleLinkedList<int>::Iterator(lst.begin()) == inserted_item_pos);
            assert(*inserted_item_pos == 555);
            assert((lst == SingleLinkedList<int>{123, 555, 1, 2, 3}));
        };
    }

    // Вспомогательный класс, бросающий исключение после создания N-копии
    struct ThrowOnCopy {
        ThrowOnCopy() = default;
        explicit ThrowOnCopy(int& copy_counter) noexcept
                : countdown_ptr(&copy_counter) {
        }
        ThrowOnCopy(const ThrowOnCopy& other)
                : countdown_ptr(other.countdown_ptr)  //
        {
            if (countdown_ptr) {
                if (*countdown_ptr == 0) {
                    throw std::bad_alloc();
                } else {
                    --(*countdown_ptr);
                }
            }
        }
        // Присваивание элементов этого типа не требуется
        ThrowOnCopy& operator=(const ThrowOnCopy& rhs) = delete;
        // Адрес счётчика обратного отсчёта. Если не равен nullptr, то уменьшается при каждом копировании.
        // Как только обнулится, конструктор копирования выбросит исключение
        int* countdown_ptr = nullptr;
    };

    // Проверка обеспечения строгой гарантии безопасности исключений
    {
        bool exception_was_thrown = false;
        for (int max_copy_counter = 10; max_copy_counter >= 0; --max_copy_counter) {
            SingleLinkedList<ThrowOnCopy> list{ThrowOnCopy{}, ThrowOnCopy{}, ThrowOnCopy{}};
            try {
                int copy_counter = max_copy_counter;
                list.InsertAfter(list.cbegin(), ThrowOnCopy(copy_counter));
                assert(list.GetSize() == 4u);
            } catch (const std::bad_alloc&) {
                exception_was_thrown = true;
                assert(list.GetSize() == 3u);
                break;
            }
        }
        assert(exception_was_thrown);
    }

    // Удаление элементов после указанной позиции
    {
        {
            SingleLinkedList<int> lst{1, 2, 3, 4};
            const auto& const_lst = lst;
            const auto item_after_erased = lst.EraseAfter(const_lst.cbefore_begin());
            assert((lst == SingleLinkedList<int>{2, 3, 4}));
            assert(item_after_erased == lst.begin());
        }
        {
            SingleLinkedList<int> lst{1, 2, 3, 4};
            const auto item_after_erased = lst.EraseAfter(lst.cbegin());
            assert((lst == SingleLinkedList<int>{1, 3, 4}));
            assert(item_after_erased == (++lst.begin()));
        }
        {
            SingleLinkedList<int> lst{1, 2, 3, 4};
            const auto item_after_erased = lst.EraseAfter(++(++lst.cbegin()));
            assert((lst == SingleLinkedList<int>{1, 2, 3}));
            assert(item_after_erased == lst.end());
        }
        {
            SingleLinkedList<DeletionSpy> list{DeletionSpy{}, DeletionSpy{}, DeletionSpy{}};
            auto after_begin = ++list.begin();
            int deletion_counter = 0;
            after_begin->deletion_counter_ptr = &deletion_counter;
            assert(deletion_counter == 0u);
            list.EraseAfter(list.cbegin());
            assert(deletion_counter == 1u);
        }
    }
}

int main() {
    Test4();
}