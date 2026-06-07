#ifndef COMPTR_H
#define COMPTR_H

#include <cstddef>
template <class T> class ComPtr {
public:
    ComPtr() : p(nullptr) {}
    ComPtr(std::nullptr_t) noexcept : p(nullptr) {} // 允许从 nullptr 构造
    explicit ComPtr(T *p) : p(p) {
        if (p)
            p->AddRef();
    }
    ComPtr(const ComPtr &other) : p(other.p) {
        if (p)
            p->AddRef();
    }
    ~ComPtr() {
        if (p)
            p->Release();
    }

    ComPtr &operator=(const ComPtr &other) {
        if (this != &other) {
            if (p)
                p->Release();
            p = other.p;
            if (p)
                p->AddRef();
        }
        return *this;
    }

    ComPtr(ComPtr &&other) noexcept : p(other.p) { other.p = nullptr; }
    ComPtr &operator=(ComPtr &&other) noexcept {
        if (this != &other) {
            if (p)
                p->Release();
            p = other.p;
            other.p = nullptr;
        }
        return *this;
    }

    T *operator->() const { return p; }
    T *get() const { return p; }
    T **operator&() {
        if (p) {
            p->Release();
            p = nullptr;
        }
        return &p;
    }
    operator T *() const { return p; }
    bool operator!() const { return p == nullptr; }
    explicit operator bool() const { return p != nullptr; }

private:
    T *p;
};

#endif // COMPTR_H