#pragma once

#include <map>
#include <mutex>
#include <memory>
#include <string>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include "platform.h"

#include <type_traits>
template <typename T, typename = int> struct HasName : std::false_type {};
template <typename T> struct HasName <T, decltype((void) T::name, 0)> : std::true_type {};

template <typename T> class NamedHandle {
public:
    // "default" construct
    NamedHandle() {}

    // create new object and store handle in map for later retrieval
    template <class... Args> NamedHandle(const std::string& name, Args&&... args) : ptr(std::make_shared<T>(name, args...)) {
        static_assert(HasName<T>::value, "Template type T is required to have a member \"name\"!");
        static_assert(std::is_same<decltype(T::name), std::string>::value || std::is_same<decltype(T::name), const std::string>::value, "bad type bro");
        assert(!map.count(ptr->name)); // check if key unique in NamedHandle<T>::map
        const std::lock_guard<std::mutex> lock(mutex);
        map[ptr->name] = *this;
    }

    virtual ~NamedHandle() {}

    // copy / move
    NamedHandle(const NamedHandle<T>& other) = default;
    NamedHandle(NamedHandle<T>&& other) = default;
    inline NamedHandle<T>& operator=(const NamedHandle<T>& other) = default;
    inline NamedHandle<T>& operator=(NamedHandle<T>&& other) = default;

    // operators for pointer-like usage
    inline explicit operator bool() const { return ptr.operator bool(); }
    inline T* operator->() { return ptr.operator->(); }
    inline const T* operator->() const { return ptr.operator->(); }
    inline T& operator*() { return *ptr; }
    inline const T& operator*() const { return *ptr; }

    // check if mapping for given name exists
    static bool valid(const std::string& name) {
        const std::lock_guard<std::mutex> lock(mutex);
        return map.count(name);
    }
    // return mapped handle for given name
    static NamedHandle<T> find(const std::string& name) {
        const std::lock_guard<std::mutex> lock(mutex);
        return map[name];
    }
    // remove element from map for given name
    static void erase(const std::string& name) {
        map.erase(name);
    }
    // clear saved handles and free unsused memory
    static void clear() {
        const std::lock_guard<std::mutex> lock(mutex);
        map.clear();
    }

    // iterators to iterate over all entries
    static typename std::map<std::string, NamedHandle<T>>::iterator begin() { return map.begin(); }
    static typename std::map<std::string, NamedHandle<T>>::iterator end() { return map.end(); }

    std::shared_ptr<T> ptr;
    static std::mutex mutex;
    static std::map<std::string, NamedHandle<T>> map;
};

// definition of static members (compiler magic)
template <typename T> std::mutex NamedHandle<T>::mutex;
template <typename T> std::map<std::string, NamedHandle<T>> NamedHandle<T>::map;
