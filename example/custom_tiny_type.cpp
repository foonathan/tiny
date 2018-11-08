// This example demonstrates how to write a tiny type.
// Required reading: example `tiny_storage.cpp`

#include <iostream>

#include <foonathan/tiny/tiny_bool.hpp>    // for `tiny::tiny_bool`
#include <foonathan/tiny/tiny_storage.hpp> // for `tiny::tiny_storage`
#include <foonathan/tiny/tiny_type.hpp>    // for writing a tiny type

namespace tiny = foonathan::tiny;

// The type we want to turn into a tiny type.
// It is basically a bool with one additional member function, `flip()`.
class my_bool
{
public:
    explicit my_bool(bool b) : impl_(b) {}

    explicit operator bool() const noexcept
    {
        return impl_;
    }

    void flip() noexcept
    {
        impl_ = !impl_;
    }

private:
    bool impl_;
};

// So we write a `tiny_my_bool` type.
// This type represents a `my_bool` object that is stored in a space-efficient way.
// We always need both types because `tiny_my_bool` is not a real type.
struct tiny_my_bool
{
    // The proper type we can use normally.
    // This is the type that we're storing in a compressed way.
    using object_type = my_bool;

    static constexpr std::size_t bit_size() noexcept
    {
        // We only need one bit to store it.
        return 1;
    }

    // This is the proxy that acts like a reference to the `object_type`.
    // `BitView` is a view to the `bit_size()`-bits that store it.
    template <class BitView>
    class proxy
    {
        // Always need to store that view.
        BitView view_;

        // Required: initialize the view.
        explicit proxy(BitView view) noexcept : view_(view) {}

        // Make a friend so the constructor can be used.
        friend tiny::tiny_type_access;

    public:
        // Required: conversion to the `object_type`.
        operator object_type() const noexcept
        {
            // We get an integer that contain our bits.
            auto bits = view_.extract();
            // And then convert it to `my_bool`.
            return my_bool(bits != 0);
        }

        // Required: assignment from `object_type`.
        // Note that all functions on the proxy are `const`-qualified.
        // Proxies to `const` are determined by the `BitView` type.
        const proxy& operator=(object_type obj) const noexcept
        {
            // Just put the bits into the view.
            // If we have proxy to `const`, this will fail with a (nice) error message.
            view_.put(static_cast<bool>(obj) ? 1 : 0);
            return *this;
        }

        // The other functions are not required and just map to the corresponding function in the
        // object type.

        explicit operator bool() const noexcept
        {
            return static_cast<bool>(static_cast<object_type>(*this));
        }

        void flip() const noexcept
        {
            auto obj = static_cast<object_type>(*this);
            obj.flip();
            *this = obj;
        }
    };
};

void use_tiny_my_bool()
{
    std::cout << "=== tiny_my_bool ===\n\n";

    // Now we can use it just like any other tiny type.
    tiny::tiny_storage<tiny_my_bool, tiny::tiny_bool> storage;
    std::cout << "My bool: " << !!storage.at<0>() << '\n';
    std::cout << "Bool: " << !!storage.at<1>() << '\n';
    std::cout << '\n';

    storage.at<0>().flip();
    storage.at<1>() = true;

    std::cout << "My bool: " << !!storage.at<0>() << '\n';
    std::cout << "Bool: " << !!storage.at<1>() << '\n';
    std::cout << '\n';

    std::cout << '\n';
}

//=== run all examples ===//
int main()
{
    use_tiny_my_bool();
}
