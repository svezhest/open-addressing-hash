#pragma once

#include <iostream>
#include "policy.h"
#include <memory>
#include <vector>

template<
        class Key,
        class T,
        class CollisionPolicy = LinearProbing,
        class Hash = std::hash<Key>,
        class Equal = std::equal_to<Key>
>
class HashMap {
public:
    using key_type = Key;
    using mapped_type = T;
    using value_type = std::pair<const Key, T>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using hasher = Hash;
    using key_equal = Equal;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;

private:
    CollisionPolicy probing{};

    struct Bucket {
        value_type value;
        bool is_deleted;

        template<class... Args>
        explicit Bucket(Args &&... args) : value(std::forward<Args>(args)...), is_deleted(false) {}
    };

    typedef typename std::vector<std::unique_ptr<Bucket>> container;
    container data;
    size_type el_count;
    size_type del_count;
    hasher hash_fn;
    key_equal equal_fn;


    class ConstIterator;

    class Iterator {
        friend class HashMap;

        friend class ConstIterator;

    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef std::pair<const Key, T> value_type;
        typedef ptrdiff_t difference_type;
        typedef value_type *pointer;
        typedef value_type &reference;


    private:
        const container *values = nullptr;
        size_type current{};
        size_type starting_pos{};
        bool iterator_end{};
        std::vector<size_type> element_order;
        bool is_ordered{};

    public:
        Iterator() : current(0),
                     starting_pos(0), iterator_end(true), is_ordered(false) {}

        Iterator(const Iterator &other) : values(other.values), current(other.current),
                                          starting_pos(other.starting_pos),
                                          iterator_end(other.iterator_end), is_ordered(false) {}

        Iterator(const ConstIterator &other) : values(other.values), current(other.current),
                                               starting_pos(other.starting_pos),
                                               iterator_end(other.iterator_end), is_ordered(false) {}

    private:
        explicit Iterator(const container *cont, size_type ind = 0) : values(cont), current(ind),
                                                                      starting_pos(ind),
                                                                      iterator_end(false), is_ordered(false) {
            if (current >= values->size()) {
                iterator_end = true;
            } else {
                while ((*values)[current].get() == nullptr || (*values)[current]->is_deleted) {
                    current = (current + 1) % values->size();
                    if (current == starting_pos) {
                        iterator_end = true;
                        break;
                    }
                }
            }
        }

        Iterator(const container *cont, std::vector<size_type> &&order_list) : values(cont),
                                                                               iterator_end(false),
                                                                               element_order(
                                                                                       std::move(order_list)),
                                                                               is_ordered(true) {
            operator++();
        }

    public:
        reference operator*() const {
            if (!iterator_end) {
                return ((*values)[current]->value);
            }
            throw std::out_of_range("Trying to access a not existing element in the map");
        }

        pointer operator->() const {
            if (!iterator_end) {
                return &((*values)[current]->value);
            }
            throw std::out_of_range("Trying to access a not existing element in the map");
        }

        Iterator &operator++() noexcept {
            if (!iterator_end) {
                if (is_ordered) {
                    if (element_order.empty()) {
                        iterator_end = true;
                    } else {
                        current = element_order.back();
                        element_order.pop_back();
                    }
                } else {
                    do {
                        current = (current + 1) % values->size();
                        if (current == starting_pos) {
                            iterator_end = true;
                            break;
                        }
                    } while ((*values)[current].get() == nullptr || (*values)[current]->is_deleted);
                }
            }
            return *this;
        }

