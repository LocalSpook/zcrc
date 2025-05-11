// SPDX-License-Identifier: MIT

// ────────── Design notes ──────────
//
// To understand this library's internals, read the following papers:
//
//   - A Painless Guide to CRC Error Detection Algorithms
//     http://ross.net/crc/download/crc_v3.txt
//     - Introduces CRCs and their classic lookup-table-based implementations.
//       Our constexpr and fallback implementations use this approach.
//     - Defines the CRC parametrization that we use.
//
//   - Fast CRC Computation for Generic Polynomials Using PCLMULQDQ Instruction
//     https://www.researchgate.net/publication/263424619_Fast_CRC_computation
//     - Our main algorithm.
//
// Our predefined CRCs come from the following sources:
//
//  - Catalogue of parametrised CRC algorithms
//    https://reveng.sourceforge.io/crc-catalogue/all.htm
//
//  - CRC Polynomial Zoo
//    https://users.ece.cmu.edu/~koopman/crc/crc32.html
//
// To minimize compile times, we prefer plain loops over range algorithms.
//
// We provide dozens of predefined CRCs, but the typical user only needs
// one or two of them. This means that defining a CRC (i.e. instantiating crc::crc)
// must be cheap: specifically, it must NOT eagerly calculate lookup tables.
//
// The code has been ADL-proofed.

#ifndef CRC_HPP_INCLUDED
#define CRC_HPP_INCLUDED

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <ranges>
#include <type_traits>
#include <utility>

// This is defined when building as a module and when building tests.
#ifndef CRC_JUST_THE_INCLUDES

#ifdef CRC_EXPORT_SYMBOLS
#define CRC_EXPORT export
#else
#define CRC_EXPORT
#endif

#define CRC_RETURNS(...)              \
    noexcept(noexcept(__VA_ARGS__)) { \
        return __VA_ARGS__;           \
    }

#if __cplusplus >= 202302L && __cpp_static_call_operator >= 202207L
#define CRC_STATIC_CALL_OPERATOR static
#define CRC_CONST_CALL_OPERATOR
#else
#define CRC_STATIC_CALL_OPERATOR
#define CRC_CONST_CALL_OPERATOR const
#endif

#if __cplusplus >= 202302L && __cpp_constexpr >= 202211L
#define CRC_STATIC23 static
#else
#define CRC_STATIC23
#endif

