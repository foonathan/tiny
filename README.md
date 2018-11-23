# foonathan/tiny

[![Build Status](https://dev.azure.com/foonathan/tiny/_apis/build/status/foonathan.tiny)](https://dev.azure.com/foonathan/tiny/_build/latest?definitionId=3)

> Note: This project is currently WIP, no guarantees are made until an 0.1 release.

This project is a C++11 library for putting every last bit to good use.
It combines various techniques such as [`llvm::PointerIntPair`](http://llvm.org/doxygen/classllvm_1_1PointerIntPair.html), [tombstones](https://youtu.be/MWBfmmg8-Yo?t=2466) and (a custom implementation of) bitfields to write types that use as little bits as possible.

**Important**: This library is a low-level implementation library.
It is meant for experienced C++ programmers who write foundational code, such as vocabulary types, containers etc.
As such, the types used by this library should *not* appear in interfaces directly.
Instead they are implementation details of other types.
This is especially true for the `_impl` types such as `tiny::optional_impl`.
Proper vocabulary types are meant to be built on top of them.

## Features

### Foundations

* `tiny::bit_view`: A view for a range of (possibly disjoint) bits.
  This is a fundamental type used internally and for implementing some traits.
* `tiny::enum_traits`: Traits to specify range of an `enum`.
  Will be automatically implemented for enumerations with members such as `unsigned_count_`,
  but can be specialized for own types.
  They are required for exposing information about your enumerations.
* `tiny::padding_traits`: Traits to specify padding bytes of your type.
  They basically provide a `tiny::bit_view` to the bytes that are padding.
  `tiny::padding_traits_aggregate` provides a semi-automatic implementation for aggregate types.

### Tiny Types

Tiny types are types that are just a couple of bits in size.
They cannot be stored directly but instead in a storage type:

* `tiny::tiny_storage`: Stores multiple tiny types tightly packed together (think bitfields).

* `tiny::pointer_tiny_storage`: Stores tiny types in the alignment bits of a pointer.

* `tiny::padding_tiny_storage`: Stores tiny types in the padding of another type.

The tiny types provided by this library:

* `tiny::tiny_bool`: a `bool`
* `tiny::tiny_int<N>`/`tiny::tiny_unsigned<N>`: `N` bit integers (where `N` is tiny)
* `tiny::tiny_int_range<Min, Max>`: the specified integers
* `tiny::tiny_enum<E>`: a tiny enumeration
* `tiny::tiny_flag_set<Flags>`: a set of flags, i.e. multiple booleans with names

### Tombstones

Optional implementations like `std::optional<T>` need to have storage for `T` and a boolean indicating whether or not one is currently stored.
Due to alignment and padding this can easily double the size beyond what is necessary.

A *tombstone* is a special "invalid" value of a type, like a `nullptr` reference.
It can be used to indicate an empty optional without needing to store a boolean.

The `tiny::tombstone_traits` are a non-intrusive way of exposing tombstones without creating the ability to expose the invalid type states.

### Vocabulary Implementation Helpers

A space efficient optional implementation can be built on top of the tombstone traits.
However, certain design decisions of such vocabulary types are somewhat controversial.
So instead of writing the full implementation, the library contains just the bare minimum.
Proper vocabulary types can be built on top.

Those are:

* `tiny::optional_impl`: a tombstone enabled and thus compact optional
* `tiny::pointer_variant_impl`: a union of multiple pointer types using alignment bits to store the currently active pointer

## FAQ

**Q: Are those tricks standard conforming C++?**

A: For the most part, yes:
The implementations are carefully crafted to avoid undefined behavior.
However, I'm certainly relying on some implementation-defined behavior.
For example, the `tiny::pointer_tiny_storage` makes some assumptions about the integral representation of pointers that are not necessarily guaranteed,
but all implementations I'm aware of work that way.
Please let me know if yours don't!

**Q: The tiny types behave weird when I use `auto`.**

A: That's not a question, but yes.
By their very nature I cannot expose references to a stored tiny type.
Proxies are used instead, which interact poorly with `auto`.
This is an implementation limit I can't really do anything about.

**Q: It breaks when I do this!**

A: Don't do that. And file an issue (or a PR, I have a lot of other projects...).

**Q: This is awesome!**

A: Thanks. I do have a Patreon page, so consider checking it out:

[![Patreon](https://c5.patreon.com/external/logo/become_a_patron_button.png)](https://patreon.com/foonathan)

## Documentation

> A full reference documentation is WIP, look at the comments in the header files for now.

Annotated tutorial style examples can be found in [the example directory](example/).

### Compiler Support

The library requires a C++11 compiler.
Compilers that are being tested on CI:

* Linux:
    * GCC 5 to 8
    * clang 4 to 7
* MacOS:
    * XCode 9 and 10
* Windows:
    * Visual Studio 2017

It only requires the following standard library headers:

* `cstddef` and `cstdint`
* `climits` and `limits`
* `cstdlib` (for `std::abort`) and `cstring` (for `std::memcpy`)
* `new` (for placement new only)
* `type_traits`

The `debug_assert` library optionally requires `cstdio` for printing messages to `stderr`.
Defining `DEBUG_ASSERT_NO_STDIO` disables that.

It does not use exceptions, RTTI or dynamic memory allocation.

### Installation

The library is header-only and has only my [debug_assert](https://github.com/foonathan/debug_assert) library as dependency.

If you use CMake, `debug_assert` will be cloned automatically if not installed on the system.
You can use it with `add_subdirectory()` or install it and use `find_package(foonathan_tiny)`,
then link to `foonathan::foonathan_tiny` and everything will be setup automatically.

## Planned Features

* NaN floating point packing
* More vocabulary type helpers