        const Iterator operator++(int) noexcept {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        friend bool operator==(const Iterator &it1, const Iterator &it2) {
            return it1.iterator_end ? it2.iterator_end : !it2.iterator_end && it1.current == it2.current;
        }

        friend bool operator==(const ConstIterator &it1, const Iterator &it2) {
            return static_cast<ConstIterator>(it2) == it1;
        }

        friend bool operator==(const Iterator &it1, const ConstIterator &it2) {
            return static_cast<ConstIterator>(it1) == it2;
        }

        friend bool operator!=(const Iterator &it1, const Iterator &it2) {
            return !(it1 == it2);
        }

        friend bool operator!=(const ConstIterator &it1, const Iterator &it2) {
            return !(it1 == it2);
        }

        friend bool operator!=(const Iterator &it1, const ConstIterator &it2) {
            return !(it1 == it2);
        }
    };

    class ConstIterator {
        friend class HashMap;

        friend class Iterator;

    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef ptrdiff_t difference_type;
        typedef std::pair<const Key, T> value_type;
        typedef const value_type *pointer;
        typedef const value_type &reference;


    private:
        const container *values = nullptr;
        size_type current{};
        size_type starting_pos{};
        bool iterator_end{};
        std::vector<size_type> element_order;
        bool is_ordered{};

    public:
        ConstIterator() : current(0),
                          starting_pos(0), iterator_end(true), is_ordered(false) {}

        ConstIterator(const Iterator &other) : values(other.values), current(other.current),
                                               starting_pos(other.starting_pos),
                                               iterator_end(other.iterator_end), is_ordered(false) {}

        ConstIterator(const ConstIterator &other) : values(other.values), current(other.current),
                                                    starting_pos(other.starting_pos),
                                                    iterator_end(other.iterator_end), is_ordered(false) {}

    private:
        explicit ConstIterator(const container *cont, size_type ind = 0) : values(cont), current(ind),
                                                                           starting_pos(ind),
                                                                           iterator_end(false), is_ordered(false) {
            if (current >= values->size()) {
                iterator_end = true;
            } else {
                while ((*values)[current].get() == nullptr || (*values)[current]->is_deleted) {
                    current = (current + 1) % values->size();
                    if (current == starting_pos) {
                        iterator_end = true;
                        break;
                    }
                }
            }
        }

        ConstIterator(const container *cont, std::vector<size_type> &&order_list) : values(cont),
                                                                                    iterator_end(false),
                                                                                    element_order(
                                                                                            std::move(order_list)),
                                                                                    is_ordered(true) {
            operator++();
        }

    public:
        reference operator*() const {
            if (!iterator_end) {
                return (*values)[current]->value;
            }
            throw std::out_of_range("Trying to access a not existing element in the map");
        }

        pointer operator->() const {
            if (!iterator_end) {
                return &((*values)[current]->value);
            }
            throw std::out_of_range("Trying to access a not existing element in the map");
        }

        ConstIterator &operator++() noexcept {
            if (!iterator_end) {
                if (is_ordered) {
                    if (element_order.empty()) {
                        iterator_end = true;
                    } else {
                        current = element_order.back();
                        element_order.pop_back();
                    }
                } else {
                    do {
                        current = (current + 1) % values->size();
                        if (current == starting_pos) {
                            iterator_end = true;
                            break;
                        }
                    } while ((*values)[current].get() == nullptr || (*values)[current]->is_deleted);
                }
            }
            return *this;
        }

        const ConstIterator operator++(int) noexcept {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        friend bool operator==(const ConstIterator &it1, const ConstIterator &it2) {
            return it1.iterator_end ? it2.iterator_end : !it2.iterator_end && it1.current == it2.current;
        }

        friend bool operator!=(const ConstIterator &it1, const ConstIterator &it2) {
            return !(it1 == it2);
        }
    };

public:
    using iterator = Iterator;
    using const_iterator = ConstIterator;

    explicit HashMap(size_type expected_max_size = 0,
                     const hasher &hash = hasher(),
                     const key_equal &equal = key_equal()) : data(),
                                                             el_count(0),
                                                             del_count(0), hash_fn(hash), equal_fn(equal) {
        for (size_type i = 0; i < expected_max_size + 1; ++i) {
            data.push_back(nullptr);
        }
    }