namespace crc::inline v1 {

namespace detail {

struct algorithm_base {};

} // namespace detail

namespace algorithms {

CRC_EXPORT template <std::size_t N>
struct slice_by_t : detail::algorithm_base {
    static_assert(N != 0);
    explicit slice_by_t() = default;
};

CRC_EXPORT template <std::size_t N>
inline constexpr slice_by_t<N> slice_by {};

} // namespace algorithms

CRC_EXPORT template <typename T>
concept algorithm = std::derived_from<T, detail::algorithm_base>;

namespace detail {

template <typename T>
concept byte_like = std::is_trivially_copyable_v<T> && sizeof(T) == 1 && !std::same_as<std::remove_cv_t<T>, bool>;

template <typename T>
[[nodiscard]] constexpr T bottom_n_mask(const std::size_t width) noexcept {
    return (width == 0) ? 0 : T(~T(0)) >> (std::numeric_limits<T>::digits - width);
}

template <typename T>
[[nodiscard]] constexpr bool bit_is_set(const T n, const std::size_t b) noexcept {
    return (n & (T{1} << b)) != T{};
}

// Reflect the bottom [b] bits of [n]. The rest of the bits must be zero.
template <typename T>
[[nodiscard]] constexpr T reflect(const T n, const std::size_t b) noexcept {
    // We reflect 10 bits at a time, transforming JIHGFEDCBA into ABCDEFGHIJ,
    // using the following pseudo-SIMD algorithm:
    //
    // 1. Multiply to broadcast the bits:
    //       ______________________________________________________JIHGFEDCBA
    //     * _________1_________1_________1_________1_________1_________1____
    //     = JIHGFEDCBAJIHGFEDCBAJIHGFEDCBAJIHGFEDCBAJIHGFEDCBAJIHGFEDCBA____
    //
    // 2. Mask out bits we don't want:
    //       JIHGFEDCBAJIHGFEDCBAJIHGFEDCBAJIHGFEDCBAJIHGFEDCBAJIHGFEDCBA____
    //     & _____1____1_____1____1_____1____1_____1____1_____1____1_________
    //     = _____E____J_____D____I_____C____H_____B____G_____A____F_________
    //
    // 3. Multiply (which is equivalent to adding together various shifts):
    //       _____E____J_____D____I_____C____H_____B____G_____A____F_________
    //
    // <<  1 ____E____J_____D____I_____C____H_____B____G_____A____F__________ +
    // << 13 ___D____I_____C____H_____B____G_____A____F______________________ +
    // << 25 __C____H_____B____G_____A____F__________________________________ +
    // << 37 _B____G_____A____F______________________________________________ +
    // << 49 A____F__________________________________________________________ +
    //     = ABCDEFGHIJ__ABCD_FGHI___ABC__FGH____AB___FG_____A____F__________
    //
    // 4. Right shift to extract the 10 reflected bits at the top,
    //    giving us our final desired value.
    //
    // It may be possible to do more bits at a time. I tried doing 11, though
    // couldn't figure it out.
    return (b <= 10)
        ? ((((n * 0x0040100401004010ULL) & 0x0420841082104200) * 0x0002002002002002) >> (64 - b))
        : ((detail::reflect(n & 0x3FF, 10) << (b - 10)) | detail::reflect(n >> 10, b - 10));
}

// std::abs is only constexpr in C++23 >_>
template <typename T>
[[nodiscard]] constexpr T abs(const T n) noexcept {
    return n < 0 ? -n : n;
}

// A generalized shift, where shifting by more bits than a type's width simply
// returns 0, and shifting by a negative amount shifts in the opposite direction
// (both cases are UB with the builtin shift). This greatly simplifies some of
// our algorithms.
template <typename T>
[[nodiscard]] constexpr T lshift(const T n, const std::int64_t b) noexcept {
    if (static_cast<std::size_t>(detail::abs(b)) >= (sizeof(T) * 8)) {
        return 0;
    } else if (b < 0) {
        return n >> -b;
    } else {
        return n << b;
    }
}

// Ditto.
template <typename T>
[[nodiscard]] constexpr T rshift(const T n, const std::int64_t b) noexcept {
    if (static_cast<std::size_t>(detail::abs(b)) >= (sizeof(T) * 8)) {
        return 0;
    } else if (b < 0) {
        return n << -b;
    } else {
        return n >> b;
    }
}

template <typename T, std::endian E, std::input_iterator I>
[[nodiscard]] constexpr T read(I it) {
    std::array<char, sizeof(T)> bytes; // NOLINT(cppcoreguidelines-pro-type-member-init)
    for (std::size_t i {0}; i < sizeof(T); ++i, ++it) {
        bytes[E == std::endian::native ? i : sizeof(T) - i] = static_cast<char>(*it);
    }
    return std::bit_cast<T>(bytes);
}

// clang-format off
template <std::size_t Bits>
using least_uint =
    std::conditional_t<Bits <=  8, std::uint8_t,
    std::conditional_t<Bits <= 16, std::uint16_t,
    std::conditional_t<Bits <= 32, std::uint32_t,
    std::uint64_t
>>>;
// clang-format on

template <typename CRCType, std::size_t Width, CRCType Poly, bool RefIn, std::size_t Slices>
inline constexpr auto tables {[] {
    // TODO: get rid of the "Slices > 1" condition and properly implement
    // the size reduction optimization for multiple slices.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    std::array<std::array<std::conditional_t<(RefIn || Slices > 1),
        CRCType, detail::least_uint<7 + std::bit_width(Poly)>>, 256>, Slices> tables_;
    for (CRCType r {RefIn ? 1 : (1ULL << (Width - 1))}; auto& table : tables_ | std::views::reverse) {
        // Step 1: calculate the power of two entries.
        table[0] = 0;
        for (std::size_t i {0}; i < 8; ++i) {
            if constexpr (RefIn) {
                r = table[1 << (7 - i)] = (r >> 1) ^ (detail::bit_is_set(r, 0) ? detail::reflect(Poly, Width) : 0);
            } else {
                r = table[1 << i] = (r << 1) ^ (detail::bit_is_set(r, Width - 1) ? Poly : 0);
            }
        }
        // Step 2: calculate the rest of the entries.
        for (std::size_t i {2}; i < 256; i <<= 1) {
            for (std::size_t j {1}; j < i; ++j) {
                table[i ^ j] = table[i] ^ table[j];
            }
        }
    }
    return tables_;
}()};

template <typename CRCType, std::size_t Width, CRCType Poly, CRCType Init, bool RefIn, bool RefOut, CRCType XOROut>
class crc_base {
private:
    CRCType m_crc {[] () noexcept {
        if constexpr (RefIn) {
            return detail::reflect(Init, Width);
        } else if constexpr (Width < 8) {
            return Init << (8 - Width);
        } else {
            return Init;
        }
    }()};

    [[nodiscard]] constexpr crc_base(const CRCType crc) noexcept : m_crc {crc} {}

    friend struct process_fn;
    friend struct finalize_fn;
    friend struct is_valid_fn;

public:
    [[nodiscard]] constexpr crc_base() noexcept = default;

