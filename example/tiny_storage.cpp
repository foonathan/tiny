// This example demonstrates how to use the tiny types and padding traits.

#include <cstdint>
#include <iostream>
#include <memory>

#include <foonathan/tiny/check_size.hpp>           // for tiny::check_size
#include <foonathan/tiny/padding_tiny_storage.hpp> // for tiny::padding_tiny_storage
#include <foonathan/tiny/pointer_tiny_storage.hpp> // for tiny::pointer_tiny_storage
#include <foonathan/tiny/tiny_bool.hpp>            // for tiny::tiny_bool
#include <foonathan/tiny/tiny_enum.hpp>            // for tiny::tiny_enum
#include <foonathan/tiny/tiny_int.hpp>             // for tiny::tiny_int_range

namespace tiny = foonathan::tiny;

//=== maybe_owning_ptr ===//
// A smart pointer that sometimes owns the `T` and sometimes it doesn't.
// (Not a complete implementation)
template <typename T>
class maybe_owning_ptr
{
    // Conceptually we need to store
    //
    //    T* ptr_;
    //    bool should_delete_;
    //
    // However, due to alignment that would be equivalent to storing two pointers,
    // instead of storing a single pointer and one bit.
    //
    // So instead:
    tiny::pointer_tiny_storage<T, tiny::tiny_bool> storage_;
    // This stores a pointer to a `T` and a (tiny) `bool`.
    //
    // If we have a type with an alignment greater `1`,
    // we know that the addresses stored in the pointer will be a multiple of at two.
    // So instead of storing `0` in the final bit, we store our `bool` instead.
    //
    // If we have a type with an alignment of `1`, it will store the `bool` separately.
    //
    // `tiny::pointer_tiny_storage` could also store multiple tiny types at once.

public:
    explicit maybe_owning_ptr(T* non_owning) noexcept
    // don't delete the raw pointer
    : storage_(non_owning, false)
    {}

    explicit maybe_owning_ptr(std::unique_ptr<T> ptr) noexcept
    // do delete the unique ptr
    : storage_(ptr.release(), true)
    {}

    ~maybe_owning_ptr() noexcept
    {
        // delete if owning
        if (is_owning())
            delete get();
    }

    // for simplicity here
    maybe_owning_ptr(const maybe_owning_ptr&) = delete;
    maybe_owning_ptr& operator=(const maybe_owning_ptr&) = delete;

    bool is_owning() const noexcept
    {
        // Whether or not it is owning is stored in the tiny type (as there is only one, it is
        // unambiguous).
        //
        // We could also use `storage_.template at<0>()` (first tiny type),
        // or `storage_[tiny::tiny_bool{}]` (we want the tiny boolean).
        //
        // Important: this returns a proxy type for implementation reasons, not a bool.
        // In particular you can't get the address.
        return storage_.tiny();
    }

    T* get() const noexcept
    {
        // Return the pointer.
        // This will clear all bits used for storage so we have a valid pointer.
        //
        // Again, this will return a proxy.
        return storage_.pointer();
    }

    T& operator*() const noexcept
    {
        return *get();
    }
    T* operator->() const noexcept
    {
        return get();
    }
};

void use_maybe_owning()
{
    std::cout << "=== maybe_owning_ptr ===\n\n";

    // `tiny::check_size` is just an utility for checking sizes that also shows the actual size if
    // it didn't match.
    // (try changing the expected size)
    static_assert(
        tiny::check_size<maybe_owning_ptr<std::uint32_t>, sizeof(void*)>(),
        "std::uint32_t has an alignment of 4, so we can easily fit one bool inside of it");
    static_assert(tiny::check_size<maybe_owning_ptr<char>, 2 * sizeof(void*)>(),
                  "char has an alignment of 1, so every address would be valid");

    std::uint32_t i = 0;

    maybe_owning_ptr<std::uint32_t> non_owning(&i);
    std::cout << "Address is: " << non_owning.get() << '\n';
    std::cout << "Is owning? " << std::boolalpha << non_owning.is_owning() << '\n';
    std::cout << '\n';

    maybe_owning_ptr<std::uint32_t> owning(std::unique_ptr<std::uint32_t>(new std::uint32_t));
    std::cout << "Address is: " << owning.get() << '\n';
    std::cout << "Is owning? " << std::boolalpha << owning.is_owning() << '\n';
    std::cout << '\n';

    std::cout << '\n';
}

