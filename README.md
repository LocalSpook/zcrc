# CRC

**Warning: this library is under construction.
It's functional, but its main selling point—SIMD acceleration—is not yet implemented.**

[![CodeQL](https://github.com/LocalSpook/crc/actions/workflows/codeql.yml/badge.svg)](https://github.com/LocalSpook/crc/actions/workflows/codeql.yml)

A C++20 SIMD-accelerated high-performance constexpr-capable single-header-only CRC library with over 100 predefined CRCs and the ability to create custom ones.

## API

Most likely, all you're looking for is a simple function that calculates a common CRC variant.

```cpp
#include <crc/crc.hpp> // Or: import crc;

std::string_view data {"Hello world!"};
std::uint32_t crc1 {crc::crc32c(data)}; // Takes a range...
std::uint32_t crc2 {crc::crc32c(data.begin(), data.end())}; // ...or an iterator pair.
```

For more complex cases, the CRC can be built up incrementally:

```cpp
// Step 1: initialize CRC state.
auto c {crc::crc64_xz::initialize()};

// Step 2: feed it data.
c.process("Some da"sv);
c.process(u8"ta processed in "sv);
c.process(std::vector<std::uint8_t> {'p', 'a', 'r', 't', 's'});

// Step 3: extract the final CRC.
auto checksum {c.finalize()};
assert(checksum == crc::crc64_xz("Some data processed in parts"sv));
```

Notice that you can pass any byte-like type to `process`, without any casts.

The CRC you're looking for almost certainly comes predefined
(if it's missing, consider [filing an issue](https://github.com/LocalSpook/crc/issues)),
but you can define your own too:

```cpp
inline constexpr auto crc32c {crc::crc<
    std::uint32_t, // The CRC's type; an unsigned integer at least as wide as the polynomial.
    32,            // The polynomial's width.
    0x1EDC6F41,    // The polynomial, with an implicit leading term.
    0xFFFFFFFF,    // The initial value of the CRC register.
    true,          // True if the bits in a byte should be ordered from LSb to MSb, false if vice-versa.
    true,          // True if the result should be reflected during finalization.
    0xFFFFFFFF     // The value XORed into the result at the very end, after any reflection.
>{}};
```

You can adapt existing CRCs like so:

```cpp
// Identical to crc::crc32, but with the opposite bit ordering.
inline constexpr auto crc32_reflected {crc::crc<
    crc::crc32::crc_type,
    crc::crc32::width,
    crc::crc32::poly,
    crc::crc32::initial,
    !crc::crc32::refin, // ⭐
    crc::crc32::refout,
    crc::crc32::xorout
>{}};
```

Note that CRCs of width greater than 64 are currently unsupported.

Every CRC is an instance of the `crc::crc` class template,
which provides the following interface:

```cpp
template <
    typename CRCType,
    std::size_t Width,
    CRCType Poly,
    CRCType Init,
    bool RefIn,
    bool RefOut,
    CRCType XOROut>
class crc {
    using crc_type = ...;
    static constexpr std::size_t width;
    static constexpr crc_type poly;
    static constexpr crc_type init;
    static constexpr crc_type xorout;
    static constexpr bool refin;
    static constexpr bool refout;
    static constexpr crc_type residue;

    template <std::ranges::contiguous_range R>
    requires __byte_like<std::ranges::range_value_t<R>>
    static constexpr crc_type operator()(R&& range);

    template <std::contiguous_iterator I, std::sentinel_for<I> S>
    requires __byte_like<std::iter_value_t<I>>
    static constexpr crc_type operator()(I begin, S end);
};

// Where __byte_like is defined as:
template <typename T>
concept __byte_like = std::is_trivially_copyable_v<T> && sizeof(T) == 1 && !std::same_as<std::remove_cv_t<T>, bool>;
```

CRCs are function objects, so you can pass them to other algorithms:

```cpp
std::vector<std::string> strings {"Apple", "Banana", "Cherry", "Dragonfruit"};
std::vector<std::uint32_t> crcs {std::from_range, strings | std::views::transform(crc::crc32c)};
```

## Installing

### With FetchContent (recommended)

```cmake
FetchContent_Declare(crc
    GIT_REPOSITORY https://github.com/LocalSpook/crc
    GIT_TAG ... # You should use the latest commit on the main branch.
    SYSTEM
)
FetchContent_MakeAvailable(crc)

target_link_libraries(... crc::crc)
```

### With find_package

```cmake
find_package(crc REQUIRED)
target_link_libraries(... crc::crc)
```

### With vendoring (discouraged)

Just copy [`include/crc/crc.hpp`](include/crc/crc.hpp) into your directory structure.
For any serious project, you're highly recommended to use a proper dependency management
system instead, but this method *is* officially supported.

## Building

Simply run:

```sh
cmake -B build [-G Ninja]
cmake --build build
```

To also build the module, add `-DCRC_MODULE=ON`, then pass `crc::crc-module` to `target_link_libraries`.
The module cannot currently be installed, so this is only available when using FetchContent.

To build tests, add `-DBUILD_TESTING=ON`.
The resulting test binary will be `build/bin/tests`.
Our testing framework is Catch2;
it will be downloaded automatically using FetchContent.
If you also configured with `-DCRC_MODULE=ON`,
the module tests will be added to the binary.
We have a 2 by 2 testing matrix:
compile versus run time, and header versus module.
To test just the header, run `./build/bin/tests --section header`.
To test just the module, run `./build/bin/tests --section module`.

## Miscellaneous

- This library provides zero ABI stability guarantees.
- It should compile cleanly even at very high warning levels.
  If you see a warning, [file an issue](https://github.com/LocalSpook/crc/issues).