    [[nodiscard]] friend constexpr bool operator==(const crc_base lhs, const crc_base rhs) noexcept {
        if constexpr (RefIn) {
            return lhs.m_crc == rhs.m_crc;
        } else {
            return (lhs.m_crc & detail::bottom_n_mask<CRCType>(Width)) == (rhs.m_crc & detail::bottom_n_mask<CRCType>(Width));
        }
    }
};

template <typename CRCType, std::size_t Width, CRCType Poly, bool RefIn,
          std::size_t N, std::contiguous_iterator I, std::sized_sentinel_for<I> S>
[[nodiscard]] constexpr CRCType process_fn_impl(algorithms::slice_by_t<N>, CRCType crc, I it, S end) noexcept {
    const auto fold {[&]<std::size_t... B>(std::index_sequence<B...>) {
        CRC_STATIC23 constexpr auto t {detail::tables<CRCType, Width, Poly, RefIn, N>.end() - sizeof...(B)};
        if constexpr (RefIn) {
            crc = (t[B][(detail::rshift(crc, 8 * B) & 0xFF) ^ static_cast<std::uint8_t>(it[B])] ^ ...
                ^ detail::rshift(crc, sizeof...(B) * 8));
        } else {
            crc = (t[B][(detail::rshift(crc, Width - 8 * (B + 1)) & 0xFF) ^ static_cast<std::uint8_t>(it[B])] ^ ...
                ^ detail::lshift(crc, sizeof...(B) * 8));
        }
    }};

    // Process the main body in chunks of N bytes.
    const auto tail_len {(end - it) % N};
    for (const auto end_of_main_loop {end - tail_len}; it != end_of_main_loop; it += N) {
        fold(std::make_index_sequence<N>{});
    }

    // Process the tail in powers of 2.
    [&]<std::size_t... P>(std::index_sequence<P...>){
        (((tail_len & (1 << P))
            ? (void)(fold(std::make_index_sequence<1 << P>{}), it += 1 << P)
            : (void)0
        ), ...);
    }(std::make_index_sequence<std::bit_width(N - 1)>{});

    return crc;
}

struct process_fn {
    // Consider a user program that calculates CRCs over several different types:
    //
    //    crc::crc32c(std::span<char>)
    //    crc::crc32c(std::span<unsigned char>)
    //    crc::crc32c(std::span<char8_t>)
    //
    // These calls all do the exact same thing, but each is a separate template
    // instantiation, so the compiler cannot deduplicate them in the final binary.
    // To achieve that, those calls must type-erase their input to const char * and
    // delegate to another function. But wait! We can't do that cast at compile time!
    // So we need to *conditionally* do that type erasure.
    //
    // This is a mess, but it's what we need to do to provide a zero-overhead abstraction.
    template <typename CRCType, std::size_t Width, CRCType Poly, CRCType Init, bool RefIn, bool RefOut, CRCType XOROut,
              std::contiguous_iterator I, std::sized_sentinel_for<I> S>
    requires detail::byte_like<std::iter_value_t<I>>
    [[nodiscard]] CRC_STATIC_CALL_OPERATOR constexpr crc_base<CRCType, Width, Poly, Init, RefIn, RefOut, XOROut>
    operator()(const algorithm auto algo, const crc_base<CRCType, Width, Poly, Init, RefIn, RefOut, XOROut> crc, I it, S end) CRC_CONST_CALL_OPERATOR
        CRC_RETURNS(std::is_constant_evaluated()
            ? detail::process_fn_impl<CRCType, Width < 8 ? 8 : Width, Width < 8 ? Poly << (8 - Width) : Poly, RefIn>(
                algo, crc.m_crc,
                std::to_address(it),
                std::to_address(it) + (end - it))
            : detail::process_fn_impl<CRCType, Width < 8 ? 8 : Width, Width < 8 ? Poly << (8 - Width) : Poly, RefIn>(
                algo, crc.m_crc,
                reinterpret_cast<const char *>(std::to_address(it)),
                reinterpret_cast<const char *>(std::to_address(it)) + (end - it))
        )

    template <typename CRCType, std::size_t Width, CRCType Poly, CRCType Init, bool RefIn, bool RefOut, CRCType XOROut,
              std::ranges::contiguous_range R>
    requires std::ranges::sized_range<R> && detail::byte_like<std::ranges::range_value_t<R>>
    [[nodiscard]] CRC_STATIC_CALL_OPERATOR constexpr crc_base<CRCType, Width, Poly, Init, RefIn, RefOut, XOROut>
    operator()(const algorithm auto algo, const crc_base<CRCType, Width, Poly, Init, RefIn, RefOut, XOROut> crc, R&& r) CRC_CONST_CALL_OPERATOR
        CRC_RETURNS(process_fn::operator()(algo, crc, std::ranges::begin(r), std::ranges::end(r)))

    template <typename CRCType, std::size_t Width, CRCType Poly, CRCType Init, bool RefIn, bool RefOut, CRCType XOROut,
              std::contiguous_iterator I, std::sized_sentinel_for<I> S>
    requires detail::byte_like<std::iter_value_t<I>>
    [[nodiscard]] CRC_STATIC_CALL_OPERATOR constexpr crc_base<CRCType, Width, Poly, Init, RefIn, RefOut, XOROut>
    operator()(const crc_base<CRCType, Width, Poly, Init, RefIn, RefOut, XOROut> crc, I begin, S end) CRC_CONST_CALL_OPERATOR
        CRC_RETURNS(process_fn::operator()(algorithms::slice_by<8>, crc, std::move(begin), std::move(end)))