    template<class InputIt>
    HashMap(InputIt first, InputIt last,
            size_type expected_max_size = 0,
            const hasher &hash = hasher(),
            const key_equal &equal = key_equal()) {
        HashMap(expected_max_size, hash, equal);
        insert(first, last);
    }

    HashMap(const HashMap &other) {
        hash_fn = other.hash_fn;
        equal_fn = other.equal_fn;
        el_count = other.el_count;
        del_count = other.del_count;
        data.clear();
        for (size_type i = 0; i < other.bucket_count(); ++i) {
            data.push_back(nullptr);
        }
        for (auto it = other.cbegin(); it != other.cend(); ++it) {
            data[it.current] = std::make_unique<Bucket>(*it);
        }
    }

    HashMap(HashMap &&other) {
        el_count = std::move(other.el_count);
        del_count = 0;
        hash_fn = std::move(other.hash_fn);
        equal_fn = std::move(other.equal_fn);
        data.clear();
        for (size_type i = 0; i < other.bucket_count(); ++i) {
            data.push_back(nullptr);
        }
        for (size_type i = 0; i < other.bucket_count(); ++i) {
            if (other.data[i].get() != nullptr && !other.data[i]->is_deleted) {
                data[i] = std::move(other.data[i]);
            }
        }
    }

    HashMap(std::initializer_list<value_type> init,
            size_type expected_max_size = 0,
            const hasher &hash = hasher(),
            const key_equal &equal = key_equal()) {
        HashMap(expected_max_size, hash, equal);
        insert(init);
    }

    HashMap &operator=(const HashMap &other) {
        hash_fn = other.hash_fn;
        equal_fn = other.equal_fn;
        el_count = other.el_count;
        del_count = other.del_count;
        data.clear();
        for (size_type i = 0; i < other.bucket_count(); ++i) {
            data.push_back(nullptr);
        }
        for (auto it = other.cbegin(); it != other.cend(); ++it) {
            data[it.current] = std::make_unique<Bucket>(*it);
        }
        return *this;
    }

    HashMap &operator=(HashMap &&other) noexcept {
        el_count = std::move(other.el_count);
        del_count = 0;
        hash_fn = std::move(other.hash_fn);
        equal_fn = std::move(other.equal_fn);
        data.clear();
        for (size_type i = 0; i < other.bucket_count(); ++i) {
            data.push_back(nullptr);
        }
        for (size_type i = 0; i < other.bucket_count(); ++i) {
            if (other.data[i].get() != nullptr && !other.data[i]->is_deleted) {
                data[i] = std::move(other.data[i]);
            }
        }
        return *this;
    }

    HashMap &operator=(std::initializer_list<value_type> init) {
        clear();
        insert(init);
        return *this;
    }

    void swap(HashMap &&other) noexcept {
        std::swap(data, other.data);
        std::swap(equal_fn, other.equal_fn);
        std::swap(hash_fn, other.hash_fn);
        std::swap(el_count, other.el_count);
        std::swap(del_count, other.del_count);
    }

    iterator begin() noexcept {
        return iterator(&data);
    }

    const_iterator begin() const noexcept {
        return cbegin();
    }

    const_iterator cbegin() const noexcept {
        return const_iterator(&data);
    }

    iterator end() noexcept {
        return iterator();
    }

    const_iterator end() const noexcept {
        return cend();
    }

    const_iterator cend() const noexcept {
        return const_iterator();
    }

private:
    void force_insert_ptr(std::unique_ptr<Bucket> ptr) {
        reserve(size() + 1);
        while (true) {
            size_type ind = bucket(ptr->value.first);
            probing.start();
            size_type cur = ind;
            for (size_type i = 0; i < bucket_count() / 2; ++i) {
                if (data[cur].get() == nullptr) {
                    ++el_count;
                } else if (equal_fn(data[cur]->value.first, ptr->value.first)) {
                    if (data[cur]->is_deleted) {
                        --del_count;
                        ++el_count;
                    }
                } else {
                    cur = (ind + probing.next()) % bucket_count();
                    continue;
                }
                data[cur] = std::move(ptr);
                return;
            }
            rehash(bucket_count() * 3);
        }
    }

public:
    std::pair<iterator, bool> insert(const value_type &inserted_value) {
        return emplace(inserted_value);
    }

