#pragma once

#include <iostream>
#include "policy.h"
#include <memory>
#include <vector>

template<
        class Key,
        class CollisionPolicy = LinearProbing,
        class Hash = std::hash<Key>,
        class Equal = std::equal_to<Key>
>
class HashSet {
public:
    using key_type = Key;
    using value_type = Key;
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
        const Key key;
        bool is_deleted;

        explicit Bucket(const Key &k, bool b = false) : key(k), is_deleted(b) {}

        explicit Bucket(Key &&k, bool b = false) : key(std::move(k)), is_deleted(b) {}

        template<class... Args>
        explicit Bucket(Args &&... args) : key(std::forward<Args>(args)...), is_deleted(false) {}
    };

    typedef typename std::vector<std::unique_ptr<Bucket>> container;
    container data;
    size_type el_count;
    size_type del_count;
    hasher hash_fn;
    key_equal equal_fn;


    class Basic_Iterator {
        friend class HashSet;

    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef ptrdiff_t difference_type;
        typedef const Key value_type;
        typedef value_type *pointer;
        typedef value_type &reference;


    private:
        const container *values  = nullptr;
        size_type current{};
        size_type starting_pos{};
        bool iterator_end;
        std::vector<size_type> element_order;
        bool is_ordered{};

    public:
        Basic_Iterator() : iterator_end(true) {}

        Basic_Iterator(const Basic_Iterator &other) : values(other.values), current(other.current),
                                                      starting_pos(other.starting_pos),
                                                      iterator_end(other.iterator_end), is_ordered(false) {}

    private:
        explicit Basic_Iterator(const container *cont, size_type ind = 0) : values(cont), current(ind),
                                                                            starting_pos(ind),
                                                                            iterator_end(false), is_ordered(false) {
            if (current >= values->size()) {
                iterator_end = true;
            } else {
                while ((*values)[current].get() == nullptr || (*values)[current].get()->is_deleted) {
                    current = (current + 1) % values->size();
                    if (current == starting_pos) {
                        iterator_end = true;
                        break;
                    }
                }
            }
        }

        Basic_Iterator(const container *cont, std::vector<size_type> &&order_list) : values(cont),
                                                                                     iterator_end(false),
                                                                                     element_order(
                                                                                             std::move(order_list)),
                                                                                     is_ordered(true) {
            operator++();
        }

    public:
        reference operator*() const {
            if (!iterator_end) {
                return ((*values)[current].get()->key);
            }
            throw std::out_of_range("Trying to access a value of the end iterator");
        }

        pointer operator->() const {
            if (!iterator_end) {
                return &((*values)[current].get()->key);
            }
            throw std::out_of_range("Trying to access a value of the end iterator");
        }

        Basic_Iterator &operator++() noexcept {
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
                    } while ((*values)[current].get() == nullptr || (*values)[current].get()->is_deleted);
                }
            }
            return *this;
        }

        const Basic_Iterator operator++(int) noexcept {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        friend bool operator==(const Basic_Iterator &it1, const Basic_Iterator &it2) {
            return it1.iterator_end ? it2.iterator_end : !it2.iterator_end && it1.current == it2.current;
        }

        friend bool operator!=(const Basic_Iterator &it1, const Basic_Iterator &it2) {
            return !(it1 == it2);
        }
    };