    template <typename CRCType, std::size_t Width, CRCType Poly, CRCType Init, bool RefIn, bool RefOut, CRCType XOROut,
              std::ranges::contiguous_range R>
    requires std::ranges::sized_range<R> && detail::byte_like<std::ranges::range_value_t<R>>
    [[nodiscard]] CRC_STATIC_CALL_OPERATOR constexpr crc_base<CRCType, Width, Poly, Init, RefIn, RefOut, XOROut>
    operator()(const crc_base<CRCType, Width, Poly, Init, RefIn, RefOut, XOROut> crc, R&& r) CRC_CONST_CALL_OPERATOR
        CRC_RETURNS(process_fn::operator()(crc, std::ranges::begin(r), std::ranges::end(r)))
};

struct finalize_fn {
    template <typename CRCType, std::size_t Width, CRCType Poly, CRCType Init, bool RefIn, bool RefOut, CRCType XOROut>
    [[nodiscard]] CRC_STATIC_CALL_OPERATOR constexpr CRCType
    operator()(crc_base<CRCType, Width, Poly, Init, RefIn, RefOut, XOROut> state) CRC_CONST_CALL_OPERATOR noexcept {
        if constexpr (Width < 8 && !RefIn) {
            state.m_crc >>= 8 - Width;
        }

        state.m_crc &= detail::bottom_n_mask<CRCType>(Width);

        if constexpr (RefIn != RefOut) {
            state.m_crc = detail::reflect(state.m_crc, Width);
        }

        return state.m_crc ^ XOROut;
    }
};

struct is_valid_fn {
    template <typename CRCType, std::size_t Width, CRCType Poly, CRCType Init, bool RefIn, bool RefOut, CRCType XOROut>
    [[nodiscard]] CRC_STATIC_CALL_OPERATOR constexpr bool
    operator()(crc_base<CRCType, Width, Poly, Init, RefIn, RefOut, XOROut> state) CRC_CONST_CALL_OPERATOR noexcept {
        CRC_STATIC23 constexpr CRCType residue {[] {
            CRCType residue_ {XOROut};

            for (std::size_t i {0}; i < Width; ++i) {
                residue_ = (residue_ << 1) ^ (detail::bit_is_set(residue_, Width - 1) ? Poly : 0);
            }

            if constexpr (RefIn) {
                return detail::reflect(residue_, Width);
            } else if constexpr (Width < 8) {
                return residue_ << (8 - Width);
            } else {
                return residue_;
            }
        }()};

        return (state.m_crc & detail::bottom_n_mask<CRCType>(Width)) == residue;
    }
};

} // namespace detail

CRC_EXPORT inline constexpr detail::process_fn process {};
CRC_EXPORT inline constexpr detail::finalize_fn finalize {};
CRC_EXPORT inline constexpr detail::is_valid_fn is_valid {};

CRC_EXPORT template <typename CRCType, std::size_t Width, CRCType Poly, CRCType Init, bool RefIn, bool RefOut, CRCType XOROut>
class crc : public detail::crc_base<CRCType, Width, Poly, Init, RefIn, RefOut, XOROut> {
private:
    template <typename WrappedFn>
    struct convenience_member_fn {
        template <std::contiguous_iterator I, std::sized_sentinel_for<I> S>
        requires detail::byte_like<std::iter_value_t<I>>
        [[nodiscard]] CRC_STATIC_CALL_OPERATOR constexpr auto
        operator()(const algorithm auto algo, I begin, S end) CRC_CONST_CALL_OPERATOR
            CRC_RETURNS(WrappedFn{}(process(algo, crc {}, std::move(begin), std::move(end))))

        template <std::ranges::contiguous_range R>
        requires std::ranges::sized_range<R> && detail::byte_like<std::ranges::range_value_t<R>>
        [[nodiscard]] CRC_STATIC_CALL_OPERATOR constexpr auto
        operator()(const algorithm auto algo, R&& r) CRC_CONST_CALL_OPERATOR
            CRC_RETURNS(convenience_member_fn::operator()(algo, std::ranges::begin(r), std::ranges::end(r)))

        template <std::contiguous_iterator I, std::sized_sentinel_for<I> S>
        requires detail::byte_like<std::iter_value_t<I>>
        [[nodiscard]] CRC_STATIC_CALL_OPERATOR constexpr auto
        operator()(I begin, S end) CRC_CONST_CALL_OPERATOR
            CRC_RETURNS(WrappedFn{}(process(crc {}, std::move(begin), std::move(end))))

        template <std::ranges::contiguous_range R>
        requires std::ranges::sized_range<R> && detail::byte_like<std::ranges::range_value_t<R>>
        [[nodiscard]] CRC_STATIC_CALL_OPERATOR constexpr auto
        operator()(R&& r) CRC_CONST_CALL_OPERATOR
            CRC_RETURNS(convenience_member_fn::operator()(std::ranges::begin(r), std::ranges::end(r)))
    };

public:
    static_assert(Width != 0);
    static_assert(std::numeric_limits<CRCType>::digits >= Width);
    static_assert((Poly & ~detail::bottom_n_mask<CRCType>(Width)) == 0);
    static_assert((Init & ~detail::bottom_n_mask<CRCType>(Width)) == 0);
    static_assert((XOROut & ~detail::bottom_n_mask<CRCType>(Width)) == 0);

