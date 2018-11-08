// This example shows how to define tombstones for your type.

#include <cassert>
#include <iostream>

#include <foonathan/tiny/check_size.hpp>    // for `tiny::check_size`
#include <foonathan/tiny/optional_impl.hpp> // for `tiny::optional_impl`
#include <foonathan/tiny/tombstone.hpp>     // for `tiny::tombstone_traits`

namespace tiny = foonathan::tiny;

// Custom type: a pointer to `T` that is never null.
template <typename T>
class non_null_ptr
{
    T* ptr_;

public:
    explicit non_null_ptr(T* ptr) : ptr_(ptr)
    {
        assert(ptr);
    }

    T& operator*() const noexcept
    {
        return *ptr_;
    }

    T* operator->() const noexcept
    {
        return ptr_;
    }

    T* get() const noexcept
    {
        return ptr_;
    }
};

// To create tombstones we need to specialize the tombstone traits.
// Here we want to just have a single tombstone, `nullptr`.
// An optimized implementation would also leverage the alignment bits.
namespace foonathan
{
namespace tiny
{
#ifdef TOMBSTONE_MANUAL
    template <typename T>
    struct tombstone_traits<non_null_ptr<T>>
    {
        // This is the type that is being stored conceptually.
        // It is usually the type of the specialization, but for a tiny type it is the actual object
        // type, for example.
        using object_type = non_null_ptr<T>;

        // The type that can store the object type or a tombstone.
        // Here we have a union of `non_null_ptr<T>` and `T*`.
        // As the two types are layout compatible, we can do type punning through the union.
        //
        // This type is never copied but must be nothrow default constructible and trivially
        // destructible.
        union storage_type
        {
            non_null_ptr<T> object;
            T*              tombstone;

            // default constructor is required here
            // doesn't actually need to initialize anything
            storage_type() : tombstone() {}
        };

        // A reference or proxy to the stored object type.
        // Here we can use a reference.
        //
        // Be careful when you are not writing a reference,
        // as this is then propagated until an optional implementation, for example.
        // If you're not careful, this can create a `vector<bool>` situation in generic code.
        //
        // The provided specialization of the tombstone traits will have actual references here,
        // except for the tiny types.
        // But those are weird in generic code anyways.
        using reference       = non_null_ptr<T>&;
        using const_reference = const non_null_ptr<T>&;

        // The number of tombstones, here it is just one.
        static constexpr std::size_t tombstone_count = 1;

        // This function creates the tombstone with the specified index.
        // It is only called if nothing is stored in the storage.
        static void create_tombstone(storage_type& storage, std::size_t index) noexcept
        {
            // We have only one tombstone, so we can ignore the index.
            assert(index == 0);
            // Our only tombstone is `nullptr`
            storage.tombstone = nullptr;
        }

        // This function creates an object.
        // It is only called if nothing is stored in the storage.
        //
        // In reality it would be template forwarding a variadic number of arguments.
        // But we know all possible arguments here.
        static void create_object(storage_type& storage, T* arg)
        {
            // For types that aren't trivially copy constructible, this might involve placement new.
            storage.object = non_null_ptr<T>(arg);
        }

        // This function destroys an object.
        // There is no `destroy_tombstone()` as tombstones must be trivial types.
        static void destroy_object(storage_type& storage) noexcept
        {
            // The object type is trivial, so no need to do anything.
            (void)storage;
        }

        // This function returns the index of the tombstone,
        // or an out of bounds index if it stores an object.
        static std::size_t get_tombstone(const storage_type& storage) noexcept
        {
            // Type punning here is legal, so just check whether we have `nullptr`.
            if (storage.tombstone == nullptr)
                return 0; // Tombstone with index 0 (and only tombstone).
            else
                return 1; // No tombstone, return an out-of-bounds index.
        }

        // These functions return the object that is being stored.
        static reference get_object(storage_type& storage) noexcept
        {
            return storage.object;
        }
        static const_reference get_object(const storage_type& storage) noexcept
        {
            return storage.object;
        }
    };
#else
    // The pattern where you have a custom type that can store the object plus tombstone and you do
    // type punning is common, so the boilerplate can be eliminated.
    template <typename T>
    struct tombstone_traits<non_null_ptr<T>>
    // Inherit from `tiny::tombstone_traits_simple`,
    // the second type is the `TombstoneType` — the trivial type that is type punned.
    : tiny::tombstone_traits_simple<non_null_ptr<T>, T*>
    {
        // Still one tombstone.
        static constexpr std::size_t tombstone_count = 1;

        // This function creates the specified `TombstoneType` object.
        static void create_tombstone_impl(void* memory, std::size_t index) noexcept
        {
            // Again, we can ignore the index.
            assert(index == 0);
            ::new (memory) T*(nullptr);
        }

        // This function returns the index by passing it the `TombstoneType` object.
        static std::size_t get_tombstone_impl(T* tombstone) noexcept
        {
            // Same implementation as before.
            return tombstone == nullptr ? 0 : 1;
        }
    };
#endif
} // namespace tiny
} // namespace foonathan

int main()
{
    // Just using `tiny::optional_impl`  for simplicity here,
    // the type isn't meant to be used directly!
    using optional = tiny::optional_impl<non_null_ptr<int>>;
    static_assert(tiny::check_size<optional, sizeof(int*)>(),
                  "no additional storage due to the tombstone");

    optional opt;
    std::cout << "optional is " << (opt.has_value() ? "not empty" : "empty") << ".\n";
    std::cout << '\n';

    int i;
    opt.create_value(&i);
    std::cout << "optional is " << (opt.has_value() ? "not empty" : "empty") << ".\n";
    std::cout << "pointer is " << opt.value().get() << '\n';
}