public:
    using const_iterator = Basic_Iterator;
    using iterator = const_iterator;

    HashSet(size_type expected_max_size = 0,
                     const hasher &hash = hasher(),
                     const key_equal &equal = key_equal()) : data(),
                                                             el_count(0),
                                                             del_count(0), hash_fn(hash), equal_fn(equal) {
        for (size_type i = 0; i < expected_max_size + 1; ++i) {
            data.push_back(nullptr);
        }
    }

    template<class InputIt>
    HashSet(InputIt first, InputIt last,
            size_type expected_max_size = 0,
            const hasher &hash = hasher(),
            const key_equal &equal = key_equal()) {
        HashSet(expected_max_size, hash, equal);
        insert(first, last);
    }

    HashSet(const HashSet &other) {;
        hash_fn = other.hash_fn;
        equal_fn = other.equal_fn;
        el_count = other.el_count;
        del_count = 0;
        data.clear();
        for (size_type i = 0; i < other.bucket_count(); ++i) {
            data.push_back(nullptr);
        }
        for (auto it = other.cbegin(); it != other.cend(); ++it) {
            data[it.current] = std::make_unique<Bucket>(*it);
        }
    }

    HashSet(HashSet &&other) {
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

    HashSet(std::initializer_list<value_type> init,
            size_type expected_max_size = 0,
            const hasher &hash = hasher(),
            const key_equal &equal = key_equal()) {
        HashSet(expected_max_size, hash, equal);
        insert(init);
    }

    HashSet &operator=(const HashSet &other) {
        hash_fn = other.hash_fn;
        equal_fn = other.equal_fn;
        el_count = other.el_count;
        del_count = 0;
        data.clear();
        for (size_type i = 0; i < other.bucket_count(); ++i) {
            data.push_back(nullptr);
        }
        for (auto it = other.cbegin(); it != other.cend(); ++it) {
            data[it.current] = std::make_unique<Bucket>(*it);
        }
        return *this;
    }

    HashSet &operator=(HashSet &&other) noexcept {
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

    HashSet &operator=(std::initializer_list<value_type> init) {
        clear();
        insert(init);
        return *this;
    }

    void swap(HashSet &&other) noexcept {
        std::swap(data, other.data);
        std::swap(equal_fn, other.equal_fn);
        std::swap(hash_fn, other.hash_fn);
        std::swap(el_count, other.el_count);
        std::swap(del_count, other.del_count);
    }

    [[nodiscard]] const_iterator begin() const noexcept {
        return cbegin();
    }

    [[nodiscard]] const_iterator cbegin() const noexcept {
        return Basic_Iterator(&data);
    }

    [[nodiscard]] const_iterator end() const noexcept {
        return cend();
    }

    [[nodiscard]] const_iterator cend() const noexcept {
        return Basic_Iterator();
    }

private:
    void insert(std::unique_ptr<Bucket> ptr) {
        reserve(size() + 1);
        size_type ind = bucket(ptr.get()->key);
        probing.start();
        size_type cur = ind;
        for (size_type i = 0; i < bucket_count() / 2; ++i) {
            if (data[cur].get() == nullptr) {
                data[cur] = std::move(ptr);
                ++el_count;
            } else if (equal_fn(data[cur]->key, ptr.get()->key)) {
                if (data[cur]->is_deleted) {
                    data[cur]->is_deleted = false;
                    --del_count;
                    ++el_count;
                }
            } else {
                cur = (ind + probing.next()) % bucket_count();
                continue;
            }
            return;
        }
        rehash(bucket_count() * 3);
        insert(std::move(ptr));
    }

public:
    std::pair<iterator, bool> insert(const value_type &inserted_value) {
        reserve(size() + 1);
        size_type ind = bucket(inserted_value);
        probing.start();
        size_type cur = ind;
        for (size_type i = 0; i < bucket_count() / 2; ++i) {
            if (data[cur].get() == nullptr) {
                data[cur] = std::make_unique<Bucket>(inserted_value, false);
                ++el_count;
                return std::make_pair(Basic_Iterator(&data, cur), true);
            }
            if (equal_fn(data[cur]->key, inserted_value)) {
                if (data[cur]->is_deleted) {
                    data[cur]->is_deleted = false;
                    --del_count;
                    ++el_count;
                    return std::make_pair(Basic_Iterator(&data, cur), true);
                } else {
                    return std::make_pair(Basic_Iterator(&data, cur), false);
                }
            }
            cur = (ind + probing.next()) % bucket_count();
        }
        rehash(bucket_count() * 3);
        return insert(inserted_value);
    }

    std::pair<iterator, bool> insert(value_type &&key) {
        reserve(size() + 1);
        size_type ind = bucket(key);
        probing.start();
        size_type cur = ind;
        for (size_type i = 0; i < bucket_count() / 2; ++i) {
            if (data[cur].get() == nullptr) {
                data[cur] = std::make_unique<Bucket>(std::move(key), false);
                ++el_count;
                return std::make_pair(Basic_Iterator(&data, cur), true);
            }
            if (equal_fn(data[cur]->key, key)) {
                if (data[cur]->is_deleted) {
                    data[cur]->is_deleted = false;
                    --del_count;
                    ++el_count;
                    return std::make_pair(Basic_Iterator(&data, cur), true);
                } else {
                    return std::make_pair(Basic_Iterator(&data, cur), false);
                }
            }
            cur = (ind + probing.next()) % bucket_count();
        }
        rehash(bucket_count() * 3);
        return insert(std::move(key));
    }

    iterator insert(const_iterator hint, const value_type &key) {
        if (hint != cend()) {
            auto t = data[hint.current].get();
            if (t != nullptr && equal_fn(t->key, key)) {
                if (t->is_deleted) {
                    t->is_deleted = false;
                    --del_count;
                    ++el_count;
                }
                return hint;
            }
        }
        return insert(key).first;
    }

    iterator insert(const_iterator hint, value_type &&key) {
        if (hint != cend()) {
            auto t = data[hint.current].get();
            if (t != nullptr && equal_fn(t->key, key)) {
                if (t->is_deleted) {
                    t->is_deleted = false;
                    --del_count;
                    ++el_count;
                }
                return hint;
            }
        }
        return insert(std::move(key)).first;
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

    template<class... Args>
    std::pair<iterator, bool> emplace(Args &&... args) {
        auto link = std::make_unique<Bucket>(std::forward<Args>(args)...);
        reserve(size() + 1);
        while (true) {
            size_type ind = bucket(link->key);
            probing.start();
            size_type cur = ind;
            for (size_type i = 0; i < bucket_count() / 2; ++i) {
                if (data[cur].get() == nullptr) {
                    ++el_count;
                } else if (equal_fn(data[cur]->key, link->key)) {
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
            if (t != nullptr && equal_fn(t->key, link->key)) {
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
            size_type ind = bucket(link->key);
            probing.start();
            size_type cur = ind;
            for (size_type i = 0; i < bucket_count() / 2; ++i) {
                if (data[cur].get() == nullptr) {
                    ++el_count;
                } else if (equal_fn(data[cur]->key, link->key)) {
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
            last_del = const_iterator(erase(it));
        }
        return last_del;
    }

    size_type erase(const key_type &key) {
        size_type counter = 0;
        for (auto it = find(key); it != cend(); it = find(key)) {
            erase(it);
            ++counter;
        }
        return counter;
    }

    [[nodiscard]] size_type count(const key_type &key) const {
        return contains(key) ? 1 : 0;
    }

    [[nodiscard]] const_iterator find(const key_type &key) const {
        CollisionPolicy probing_local{};
        size_type ind = bucket(key);
        probing_local.start();
        size_type cur = ind;
        for (size_type i = 0; i < bucket_count(); ++i) {
            if (data[cur].get() == nullptr) {
                return cend();
            }
            if (equal_fn(data[cur]->key, key)) {
                if (data[cur]->is_deleted) {
                    return cend();
                } else {
                    return Basic_Iterator(&data, cur);
                }
            }
            cur = (ind + probing_local.next()) % bucket_count();
        }
        return cend();
    }

    [[nodiscard]] bool contains(const key_type &key) const {
        return find(key) != end();
    }

    std::pair<const_iterator, const_iterator> equal_range(const key_type &key) const {
        std::vector<size_type> order_list;
        for (auto it = cbegin(); it != cend(); it++) {
            if (equal_fn(key, *it)) {
                order_list.push_back(it.current);
            }
        }
        return {Basic_Iterator(&data, std::move(order_list)), Basic_Iterator()};
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
            HashSet t = HashSet(std::max(bucket_count(), count), hash_fn, equal_fn);
            for (auto it = begin(); it != end(); ++it) {
                t.insert(std::move(data[it.current]));
            }
            operator=(std::move(t));
        }
    }

    void reserve(size_type count) {
        rehash(count);
    }

    friend bool operator==(const HashSet &first, const HashSet &second) {
        for (auto it = first.begin(); it != first.end(); it++) {
            if (!second.contains(*it)) {
                return false;
            }
        }
        for (auto it = second.begin(); it != second.end(); it++) {
            if (!first.contains(*it)) {
                return false;
            }
        }
        return true;
    }

    friend bool operator!=(const HashSet &first, const HashSet &second) {
        return !(first == second);
    }
};