    using crc_type = CRCType;
    static constexpr std::size_t width {Width};
    static constexpr CRCType poly {Poly};
    static constexpr CRCType init {Init};
    static constexpr bool refin {RefIn};
    static constexpr bool refout {RefOut};
    static constexpr CRCType xorout {XOROut};

    static constexpr convenience_member_fn<detail::finalize_fn> calculate {};
    static constexpr convenience_member_fn<detail::is_valid_fn> is_valid {};
};

// clang-format off

/// Parity bit.
CRC_EXPORT using crc1                    = crc<std::uint8_t /* TODO: make bool */,           1,      0x1,      0x0, false, false,      0x0>; // NOLINT(modernize-use-bool-literals)
CRC_EXPORT using crc3_gsm                = crc<std::uint8_t,   3,      0x3,      0x0, false, false,      0x7>; // academic
CRC_EXPORT using crc3_rohc               = crc<std::uint8_t,   3,      0x3,      0x7,  true,  true,      0x0>; // academic
CRC_EXPORT using crc4_g_704              = crc<std::uint8_t,   4,      0x3,      0x0,  true,  true,      0x0>; // academic
CRC_EXPORT using crc4_interlaken         = crc<std::uint8_t,   4,      0x3,      0xF, false, false,      0xF>; // academic
CRC_EXPORT using crc5_epc_c1g2           = crc<std::uint8_t,   5,     0x09,     0x09, false, false,     0x00>; // attested
CRC_EXPORT using crc5_g_704              = crc<std::uint8_t,   5,     0x15,     0x00,  true,  true,     0x00>; // academic
CRC_EXPORT using crc5_usb                = crc<std::uint8_t,   5,     0x05,     0x1F,  true,  true,     0x1F>; // confirmed
CRC_EXPORT using crc6_cdma2000_a         = crc<std::uint8_t,   6,     0x27,     0x3F, false, false,     0x00>; // attested
CRC_EXPORT using crc6_cdma2000_b         = crc<std::uint8_t,   6,     0x07,     0x3F, false, false,     0x00>; // academic
CRC_EXPORT using crc6_darc               = crc<std::uint8_t,   6,     0x19,     0x00,  true,  true,     0x00>; // attested
CRC_EXPORT using crc6_g_704              = crc<std::uint8_t,   6,     0x03,     0x00,  true,  true,     0x00>; // academic
CRC_EXPORT using crc6_gsm                = crc<std::uint8_t,   6,     0x2F,     0x00, false, false,     0x3F>; // academic
CRC_EXPORT using crc7_mmc                = crc<std::uint8_t,   7,     0x09,     0x00, false, false,     0x00>; // academic
CRC_EXPORT using crc7_rohc               = crc<std::uint8_t,   7,     0x4F,     0x7F,  true,  true,     0x00>; // academic
CRC_EXPORT using crc7_umts               = crc<std::uint8_t,   7,     0x45,     0x00, false, false,     0x00>; // academic
CRC_EXPORT using crc8_autosar            = crc<std::uint8_t,   8,     0x2F,     0xFF, false, false,     0xFF>; // attested
CRC_EXPORT using crc8_bluetooth          = crc<std::uint8_t,   8,     0xA7,     0x00,  true,  true,     0x00>; // attested
CRC_EXPORT using crc8_cdma2000           = crc<std::uint8_t,   8,     0x9B,     0xFF, false, false,     0x00>; // academic
CRC_EXPORT using crc8_darc               = crc<std::uint8_t,   8,     0x39,     0x00,  true,  true,     0x00>; // attested
CRC_EXPORT using crc8_dvb_s2             = crc<std::uint8_t,   8,     0xD5,     0x00, false, false,     0x00>; // academic
CRC_EXPORT using crc8_gsm_a              = crc<std::uint8_t,   8,     0x1D,     0x00, false, false,     0x00>; // academic
CRC_EXPORT using crc8_gsm_b              = crc<std::uint8_t,   8,     0x49,     0x00, false, false,     0xFF>; // academic
CRC_EXPORT using crc8_hitag              = crc<std::uint8_t,   8,     0x1D,     0xFF, false, false,     0x00>; // attested
CRC_EXPORT using crc8_i_432_1            = crc<std::uint8_t,   8,     0x07,     0x00, false, false,     0x55>; // academic
CRC_EXPORT using crc8_i_code             = crc<std::uint8_t,   8,     0x1D,     0xFD, false, false,     0x00>; // attested
CRC_EXPORT using crc8_lte                = crc<std::uint8_t,   8,     0x9B,     0x00, false, false,     0x00>; // academic
CRC_EXPORT using crc8_maxim_dow          = crc<std::uint8_t,   8,     0x31,     0x00,  true,  true,     0x00>; // attested
CRC_EXPORT using crc8_mifare_mad         = crc<std::uint8_t,   8,     0x1D,     0xC7, false, false,     0x00>; // attested
CRC_EXPORT using crc8_nrsc_5             = crc<std::uint8_t,   8,     0x31,     0xFF, false, false,     0x00>; // attested
CRC_EXPORT using crc8_opensafety         = crc<std::uint8_t,   8,     0x2F,     0x00, false, false,     0x00>; // attested
CRC_EXPORT using crc8_rohc               = crc<std::uint8_t,   8,     0x07,     0xFF,  true,  true,     0x00>; // academic
CRC_EXPORT using crc8_sae_j1850          = crc<std::uint8_t,   8,     0x1D,     0xFF, false, false,     0xFF>; // attested
CRC_EXPORT using crc8_smbus              = crc<std::uint8_t,   8,     0x07,     0x00, false, false,     0x00>; // attested
CRC_EXPORT using crc8_tech_3250          = crc<std::uint8_t,   8,     0x1D,     0xFF,  true,  true,     0x00>; // attested
CRC_EXPORT using crc8_wcdma              = crc<std::uint8_t,   8,     0x9B,     0x00,  true,  true,     0x00>; // third_party
CRC_EXPORT using crc10_atm               = crc<std::uint16_t, 10,    0x233,    0x000, false, false,    0x000>; // attested
CRC_EXPORT using crc10_cdma2000          = crc<std::uint16_t, 10,    0x3D9,    0x3FF, false, false,    0x000>; // academic
CRC_EXPORT using crc10_gsm               = crc<std::uint16_t, 10,    0x175,    0x000, false, false,    0x3FF>; // academic
CRC_EXPORT using crc11_flexray           = crc<std::uint16_t, 11,    0x385,    0x01A, false, false,    0x000>; // attested
CRC_EXPORT using crc11_umts              = crc<std::uint16_t, 11,    0x307,    0x000, false, false,    0x000>; // academic
CRC_EXPORT using crc12_cdma2000          = crc<std::uint16_t, 12,    0xF13,    0xFFF, false, false,    0x000>; // academic
CRC_EXPORT using crc12_dect              = crc<std::uint16_t, 12,    0x80F,    0x000, false, false,    0x000>; // academic
CRC_EXPORT using crc12_gsm               = crc<std::uint16_t, 12,    0xD31,    0x000, false, false,    0xFFF>; // academic
CRC_EXPORT using crc12_umts              = crc<std::uint16_t, 12,    0x80F,    0x000, false,  true,    0x000>; // academic
CRC_EXPORT using crc13_bbc               = crc<std::uint16_t, 13,   0x1CF5,   0x0000, false, false,   0x0000>; // attested
CRC_EXPORT using crc14_darc              = crc<std::uint16_t, 14,   0x0805,   0x0000,  true,  true,   0x0000>; // attested
CRC_EXPORT using crc14_gsm               = crc<std::uint16_t, 14,   0x202D,   0x0000, false, false,   0x3FFF>; // academic
CRC_EXPORT using crc15_can               = crc<std::uint16_t, 15,   0x4599,   0x0000, false, false,   0x0000>; // academic
CRC_EXPORT using crc15_mpt1327           = crc<std::uint16_t, 15,   0x6815,   0x0000, false, false,   0x0001>; // attested
CRC_EXPORT using crc16_arc               = crc<std::uint16_t, 16,   0x8005,   0x0000,  true,  true,   0x0000>; // attested
CRC_EXPORT using crc16_cdma2000          = crc<std::uint16_t, 16,   0xC867,   0xFFFF, false, false,   0x0000>; // academic
CRC_EXPORT using crc16_cms               = crc<std::uint16_t, 16,   0x8005,   0xFFFF, false, false,   0x0000>; // third_party
CRC_EXPORT using crc16_dds_110           = crc<std::uint16_t, 16,   0x8005,   0x800D, false, false,   0x0000>; // attested
CRC_EXPORT using crc16_dect_r            = crc<std::uint16_t, 16,   0x0589,   0x0000, false, false,   0x0001>; // attested
CRC_EXPORT using crc16_dect_x            = crc<std::uint16_t, 16,   0x0589,   0x0000, false, false,   0x0000>; // attested
CRC_EXPORT using crc16_dnp               = crc<std::uint16_t, 16,   0x3D65,   0x0000,  true,  true,   0xFFFF>; // confirmed
CRC_EXPORT using crc16_en_13757          = crc<std::uint16_t, 16,   0x3D65,   0x0000, false, false,   0xFFFF>; // confirmed
CRC_EXPORT using crc16_genibus           = crc<std::uint16_t, 16,   0x1021,   0xFFFF, false, false,   0xFFFF>; // attested
CRC_EXPORT using crc16_gsm               = crc<std::uint16_t, 16,   0x1021,   0x0000, false, false,   0xFFFF>; // attested
CRC_EXPORT using crc16_ibm_3740          = crc<std::uint16_t, 16,   0x1021,   0xFFFF, false, false,   0x0000>; // attested
CRC_EXPORT using crc16_ibm_sdlc          = crc<std::uint16_t, 16,   0x1021,   0xFFFF,  true,  true,   0xFFFF>; // attested
CRC_EXPORT using crc16_iso_iec_14443_3_a = crc<std::uint16_t, 16,   0x1021,   0xC6C6,  true,  true,   0x0000>; // attested
CRC_EXPORT using crc16_kermit            = crc<std::uint16_t, 16,   0x1021,   0x0000,  true,  true,   0x0000>; // attested
CRC_EXPORT using crc16_lj1200            = crc<std::uint16_t, 16,   0x6F63,   0x0000, false, false,   0x0000>; // third_party
CRC_EXPORT using crc16_m17               = crc<std::uint16_t, 16,   0x5935,   0xFFFF, false, false,   0x0000>; // attested
CRC_EXPORT using crc16_maxim_dow         = crc<std::uint16_t, 16,   0x8005,   0x0000,  true,  true,   0xFFFF>; // attested
CRC_EXPORT using crc16_mcrf4xx           = crc<std::uint16_t, 16,   0x1021,   0xFFFF,  true,  true,   0x0000>; // attested
CRC_EXPORT using crc16_modbus            = crc<std::uint16_t, 16,   0x8005,   0xFFFF,  true,  true,   0x0000>; // attested
CRC_EXPORT using crc16_nrsc_5            = crc<std::uint16_t, 16,   0x080B,   0xFFFF,  true,  true,   0x0000>; // attested
CRC_EXPORT using crc16_opensafety_a      = crc<std::uint16_t, 16,   0x5935,   0x0000, false, false,   0x0000>; // attested
CRC_EXPORT using crc16_opensafety_b      = crc<std::uint16_t, 16,   0x755B,   0x0000, false, false,   0x0000>; // attested
CRC_EXPORT using crc16_profibus          = crc<std::uint16_t, 16,   0x1DCF,   0xFFFF, false, false,   0xFFFF>; // attested
CRC_EXPORT using crc16_riello            = crc<std::uint16_t, 16,   0x1021,   0xB2AA,  true,  true,   0x0000>; // third_party
CRC_EXPORT using crc16_spi_fujitsu       = crc<std::uint16_t, 16,   0x1021,   0x1D0F, false, false,   0x0000>; // attested
CRC_EXPORT using crc16_t10_dif           = crc<std::uint16_t, 16,   0x8BB7,   0x0000, false, false,   0x0000>; // attested
CRC_EXPORT using crc16_teledisk          = crc<std::uint16_t, 16,   0xA097,   0x0000, false, false,   0x0000>; // confirmed
CRC_EXPORT using crc16_tms37157          = crc<std::uint16_t, 16,   0x1021,   0x89EC,  true,  true,   0x0000>; // attested
CRC_EXPORT using crc16_umts              = crc<std::uint16_t, 16,   0x8005,   0x0000, false, false,   0x0000>; // attested
CRC_EXPORT using crc16_usb               = crc<std::uint16_t, 16,   0x8005,   0xFFFF,  true,  true,   0xFFFF>; // confirmed
CRC_EXPORT using crc16_xmodem            = crc<std::uint16_t, 16,   0x1021,   0x0000, false, false,   0x0000>; // attested
CRC_EXPORT using crc17_can_fd            = crc<std::uint32_t, 17,  0x1685B,  0x00000, false, false,  0x00000>; // academic
CRC_EXPORT using crc21_can_fd            = crc<std::uint32_t, 21, 0x102899, 0x000000, false, false, 0x000000>; // academic
CRC_EXPORT using crc24_ble               = crc<std::uint32_t, 24, 0x00065B, 0x555555,  true,  true, 0x000000>; // attested
CRC_EXPORT using crc24_flexray_a         = crc<std::uint32_t, 24, 0x5D6DCB, 0xFEDCBA, false, false, 0x000000>; // attested
CRC_EXPORT using crc24_flexray_b         = crc<std::uint32_t, 24, 0x5D6DCB, 0xABCDEF, false, false, 0x000000>; // attested
CRC_EXPORT using crc24_interlaken        = crc<std::uint32_t, 24, 0x328B63, 0xFFFFFF, false, false, 0xFFFFFF>; // academic
CRC_EXPORT using crc24_lte_a             = crc<std::uint32_t, 24, 0x864CFB, 0x000000, false, false, 0x000000>; // academic
CRC_EXPORT using crc24_lte_b             = crc<std::uint32_t, 24, 0x800063, 0x000000, false, false, 0x000000>; // academic
CRC_EXPORT using crc24_openpgp           = crc<std::uint32_t, 24, 0x864CFB, 0xB704CE, false, false, 0x000000>; // attested
CRC_EXPORT using crc24_os_9              = crc<std::uint32_t, 24, 0x800063, 0xFFFFFF, false, false, 0xFFFFFF>; // attested
CRC_EXPORT using crc30_cdma              = crc<std::uint32_t, 30, 0x2030B9C7, 0x3FFFFFFF, false, false, 0x3FFFFFFF>; // academic
CRC_EXPORT using crc31_philips           = crc<std::uint32_t, 31, 0x04C11DB7, 0x7FFFFFFF, false, false, 0x7FFFFFFF>; // confirmed
CRC_EXPORT using crc32_aixm              = crc<std::uint32_t, 32, 0x814141AB, 0x00000000, false, false, 0x00000000>; // attested
CRC_EXPORT using crc32_autosar           = crc<std::uint32_t, 32, 0xF4ACFB13, 0xFFFFFFFF,  true,  true, 0xFFFFFFFF>; // attested
CRC_EXPORT using crc32_base91_d          = crc<std::uint32_t, 32, 0xA833982B, 0xFFFFFFFF,  true,  true, 0xFFFFFFFF>; // confirmed
CRC_EXPORT using crc32                   = crc<std::uint32_t, 32, 0x04C11DB7, 0xFFFFFFFF, false, false, 0xFFFFFFFF>; // attested
CRC_EXPORT using crc32_cd_rom_edc        = crc<std::uint32_t, 32, 0x8001801B, 0x00000000,  true,  true, 0x00000000>; // academic
CRC_EXPORT using crc32_cksum             = crc<std::uint32_t, 32, 0x04C11DB7, 0x00000000, false, false, 0xFFFFFFFF>; // attested
CRC_EXPORT using crc32c                  = crc<std::uint32_t, 32, 0x1EDC6F41, 0xFFFFFFFF,  true,  true, 0xFFFFFFFF>; // attested
CRC_EXPORT using crc32_iso_hdlc          = crc<std::uint32_t, 32, 0x04C11DB7, 0xFFFFFFFF,  true,  true, 0xFFFFFFFF>; // attested
CRC_EXPORT using crc32_jamcrc            = crc<std::uint32_t, 32, 0x04C11DB7, 0xFFFFFFFF,  true,  true, 0x00000000>; // confirmed
CRC_EXPORT using crc32_mef               = crc<std::uint32_t, 32, 0x741B8CD7, 0xFFFFFFFF,  true,  true, 0x00000000>; // attested
CRC_EXPORT using crc32_mpeg2             = crc<std::uint32_t, 32, 0x04C11DB7, 0xFFFFFFFF, false, false, 0x00000000>; // attested
CRC_EXPORT using crc32_xfer              = crc<std::uint32_t, 32, 0x000000AF, 0x00000000, false, false, 0x00000000>; // confirmed
CRC_EXPORT using crc40_gsm               = crc<std::uint64_t, 40, 0x0004820009, 0x0000000000, false, false, 0xFFFFFFFFFF>; // academic
CRC_EXPORT using crc64_ecma_182          = crc<std::uint64_t, 64, 0x42F0E1EBA9EA3693, 0x0000000000000000, false, false, 0x0000000000000000>; // academic
CRC_EXPORT using crc64_go_iso            = crc<std::uint64_t, 64, 0x000000000000001B, 0xFFFFFFFFFFFFFFFF,  true,  true, 0xFFFFFFFFFFFFFFFF>; // confirmed
CRC_EXPORT using crc64_ms                = crc<std::uint64_t, 64, 0x259C84CBA6426349, 0xFFFFFFFFFFFFFFFF,  true,  true, 0x0000000000000000>; // attested
CRC_EXPORT using crc64_nvme              = crc<std::uint64_t, 64, 0xAD93D23594C93659, 0xFFFFFFFFFFFFFFFF,  true,  true, 0xFFFFFFFFFFFFFFFF>; // attested
CRC_EXPORT using crc64_redis             = crc<std::uint64_t, 64, 0xAD93D23594C935A9, 0x0000000000000000,  true,  true, 0x0000000000000000>; // academic
CRC_EXPORT using crc64_we                = crc<std::uint64_t, 64, 0x42F0E1EBA9EA3693, 0xFFFFFFFFFFFFFFFF, false, false, 0xFFFFFFFFFFFFFFFF>; // confirmed
CRC_EXPORT using crc64_xz                = crc<std::uint64_t, 64, 0x42F0E1EBA9EA3693, 0xFFFFFFFFFFFFFFFF,  true,  true, 0xFFFFFFFFFFFFFFFF>; // attested
// CRC_EXPORT using crc82_darc = crc<std::uint82_t, 82, std::bitset<82>{"0000000000110000100011000000000100010001000000010001010000000001010001000000010000010001"}, 0x0,  true,  true, >; // attested
// clang-format on

} // namespace crc

#undef CRC_EXPORT
#undef CRC_RETURNS
#undef CRC_STATIC_CALL_OPERATOR
#undef CRC_CONST_CALL_OPERATOR
#undef CRC_STATIC23

#endif // CRC_JUST_THE_INCLUDES

#endif // CRC_HPP_INCLUDED