//=== gregorian_date ===//
// Months in a year.
enum class month
{
    jan,
    feb,
    mar,
    apr,
    may,
    jun,
    jul,
    aug,
    sep,
    oct,
    nov,
    dec,

    // Informs tiny that this is an enum with enumerators in the range `[0, _unsigned_count)`.
    // See `enum_traits.hpp` for details.
    _unsigned_count,
};

// Suppose we don't care about the year, so we only need to store month + day.
// There are 365.24219 days in a year, so we need 8.51270961388 bits to store every day.
//
// A naive implementation:
//
//    month m;
//    std::uint8_t day;
//
// This uses `2 * sizeof(int) * CHAR_BIT` (so usually 64 bits), as the underlying type of `month` is
// `int`!
//
// The more space efficient implementation:
class gregorian_day_of_year
{
    // We again use tiny types:
    // `tiny::tiny_enum` stores the specified enumeration in a space efficient way,
    // and `tiny::tiny_int_range` can store the specified integers.
    //
    // This time we don't have any storage already available, so we use `tiny::tiny_storage`,
    // which stores just tiny types.
    //
    // In this case it needs to store `4` bits for the month and `5` bits for the day,
    // so 2 bytes in total.
    tiny::tiny_storage<tiny::tiny_enum<::month>, tiny::tiny_int_range<1, 31>> storage_;

public:
    // Simple constructor ignoring validation.
    explicit gregorian_day_of_year(::month m, int day) noexcept : storage_(m, day) {}

    ::month month() const noexcept
    {
        // Again, this actually returns a proxy.
        // Alternatively you could use `storage_[tiny::tiny_enum<month>{}]`.
        return storage_.at<0>();
    }

    int day() const noexcept
    {
        // Again, this actually returns a proxy.
        // Alternatively you could use `storage_[tiny::tiny_int_range<1, 31>{}]`.
        return storage_.at<1>();
    }

    void set(::month m, int day)
    {
        // Assignment just works and will do a range check
        // (but not verify the month and day combination itself obviously).
        //
        // This time the other access method is used because why not.
        // (It is nicer if we have a typedef).
        storage_[tiny::tiny_enum<::month>{}]    = m;
        storage_[tiny::tiny_int_range<1, 31>{}] = day;
        // (In case you're wondering, the tiny types themselves are empty, so the default
        // constructor does nothing)
    }

    // Note that writing access functions is necessary because the tiny storage interface isn't
    // ideal. It is just an implementation detail.

    // See below.
    friend tiny::padding_traits<gregorian_day_of_year>;
};

void use_gregorian_day_of_year()
{
    std::cout << "=== gregorian_day_of_year ===\n\n";

    static_assert(tiny::check_size<gregorian_day_of_year, 2>(), "should use 9 bits");

    gregorian_day_of_year doy(month::jan, 1);
    std::cout << "Is january? " << std::boolalpha << (doy.month() == month::jan) << '\n';
    std::cout << "Day: " << doy.day() << '\n';
    std::cout << '\n';

    doy.set(month::may, 5);
    std::cout << "Is january? " << std::boolalpha << (doy.month() == month::jan) << '\n';
    std::cout << "Day: " << doy.day() << '\n';
    std::cout << '\n';

    std::cout << "\n";
}