    std::pair<iterator, bool> insert(value_type &&inserted_value) {
        return emplace(std::move(inserted_value));
    }

    template<class P>
    std::pair<iterator, bool> insert(P &&inserted_value) {
        return emplace(std::forward<P>(inserted_value));
    }

    iterator insert(const_iterator hint, const value_type &inserted_value) {
        return emplace_hint(hint, inserted_value);
    }

    iterator insert(const_iterator hint, value_type &&inserted_value) {
        return emplace_hint(hint, std::move(inserted_value));
    }

    template<class P>
    iterator insert(const_iterator hint, P &&inserted_value) {
        return emplace(hint, std::forward<P>(inserted_value));
    }

    template<class InputIt>
    void insert(InputIt first, InputIt last) {
        for (auto it = first; it != last; ++it) {
            insert(*it);
        }
    }

    void insert(std::initializer_list<value_type> init) {
        for (auto it : init) {
            insert(*it);
        }
    }

    template<class M>
    std::pair<iterator, bool> insert_or_assign(const key_type &key, M &&inserted_value) {
        reserve(size() + 1);
        while (true) {
            size_type ind = bucket(key);
            probing.start();
            size_type cur = ind;
            for (size_type i = 0; i < bucket_count() / 2; ++i) {
                if (data[cur].get() == nullptr) {
                    ++el_count;
                } else if (equal_fn(data[cur]->value.first, key)) {
                    if (data[cur]->is_deleted) {
                        --del_count;
                        ++el_count;
                    } else {
                        data[cur]->value.second = mapped_type(std::forward<M>(inserted_value));
                        return std::make_pair(iterator(&data, cur), false);
                    }
                } else {
                    cur = (ind + probing.next()) % bucket_count();
                    continue;
                }
                data[cur] = std::make_unique<Bucket>(key, std::forward<M>(inserted_value));
                return std::make_pair(iterator(&data, cur), true);
            }
            rehash(bucket_count() * 3);
        }
    }

    template<class M>
    std::pair<iterator, bool> insert_or_assign(key_type &&key, M &&inserted_value) {
        reserve(size() + 1);
        while (true) {
            size_type ind = bucket(key);
            probing.start();
            size_type cur = ind;
            for (size_type i = 0; i < bucket_count() / 2; ++i) {
                if (data[cur].get() == nullptr) {
                    ++el_count;
                } else if (equal_fn(data[cur]->value.first, key)) {
                    if (data[cur]->is_deleted) {
                        --del_count;
                        ++el_count;
                    } else {
                        data[cur]->value.second = mapped_type(std::forward<M>(inserted_value));
                        return std::make_pair(iterator(&data, cur), false);
                    }
                } else {
                    cur = (ind + probing.next()) % bucket_count();
                    continue;
                }
                data[cur] = std::make_unique<Bucket>(std::move(key), std::forward<M>(inserted_value));
                return std::make_pair(iterator(&data, cur), true);
            }
            rehash(bucket_count() * 3);
        }
    }

