# CRC

**Warning: this library is under construction.
It's functional, but its main selling point—SIMD acceleration—is not yet implemented.**

[![CodeQL](https://github.com/LocalSpook/crc/actions/workflows/codeql.yml/badge.svg)](https://github.com/LocalSpook/crc/actions/workflows/codeql.yml)

A C++20 SIMD-accelerated high-performance constexpr-capable single-header-only CRC library with over 100 predefined CRCs and the ability to create custom ones.

## API

Most likely, all you're looking for is a simple function that calculates a common CRC variant.
There are two fundamental CRC operations: *building* a CRC to append to your message,
and *verifying* whether an existing message is valid:

```cpp
#include <crc/crc.hpp> // Or: import crc;

// Build a CRC:
std::string_view data {"Hello world!"};
std::uint32_t crc {crc::crc32c::calculate(data)};

// Verify a CRC:
if (!crc::crc8_bluetooth::is_valid(some_message)) {
    throw std::runtime_error {"Message is corrupted!"};
}
```

For more complex cases, the CRC can be built up incrementally:

```cpp
// Step 1: initialize CRC state.
crc::crc64_xz crc {};

// Step 2: feed it data.
// Notice how you can pass in any byte-like type, without any casts.
crc = crc::process(crc, "Some da"sv);
crc = crc::process(crc, u8"ta processed in "sv);
crc = crc::process(crc, std::vector<std::uint8_t> {'p', 'a', 'r', 't', 's'});

// Step 3: extract the final CRC.
std::uint64_t result {crc::finalize(crc)};
assert(result == crc::crc64_xz::calculate("Some data processed in parts"sv));
```

All the functions above also have overloads taking iterator pairs instead of ranges.

### Choosing an algorithm

There are many algorithms for calculating CRCs.
The library will pick a good default (currently, that's `crc::algorithms::slice_by<8>`),
but it isn't omniscient,
so you have the ability to specify which algorithm you want.
The following algorithms are available:

- **`crc::algorithms::slice_by<N>`:** process `N` bytes at a time.
  Requires an `N * 256 * sizeof(CRCType)` byte lookup table.
  For example, CRC32C implemented with slice-by-4 requires a 4 KiB lookup table.

To specify an algorithm, pass it as the first parameter to `crc::<...>::calculate` or `crc::process`:

```cpp
crc::crc32_mpeg2::calculate(crc::algorithms::slice_by<8>, ...);

crc::crc32_mpeg2 crc {};
crc = crc::process(crc::algorithms::slice_by<8>, crc, ...);
```

If you want to write your own functions that take CRC algorithms as arguments,
constrain them with the `crc::algorithm` concept:

```cpp
void my_function(crc::algorithm auto algo, ...) {
    crc::crc32c::calculate(algo, ...); // Pass along the algorithm.
}
```

### Defining your own CRCs

The CRC you're looking for almost certainly comes predefined
(if it's missing, consider [filing an issue](https://github.com/LocalSpook/crc/issues)),
but you can define your own too:

```cpp
using crc32c = crc::crc<
    std::uint32_t, // The CRC's type; an unsigned integer at least as wide as the polynomial.
    32,            // The polynomial's width.
    0x1EDC6F41,    // The polynomial, with an implicit leading term.
    0xFFFFFFFF,    // The initial value of the CRC register.
    true,          // True if the bits in a byte should be ordered from LSb to MSb, false if vice-versa.
    true,          // True if the result should be reflected during finalization.
    0xFFFFFFFF     // The value XORed into the result at the very end, after any reflection.
>;
```

Or you can adapt existing CRCs:

```cpp
// Identical to crc::crc32, but with the opposite bit ordering.
using crc32_reflected = crc::crc<
    crc::crc32::crc_type,
    crc::crc32::width,
    crc::crc32::poly,
    crc::crc32::initial,
    !crc::crc32::refin, // ⭐
    !crc::crc32::refout, // ⭐
    crc::crc32::xorout
>;
```

Note that CRCs of width greater than 64 are currently unsupported.

### Composability

All provided functions are function objects and can be passed to other algorithms:

```cpp
std::vector<std::string> strings {"Apple", "Banana", "Cherry", "Dragonfruit"};
std::vector<std::uint32_t> crcs {std::from_range, strings | std::views::transform(crc::crc32c::calculate)};

// Calculate a CRC over several noncontiguous chunks.
std::vector<std::vector<unsigned char>> data {...};
std::uint32_t crc {crc::finalize(std::ranges::fold_left(data, crc::crc32c {}, crc::process))};
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