// Now we want to store a year as well.
// Having learned nothing from the Y2K, we store the year in the range `[0, 99]` only.
// This requires an additional 7 bit, so we have 16 bits in total.
//
// But the `gregorian_day_of_year` is 2 bytes already, so we need 3 bytes in total,
// even though there only one bit of the second byte is used!
//
// No problem, we just need to tell tiny that we have 7 unused bits.
// We do that by specializing the `tiny::padding_traits`.
namespace foonathan
{
namespace tiny
{
    template <>
    struct padding_traits<gregorian_day_of_year>
    {
        // We have a custom specialization.
        static constexpr auto is_specialized = true;

        // This function returns a `tiny::bit_view` (a view into a subset of bits),
        // for the (possibly disjoint) range of memory we're not using.
        // Here we just forward to the `spare_bits()` of our storage.
        static auto padding_view(unsigned char* memory) noexcept
            -> decltype(std::declval<gregorian_day_of_year>().storage_.spare_bits())
        {
            // reinterpret_cast here is always fine.
            return reinterpret_cast<gregorian_day_of_year*>(memory)->storage_.spare_bits();
        }
        static auto padding_view(const unsigned char* memory) noexcept
            -> decltype(std::declval<const gregorian_day_of_year>().storage_.spare_bits())
        {
            return reinterpret_cast<const gregorian_day_of_year*>(memory)->storage_.spare_bits();
        }
    };
} // namespace tiny
} // namespace foonathan

// Now we can write the `gregorian_date`.
class gregorian_date
{
    // This time we use the padding bits of `gregorian_day_of_year` to store our tiny integer.
    // (The class also takes care that the padding bits are initialized and copied properly).
    tiny::padding_tiny_storage<gregorian_day_of_year, tiny::tiny_int_range<0, 99>> storage_;

public:
    explicit gregorian_date(gregorian_day_of_year doy, int year) : storage_(doy, year) {}

    const gregorian_day_of_year& day_of_year() const noexcept
    {
        // This time we can actually return a true reference, not a proxy.
        return storage_.object();
    }

    int year() const noexcept
    {
        // But the year is a proxy again.
        return storage_.tiny();
    }
};

void use_gregorian_date()
{
    std::cout << "=== gregorian_date ===\n\n";

    static_assert(tiny::check_size<gregorian_date, 2>(), "this is ideal!");

    gregorian_date date(gregorian_day_of_year(month::jan, 1), 70);
    std::cout << "Is january? " << std::boolalpha << (date.day_of_year().month() == month::jan)
              << '\n';
    std::cout << "Day: " << date.day_of_year().day() << '\n';
    std::cout << "Year: " << date.year() << '\n';
    std::cout << '\n';

    std::cout << '\n';
}

//=== padding_traits for aggregates ===//
// If we have a simple aggregate we can specialize the padding traits a lot easier.
struct some_aggregate
{
    bool a;
    // 7 bytes padding
    std::uint64_t b;
    std::uint8_t  c;
    // 3 bytes padding
    std::uint32_t d;
    // 4 bytes padding
};

namespace foonathan
{
namespace tiny
{
    // Just inherit from `tiny::padding_traits_aggregate` and list all members.
    // Meta-programming calculates the padding for you.
    template <>
    struct padding_traits<some_aggregate>
    : tiny::padding_traits_aggregate<FOONATHAN_TINY_MEMBER(some_aggregate, a),
                                     FOONATHAN_TINY_MEMBER(some_aggregate, b),
                                     FOONATHAN_TINY_MEMBER(some_aggregate, c)>
    {};
} // namespace tiny
} // namespace foonathan

void use_padding_aggregate()
{
    std::cout << "=== padding_traits aggregate ===\n\n";

    std::cout << "some_aggregate has padding of: " << tiny::padding_bit_size<some_aggregate>()
              << '\n';

    std::cout << '\n';
}

//=== run all examples ===//
int main()
{
    use_maybe_owning();
    use_gregorian_day_of_year();
    use_gregorian_date();
    use_padding_aggregate();
}