    template<class M>
    iterator insert_or_assign(const_iterator hint, const key_type &key, M &&inserted_value) {
        if (hint != cend()) {
            auto t = data[hint.current].get();
            if (t != nullptr && equal_fn(t->value.first, key)) {
                if (t->is_deleted) {
                    t->is_deleted = false;
                    --del_count;
                    ++el_count;
                }
                t->value.second = mapped_type(std::forward<M>(inserted_value));
                return hint;
            }
        }
        reserve(size() + 1);
        while (true) {
            size_type ind = bucket(key);
            probing.start();
            size_type cur = ind;
            for (size_type i = 0; i < bucket_count() / 2; ++i) {
                if (data[cur].get() == nullptr) {
                    ++el_count;
                } else if (equal_fn(data[cur]->value.first, key)) {
                    if (data[cur]->is_deleted) {
                        --del_count;
                        ++el_count;
                    } else {
                        data[cur]->value.second = mapped_type(std::forward<M>(inserted_value));
                        return iterator(&data, cur);
                    }
                } else {
                    cur = (ind + probing.next()) % bucket_count();
                    continue;
                }
                data[cur] = std::make_unique<Bucket>(key, std::forward<M>(inserted_value));
                return iterator(&data, cur);
            }
            rehash(bucket_count() * 3);
        }
    }

    template<class M>
    iterator insert_or_assign(const_iterator hint, key_type &&key, M &&inserted_value) {
        if (hint != cend()) {
            auto t = data[hint.current].get();
            if (t != nullptr && equal_fn(t->value.first, key)) {
                if (t->is_deleted) {
                    t->is_deleted = false;
                    --del_count;
                    ++el_count;
                }
                t->value.second = mapped_type(std::forward<M>(inserted_value));
                return hint;
            }
        }
        reserve(size() + 1);
        while (true) {
            size_type ind = bucket(key);
            probing.start();
            size_type cur = ind;
            for (size_type i = 0; i < bucket_count() / 2; ++i) {
                if (data[cur].get() == nullptr) {
                    ++el_count;
                } else if (equal_fn(data[cur]->value.first, key)) {
                    if (data[cur]->is_deleted) {
                        --del_count;
                        ++el_count;
                    } else {
                        data[cur]->value.second = mapped_type(std::forward<M>(inserted_value));
                        return iterator(&data, cur);
                    }
                } else {
                    cur = (ind + probing.next()) % bucket_count();
                    continue;
                }
                data[cur] = std::make_unique<Bucket>(std::move(key), std::forward<M>(inserted_value));
                return iterator(&data, cur);
            }
            rehash(bucket_count() * 3);
        }
    }

    template<class... Args>
    std::pair<iterator, bool> emplace(Args &&... args) {
        auto link = std::make_unique<Bucket>(std::forward<Args>(args)...);
        reserve(size() + 1);
        while (true) {
            size_type ind = bucket(link->value.first);
            probing.start();
            size_type cur = ind;
            for (size_type i = 0; i < bucket_count() / 2; ++i) {
                if (data[cur].get() == nullptr) {
                    ++el_count;
                } else if (equal_fn(data[cur]->value.first, link->value.first)) {
                    if (data[cur]->is_deleted) {
                        --del_count;
                        ++el_count;
                    } else {
                        return std::make_pair(iterator(&data, cur), false);
                    }
                } else {
                    cur = (ind + probing.next()) % bucket_count();
                    continue;
                }
                data[cur] = std::move(link);
                return std::make_pair(iterator(&data, cur), true);
            }
            rehash(bucket_count() * 3);
        }
    }

    template<class... Args>
    iterator emplace_hint(const_iterator hint, Args &&... args) {
        auto link = std::make_unique<Bucket>(std::forward<Args>(args)...);
        if (hint != cend()) {
            auto t = data[hint.current].get();
            if (t != nullptr && equal_fn(t->value.first, link->value.first)) {
                if (t->is_deleted) {
                    --del_count;
                    ++el_count;
                    data[hint.current] = std::move(link);
                }
                return hint;
            }
        }
        reserve(size() + 1);
        while (true) {
            size_type ind = bucket(link->value.first);
            probing.start();
            size_type cur = ind;
            for (size_type i = 0; i < bucket_count() / 2; ++i) {
                if (data[cur].get() == nullptr) {
                    ++el_count;
                } else if (equal_fn(data[cur]->value.first, link->value.first)) {
                    if (data[cur]->is_deleted) {
                        --del_count;
                        ++el_count;
                    } else {
                        return iterator(&data, cur);
                    }
                } else {
                    cur = (ind + probing.next()) % bucket_count();
                    continue;
                }
                data[cur] = std::move(link);
                return iterator(&data, cur);
            }
            rehash(bucket_count() * 3);
        }
    }

    template<class... Args>
    std::pair<iterator, bool> try_emplace(const key_type &key, Args &&... args) {
        reserve(size() + 1);
        while (true) {
            size_type ind = bucket(key);
            probing.start();
            size_type cur = ind;
            for (size_type i = 0; i < bucket_count() / 2; ++i) {
                if (data[cur].get() == nullptr) {
                    ++el_count;
                } else if (equal_fn(data[cur]->value.first, key)) {
                    if (data[cur]->is_deleted) {
                        --del_count;
                        ++el_count;
                    } else {
                        return std::make_pair(iterator(&data, cur), false);
                    }
                } else {
                    cur = (ind + probing.next()) % bucket_count();
                    continue;
                }
                data[cur] = std::make_unique<Bucket>(std::piecewise_construct,
                                                     std::forward_as_tuple(key),
                                                     std::forward_as_tuple(std::forward<Args>(args)...));
                return std::make_pair(iterator(&data, cur), true);
            }
            rehash(bucket_count() * 3);
        }
    }

    template<class... Args>
    std::pair<iterator, bool> try_emplace(key_type &&key, Args &&... args) {
        reserve(size() + 1);
        while (true) {
            size_type ind = bucket(key);
            probing.start();
            size_type cur = ind;
            for (size_type i = 0; i < bucket_count() / 2; ++i) {
                if (data[cur].get() == nullptr) {
                    ++el_count;
                } else if (equal_fn(data[cur]->value.first, key)) {
                    if (data[cur]->is_deleted) {
                        --del_count;
                        ++el_count;
                    } else {
                        return std::make_pair(iterator(&data, cur), false);
                    }
                } else {
                    cur = (ind + probing.next()) % bucket_count();
                    continue;
                }
                data[cur] = std::make_unique<Bucket>(std::piecewise_construct,
                                                     std::forward_as_tuple(std::move(key)),
                                                     std::forward_as_tuple(std::forward<Args>(args)...));
                return std::make_pair(iterator(&data, cur), true);
            }
            rehash(bucket_count() * 3);
        }
    }

    template<class... Args>
    iterator try_emplace(const_iterator hint, const key_type &key, Args &&... args) {
        if (hint != cend()) {
            auto t = data[hint.current].get();
            if (t != nullptr && equal_fn(t->value.first, key)) {
                if (t->is_deleted) {
                    t->is_deleted = false;
                    --del_count;
                    ++el_count;
                    t->value.second = mapped_typ(std::forward<Args>(args)...);
                }
                return hint;
            }
        }
        reserve(size() + 1);
        while (true) {
            size_type ind = bucket(key);
            probing.start();
            size_type cur = ind;
            for (size_type i = 0; i < bucket_count() / 2; ++i) {
                if (data[cur].get() == nullptr) {
                    ++el_count;
                } else if (equal_fn(data[cur]->value.first, key)) {
                    if (data[cur]->is_deleted) {
                        --del_count;
                        ++el_count;
                    } else {
                        return iterator(&data, cur);
                    }
                } else {
                    cur = (ind + probing.next()) % bucket_count();
                    continue;
                }
                data[cur] = std::make_unique<Bucket>(std::piecewise_construct,
                                                     std::forward_as_tuple(key),
                                                     std::forward_as_tuple(std::forward<Args>(args)...));
                return iterator(&data, cur);
            }
            rehash(bucket_count() * 3);
        }
    }

    template<class... Args>
    iterator try_emplace(const_iterator hint, key_type &&key, Args &&... args) {
        if (hint != cend()) {
            auto t = data[hint.current].get();
            if (t != nullptr && equal_fn(t->value.first, key)) {
                if (t->is_deleted) {
                    t->is_deleted = false;
                    --del_count;
                    ++el_count;
                    t->value.second = mapped_type(std::forward<Args>(args)...);
                }
                return hint;
            }
        }
        reserve(size() + 1);
        while (true) {
            size_type ind = bucket(key);
            probing.start();
            size_type cur = ind;
            for (size_type i = 0; i < bucket_count() / 2; ++i) {
                if (data[cur].get() == nullptr) {
                    ++el_count;
                } else if (equal_fn(data[cur]->value.first, key)) {
                    if (data[cur]->is_deleted) {
                        --del_count;
                        ++el_count;
                    } else {
                        return iterator(&data, cur);
                    }
                } else {
                    cur = (ind + probing.next()) % bucket_count();
                    continue;
                }
                data[cur] = std::make_unique<Bucket>(std::piecewise_construct,
                                                     std::forward_as_tuple(std::move(key)),
                                                     std::forward_as_tuple(std::forward<Args>(args)...));
                return iterator(&data, cur);
            }
            rehash(bucket_count() * 3);
        }
    }

    iterator erase(const_iterator pos) {
        if (pos == cend()) {
            return pos;
        }
        data[pos.current]->is_deleted = true;
        ++del_count;
        --el_count;
        return ++pos;
    }

    iterator erase(const_iterator first, const_iterator last) {
        iterator last_del = cend();
        for (auto it = first; it != last; ++it) {
            last_del = iterator(erase(it));
        }
        return last_del;
    }

    size_type erase(const key_type &key) {
        size_type counter = 0;
        for (auto it = find(key); it != end(); it = find(key)) {
            erase(it);
            ++counter;
        }
        return counter;
    }

    [[nodiscard]] size_type count(const key_type &key) const {
        return contains(key) ? 1 : 0;
    }

    [[nodiscard]] iterator find(const key_type &key) {
        CollisionPolicy probing_local{};
        size_type ind = bucket(key);
        probing_local.start();
        size_type cur = ind;
        for (size_type i = 0; i < bucket_count(); ++i) {
            if (data[cur].get() == nullptr) {
                return iterator();
            }
            if (equal_fn(data[cur]->value.first, key)) {
                if (data[cur]->is_deleted) {
                    return iterator();
                } else {
                    return iterator(&data, cur);
                }
            }
            cur = (ind + probing_local.next()) % bucket_count();
        }
        return iterator();
    }

    [[nodiscard]] const_iterator find(const key_type &key) const {
        CollisionPolicy probing_local{};
        size_type ind = bucket(key);
        probing_local.start();
        size_type cur = ind;
        for (size_type i = 0; i < bucket_count(); ++i) {
            if (data[cur].get() == nullptr) {
                return const_iterator();
            }
            if (equal_fn(data[cur]->value.first, key)) {
                if (data[cur]->is_deleted) {
                    return const_iterator();
                } else {
                    return const_iterator(&data, cur);
                }
            }
            cur = (ind + probing_local.next()) % bucket_count();
        }
        return const_iterator();
    }

    [[nodiscard]] bool contains(const key_type &key) const {
        return find(key) != cend();
    }

    std::pair<iterator, iterator> equal_range(const key_type &key) {
        std::vector<size_type> order_list;
        for (auto it = begin(); it != end(); it++) {
            if (equal_fn(key, (*it).first)) {
                order_list.push_back(it.current);
            }
        }
        return {iterator(&data, std::move(order_list)), iterator()};
    }

    std::pair<const_iterator, const_iterator> equal_range(const key_type &key) const {
        std::vector<size_type> order_list;
        for (auto it = begin(); it != end(); it++) {
            if (equal_fn(key, (*it).first)) {
                order_list.push_back(it.current);
            }
        }
        return {const_iterator(&data, std::move(order_list)), const_iterator()};
    }

    mapped_type &at(const key_type &key) {
        return (*(static_cast<iterator>(find(key)))).second;
    }

    const mapped_type &at(const key_type &key) const {
        return (*find(key)).second;
    }


    mapped_type &operator[](const key_type &key) {
        reserve(size() + 1);
        while (true) {
            size_type ind = bucket(key);
            probing.start();
            size_type cur = ind;
            for (size_type i = 0; i < bucket_count() / 2; ++i) {
                if (data[cur].get() == nullptr) {
                    ++el_count;
                } else if (equal_fn(data[cur]->value.first, key)) {
                    if (data[cur]->is_deleted) {
                        --del_count;
                        ++el_count;
                    } else {
                        return data[cur]->value.second;
                    }
                } else {
                    cur = (ind + probing.next()) % bucket_count();
                    continue;
                }
                data[cur] = std::make_unique<Bucket>(key, mapped_type());
                return data[cur]->value.second;
            }
            rehash(bucket_count() * 3);
        }
    }

    mapped_type &operator[](key_type &&key) {
        reserve(size() + 1);
        while (true) {
            size_type ind = bucket(key);
            probing.start();
            size_type cur = ind;
            for (size_type i = 0; i < bucket_count() / 2; ++i) {
                if (data[cur].get() == nullptr) {
                    ++el_count;
                } else if (equal_fn(data[cur]->value.first, key)) {
                    if (data[cur]->is_deleted) {
                        --del_count;
                        ++el_count;
                    } else {
                        return data[cur]->value.second;
                    }
                } else {
                    cur = (ind + probing.next()) % bucket_count();
                    continue;
                }
                data[cur] = std::make_unique<Bucket>(std::piecewise_construct,
                                                     std::forward_as_tuple(std::move(key)),
                                                     std::forward_as_tuple());
                return data[cur]->value.second;
            }
            rehash(bucket_count() * 3);
        }
    }

    void clear() noexcept {
        data.clear();
        el_count = 0;
        del_count = 0;
    }

    [[nodiscard]] size_type bucket_size(const size_type) const noexcept {
        return 1;
    }

    [[nodiscard]] size_type size() const noexcept {
        return el_count;
    }

    [[nodiscard]] bool empty() const noexcept {
        return size() == 0;
    }

    [[nodiscard]] size_type bucket_count() const noexcept {
        return data.size();
    }

    [[nodiscard]] size_type max_bucket_count() const noexcept {
        return data.max_size();
    }

    [[nodiscard]] size_type max_size() const noexcept {
        return max_bucket_count();
    }

    [[nodiscard]] size_type bucket(const key_type &key) const {
        return hash_fn(key) % bucket_count();
    }

    [[nodiscard]] float load_factor() const {
        return static_cast<float>(size()) / bucket_count();
    }

    [[nodiscard]] float max_load_factor() const noexcept {
        return 1;
    }

    void rehash(const size_type count) {
        if (count > bucket_count() || del_count > size()) {
            HashMap t = HashMap(std::max(bucket_count(), count), hash_fn, equal_fn);
            for (auto it = begin(); it != end(); ++it) {
                t.force_insert_ptr(std::move(data[it.current]));
            }
            operator=(std::move(t));
        }
    }

    void reserve(size_type count) {
        rehash(count);
    }

    friend bool operator==(const HashMap &first, const HashMap &second) {
        for (auto it = first.cbegin(); it != first.cend(); it++) {
            auto el = second.find((*it).first);
            if (el == second.cend() || (*el) != (*it)) {
                return false;
            }
        }
        for (auto it = second.cbegin(); it != second.cend(); it++) {
            auto el = first.find((*it).first);
            if (el == first.cend() || (*el) != (*it)) {
                return false;
            }
        }
        return true;
    }

    friend bool operator!=(const HashMap &first, const HashMap &second) {
        return !(first == second);
    }
};