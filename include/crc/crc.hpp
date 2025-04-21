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

namespace crc::inline v1 {

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
    // using a pseudo-SIMD algorithm.
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

template <typename T, std::endian E, std::input_iterator I>
[[nodiscard]] constexpr T read(I it) {
    std::array<char, sizeof(T)> bytes; // NOLINT(cppcoreguidelines-pro-type-member-init)
    for (std::size_t i {0}; i < sizeof(T); ++i, ++it) {
        bytes[E == std::endian::native ? i : sizeof(T) - i] = static_cast<char>(*it);
    }
    return std::bit_cast<T>(bytes);
}

template <typename CRCType, std::size_t Width, CRCType Poly, bool RefIn>
inline constexpr auto table {[] {
    std::array<CRCType, 256> m_table; // NOLINT(cppcoreguidelines-pro-type-member-init)
    for (std::size_t i {0}; i < 256; ++i) {
        auto r {static_cast<CRCType>((RefIn ? detail::reflect(i, 8) : i) << (Width - 8))};
        for (std::size_t j {0}; j < 8; ++j) {
            r = (r << 1) ^ (detail::bit_is_set(r, Width - 1) ? Poly : 0);
        }

        r &= detail::bottom_n_mask<CRCType>(Width);

        if constexpr (RefIn) {
            r = detail::reflect(r, Width);
        }

        m_table[i] = r;
    }
    return m_table;
}()};

template <typename CRCType, std::size_t Width, CRCType Poly, bool RefIn>
struct crc_base {
    [[nodiscard]] static constexpr CRCType process(CRCType crc, std::ranges::contiguous_range auto&& r) {
        for (const std::uint8_t byte : r) {
            if constexpr (RefIn) {
                crc = (crc >> 8) ^ detail::table<CRCType, Width, Poly, RefIn>[
                    static_cast<std::uint8_t>(crc) ^ byte];
            } else {
                crc = (crc << 8) ^ detail::table<CRCType, Width, Poly, RefIn>[
                    static_cast<std::uint8_t>(crc >> (Width - 8)) ^ byte];
            }
        }
        return crc;
    }
};

} // namespace detail

CRC_EXPORT template<
    typename CRCType,
    std::size_t Width,
    CRCType Poly,
    CRCType Init,
    bool RefIn,
    bool RefOut,
    CRCType XOROut>
class crc : public detail::crc_base<CRCType, std::max<std::size_t>(Width, 8), Poly << (8 - std::min<CRCType>(Width, 8)), RefIn> {
public:
    using crc_type = CRCType;
    static constexpr std::size_t width {Width};
    static constexpr CRCType poly {Poly};
    static constexpr CRCType init {Init};
    static constexpr bool refin {RefIn};
    static constexpr bool refout {RefOut};
    static constexpr CRCType xorout {XOROut};

    [[nodiscard]] static constexpr CRCType initialize() noexcept {
        if constexpr (RefIn) {
            return detail::reflect(Init, Width);
        } else if constexpr (Width < 8) {
            return Init << (8 - Width);
        } else {
            return Init;
        }
    }

    [[nodiscard]] static constexpr CRCType finalize(CRCType crc) noexcept {
        if constexpr (Width < 8 && !RefIn) {
            crc >>= 8 - Width;
        }

        crc &= detail::bottom_n_mask<CRCType>(Width);

        if constexpr (RefIn != RefOut) {
            crc = detail::reflect(crc, Width);
        }

        return crc ^ XOROut;
    }

    template <std::ranges::contiguous_range R>
    requires detail::byte_like<std::ranges::range_value_t<R>>
    [[nodiscard]] CRC_STATIC_CALL_OPERATOR constexpr CRCType operator()(R&& r) CRC_CONST_CALL_OPERATOR
        CRC_RETURNS(crc::finalize(crc::process(crc::initialize(), std::forward<R>(r))))

    template <std::contiguous_iterator I, std::sentinel_for<I> S>
    requires detail::byte_like<std::iter_value_t<I>>
    [[nodiscard]] CRC_STATIC_CALL_OPERATOR constexpr CRCType operator()(I begin, S end) CRC_CONST_CALL_OPERATOR
        CRC_RETURNS(crc::operator()(std::ranges::subrange(std::move(begin), std::move(end))))
};

// clang-format off

/// Parity bit.
CRC_EXPORT inline constexpr auto crc1                    {crc<std::uint8_t /* TODO: make bool */,           1,      0x1,      0x0, false, false,      0x0>{}}; // NOLINT(modernize-use-bool-literals)
CRC_EXPORT inline constexpr auto crc3_gsm                {crc<std::uint8_t,   3,      0x3,      0x0, false, false,      0x7>{}}; // academic
CRC_EXPORT inline constexpr auto crc3_rohc               {crc<std::uint8_t,   3,      0x3,      0x7,  true,  true,      0x0>{}}; // academic
CRC_EXPORT inline constexpr auto crc4_g_704              {crc<std::uint8_t,   4,      0x3,      0x0,  true,  true,      0x0>{}}; // academic
CRC_EXPORT inline constexpr auto crc4_interlaken         {crc<std::uint8_t,   4,      0x3,      0xF, false, false,      0xF>{}}; // academic
CRC_EXPORT inline constexpr auto crc5_epc_c1g2           {crc<std::uint8_t,   5,     0x09,     0x09, false, false,     0x00>{}}; // attested
CRC_EXPORT inline constexpr auto crc5_g_704              {crc<std::uint8_t,   5,     0x15,     0x00,  true,  true,     0x00>{}}; // academic
CRC_EXPORT inline constexpr auto crc5_usb                {crc<std::uint8_t,   5,     0x05,     0x1F,  true,  true,     0x1F>{}}; // confirmed
CRC_EXPORT inline constexpr auto crc6_cdma2000_a         {crc<std::uint8_t,   6,     0x27,     0x3F, false, false,     0x00>{}}; // attested
CRC_EXPORT inline constexpr auto crc6_cdma2000_b         {crc<std::uint8_t,   6,     0x07,     0x3F, false, false,     0x00>{}}; // academic
CRC_EXPORT inline constexpr auto crc6_darc               {crc<std::uint8_t,   6,     0x19,     0x00,  true,  true,     0x00>{}}; // attested
CRC_EXPORT inline constexpr auto crc6_g_704              {crc<std::uint8_t,   6,     0x03,     0x00,  true,  true,     0x00>{}}; // academic
CRC_EXPORT inline constexpr auto crc6_gsm                {crc<std::uint8_t,   6,     0x2F,     0x00, false, false,     0x3F>{}}; // academic
CRC_EXPORT inline constexpr auto crc7_mmc                {crc<std::uint8_t,   7,     0x09,     0x00, false, false,     0x00>{}}; // academic
CRC_EXPORT inline constexpr auto crc7_rohc               {crc<std::uint8_t,   7,     0x4F,     0x7F,  true,  true,     0x00>{}}; // academic
CRC_EXPORT inline constexpr auto crc7_umts               {crc<std::uint8_t,   7,     0x45,     0x00, false, false,     0x00>{}}; // academic
CRC_EXPORT inline constexpr auto crc8_autosar            {crc<std::uint8_t,   8,     0x2F,     0xFF, false, false,     0xFF>{}}; // attested
CRC_EXPORT inline constexpr auto crc8_bluetooth          {crc<std::uint8_t,   8,     0xA7,     0x00,  true,  true,     0x00>{}}; // attested
CRC_EXPORT inline constexpr auto crc8_cdma2000           {crc<std::uint8_t,   8,     0x9B,     0xFF, false, false,     0x00>{}}; // academic
CRC_EXPORT inline constexpr auto crc8_darc               {crc<std::uint8_t,   8,     0x39,     0x00,  true,  true,     0x00>{}}; // attested
CRC_EXPORT inline constexpr auto crc8_dvb_s2             {crc<std::uint8_t,   8,     0xD5,     0x00, false, false,     0x00>{}}; // academic
CRC_EXPORT inline constexpr auto crc8_gsm_a              {crc<std::uint8_t,   8,     0x1D,     0x00, false, false,     0x00>{}}; // academic
CRC_EXPORT inline constexpr auto crc8_gsm_b              {crc<std::uint8_t,   8,     0x49,     0x00, false, false,     0xFF>{}}; // academic
CRC_EXPORT inline constexpr auto crc8_hitag              {crc<std::uint8_t,   8,     0x1D,     0xFF, false, false,     0x00>{}}; // attested
CRC_EXPORT inline constexpr auto crc8_i_432_1            {crc<std::uint8_t,   8,     0x07,     0x00, false, false,     0x55>{}}; // academic
CRC_EXPORT inline constexpr auto crc8_i_code             {crc<std::uint8_t,   8,     0x1D,     0xFD, false, false,     0x00>{}}; // attested
CRC_EXPORT inline constexpr auto crc8_lte                {crc<std::uint8_t,   8,     0x9B,     0x00, false, false,     0x00>{}}; // academic
CRC_EXPORT inline constexpr auto crc8_maxim_dow          {crc<std::uint8_t,   8,     0x31,     0x00,  true,  true,     0x00>{}}; // attested
CRC_EXPORT inline constexpr auto crc8_mifare_mad         {crc<std::uint8_t,   8,     0x1D,     0xC7, false, false,     0x00>{}}; // attested
CRC_EXPORT inline constexpr auto crc8_nrsc_5             {crc<std::uint8_t,   8,     0x31,     0xFF, false, false,     0x00>{}}; // attested
CRC_EXPORT inline constexpr auto crc8_opensafety         {crc<std::uint8_t,   8,     0x2F,     0x00, false, false,     0x00>{}}; // attested
CRC_EXPORT inline constexpr auto crc8_rohc               {crc<std::uint8_t,   8,     0x07,     0xFF,  true,  true,     0x00>{}}; // academic
CRC_EXPORT inline constexpr auto crc8_sae_j1850          {crc<std::uint8_t,   8,     0x1D,     0xFF, false, false,     0xFF>{}}; // attested
CRC_EXPORT inline constexpr auto crc8_smbus              {crc<std::uint8_t,   8,     0x07,     0x00, false, false,     0x00>{}}; // attested
CRC_EXPORT inline constexpr auto crc8_tech_3250          {crc<std::uint8_t,   8,     0x1D,     0xFF,  true,  true,     0x00>{}}; // attested
CRC_EXPORT inline constexpr auto crc8_wcdma              {crc<std::uint8_t,   8,     0x9B,     0x00,  true,  true,     0x00>{}}; // third_party
CRC_EXPORT inline constexpr auto crc10_atm               {crc<std::uint16_t, 10,    0x233,    0x000, false, false,    0x000>{}}; // attested
CRC_EXPORT inline constexpr auto crc10_cdma2000          {crc<std::uint16_t, 10,    0x3D9,    0x3FF, false, false,    0x000>{}}; // academic
CRC_EXPORT inline constexpr auto crc10_gsm               {crc<std::uint16_t, 10,    0x175,    0x000, false, false,    0x3FF>{}}; // academic
CRC_EXPORT inline constexpr auto crc11_flexray           {crc<std::uint16_t, 11,    0x385,    0x01A, false, false,    0x000>{}}; // attested
CRC_EXPORT inline constexpr auto crc11_umts              {crc<std::uint16_t, 11,    0x307,    0x000, false, false,    0x000>{}}; // academic
CRC_EXPORT inline constexpr auto crc12_cdma2000          {crc<std::uint16_t, 12,    0xF13,    0xFFF, false, false,    0x000>{}}; // academic
CRC_EXPORT inline constexpr auto crc12_dect              {crc<std::uint16_t, 12,    0x80F,    0x000, false, false,    0x000>{}}; // academic
CRC_EXPORT inline constexpr auto crc12_gsm               {crc<std::uint16_t, 12,    0xD31,    0x000, false, false,    0xFFF>{}}; // academic
CRC_EXPORT inline constexpr auto crc12_umts              {crc<std::uint16_t, 12,    0x80F,    0x000, false,  true,    0x000>{}}; // academic
CRC_EXPORT inline constexpr auto crc13_bbc               {crc<std::uint16_t, 13,   0x1CF5,   0x0000, false, false,   0x0000>{}}; // attested
CRC_EXPORT inline constexpr auto crc14_darc              {crc<std::uint16_t, 14,   0x0805,   0x0000,  true,  true,   0x0000>{}}; // attested
CRC_EXPORT inline constexpr auto crc14_gsm               {crc<std::uint16_t, 14,   0x202D,   0x0000, false, false,   0x3FFF>{}}; // academic
CRC_EXPORT inline constexpr auto crc15_can               {crc<std::uint16_t, 15,   0x4599,   0x0000, false, false,   0x0000>{}}; // academic
CRC_EXPORT inline constexpr auto crc15_mpt1327           {crc<std::uint16_t, 15,   0x6815,   0x0000, false, false,   0x0001>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_arc               {crc<std::uint16_t, 16,   0x8005,   0x0000,  true,  true,   0x0000>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_cdma2000          {crc<std::uint16_t, 16,   0xC867,   0xFFFF, false, false,   0x0000>{}}; // academic
CRC_EXPORT inline constexpr auto crc16_cms               {crc<std::uint16_t, 16,   0x8005,   0xFFFF, false, false,   0x0000>{}}; // third_party
CRC_EXPORT inline constexpr auto crc16_dds_110           {crc<std::uint16_t, 16,   0x8005,   0x800D, false, false,   0x0000>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_dect_r            {crc<std::uint16_t, 16,   0x0589,   0x0000, false, false,   0x0001>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_dect_x            {crc<std::uint16_t, 16,   0x0589,   0x0000, false, false,   0x0000>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_dnp               {crc<std::uint16_t, 16,   0x3D65,   0x0000,  true,  true,   0xFFFF>{}}; // confirmed
CRC_EXPORT inline constexpr auto crc16_en_13757          {crc<std::uint16_t, 16,   0x3D65,   0x0000, false, false,   0xFFFF>{}}; // confirmed
CRC_EXPORT inline constexpr auto crc16_genibus           {crc<std::uint16_t, 16,   0x1021,   0xFFFF, false, false,   0xFFFF>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_gsm               {crc<std::uint16_t, 16,   0x1021,   0x0000, false, false,   0xFFFF>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_ibm_3740          {crc<std::uint16_t, 16,   0x1021,   0xFFFF, false, false,   0x0000>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_ibm_sdlc          {crc<std::uint16_t, 16,   0x1021,   0xFFFF,  true,  true,   0xFFFF>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_iso_iec_14443_3_a {crc<std::uint16_t, 16,   0x1021,   0xC6C6,  true,  true,   0x0000>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_kermit            {crc<std::uint16_t, 16,   0x1021,   0x0000,  true,  true,   0x0000>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_lj1200            {crc<std::uint16_t, 16,   0x6F63,   0x0000, false, false,   0x0000>{}}; // third_party
CRC_EXPORT inline constexpr auto crc16_m17               {crc<std::uint16_t, 16,   0x5935,   0xFFFF, false, false,   0x0000>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_maxim_dow         {crc<std::uint16_t, 16,   0x8005,   0x0000,  true,  true,   0xFFFF>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_mcrf4xx           {crc<std::uint16_t, 16,   0x1021,   0xFFFF,  true,  true,   0x0000>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_modbus            {crc<std::uint16_t, 16,   0x8005,   0xFFFF,  true,  true,   0x0000>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_nrsc_5            {crc<std::uint16_t, 16,   0x080B,   0xFFFF,  true,  true,   0x0000>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_opensafety_a      {crc<std::uint16_t, 16,   0x5935,   0x0000, false, false,   0x0000>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_opensafety_b      {crc<std::uint16_t, 16,   0x755B,   0x0000, false, false,   0x0000>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_profibus          {crc<std::uint16_t, 16,   0x1DCF,   0xFFFF, false, false,   0xFFFF>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_riello            {crc<std::uint16_t, 16,   0x1021,   0xB2AA,  true,  true,   0x0000>{}}; // third_party
CRC_EXPORT inline constexpr auto crc16_spi_fujitsu       {crc<std::uint16_t, 16,   0x1021,   0x1D0F, false, false,   0x0000>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_t10_dif           {crc<std::uint16_t, 16,   0x8BB7,   0x0000, false, false,   0x0000>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_teledisk          {crc<std::uint16_t, 16,   0xA097,   0x0000, false, false,   0x0000>{}}; // confirmed
CRC_EXPORT inline constexpr auto crc16_tms37157          {crc<std::uint16_t, 16,   0x1021,   0x89EC,  true,  true,   0x0000>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_umts              {crc<std::uint16_t, 16,   0x8005,   0x0000, false, false,   0x0000>{}}; // attested
CRC_EXPORT inline constexpr auto crc16_usb               {crc<std::uint16_t, 16,   0x8005,   0xFFFF,  true,  true,   0xFFFF>{}}; // confirmed
CRC_EXPORT inline constexpr auto crc16_xmodem            {crc<std::uint16_t, 16,   0x1021,   0x0000, false, false,   0x0000>{}}; // attested
CRC_EXPORT inline constexpr auto crc17_can_fd            {crc<std::uint32_t, 17,  0x1685B,  0x00000, false, false,  0x00000>{}}; // academic
CRC_EXPORT inline constexpr auto crc21_can_fd            {crc<std::uint32_t, 21, 0x102899, 0x000000, false, false, 0x000000>{}}; // academic
CRC_EXPORT inline constexpr auto crc24_ble               {crc<std::uint32_t, 24, 0x00065B, 0x555555,  true,  true, 0x000000>{}}; // attested
CRC_EXPORT inline constexpr auto crc24_flexray_a         {crc<std::uint32_t, 24, 0x5D6DCB, 0xFEDCBA, false, false, 0x000000>{}}; // attested
CRC_EXPORT inline constexpr auto crc24_flexray_b         {crc<std::uint32_t, 24, 0x5D6DCB, 0xABCDEF, false, false, 0x000000>{}}; // attested
CRC_EXPORT inline constexpr auto crc24_interlaken        {crc<std::uint32_t, 24, 0x328B63, 0xFFFFFF, false, false, 0xFFFFFF>{}}; // academic
CRC_EXPORT inline constexpr auto crc24_lte_a             {crc<std::uint32_t, 24, 0x864CFB, 0x000000, false, false, 0x000000>{}}; // academic
CRC_EXPORT inline constexpr auto crc24_lte_b             {crc<std::uint32_t, 24, 0x800063, 0x000000, false, false, 0x000000>{}}; // academic
CRC_EXPORT inline constexpr auto crc24_openpgp           {crc<std::uint32_t, 24, 0x864CFB, 0xB704CE, false, false, 0x000000>{}}; // attested
CRC_EXPORT inline constexpr auto crc24_os_9              {crc<std::uint32_t, 24, 0x800063, 0xFFFFFF, false, false, 0xFFFFFF>{}}; // attested
CRC_EXPORT inline constexpr auto crc30_cdma              {crc<std::uint32_t, 30, 0x2030B9C7, 0x3FFFFFFF, false, false, 0x3FFFFFFF>{}}; // academic
CRC_EXPORT inline constexpr auto crc31_philips           {crc<std::uint32_t, 31, 0x04C11DB7, 0x7FFFFFFF, false, false, 0x7FFFFFFF>{}}; // confirmed
CRC_EXPORT inline constexpr auto crc32_aixm              {crc<std::uint32_t, 32, 0x814141AB, 0x00000000, false, false, 0x00000000>{}}; // attested
CRC_EXPORT inline constexpr auto crc32_autosar           {crc<std::uint32_t, 32, 0xF4ACFB13, 0xFFFFFFFF,  true,  true, 0xFFFFFFFF>{}}; // attested
CRC_EXPORT inline constexpr auto crc32_base91_d          {crc<std::uint32_t, 32, 0xA833982B, 0xFFFFFFFF,  true,  true, 0xFFFFFFFF>{}}; // confirmed
CRC_EXPORT inline constexpr auto crc32                   {crc<std::uint32_t, 32, 0x04C11DB7, 0xFFFFFFFF, false, false, 0xFFFFFFFF>{}}; // attested
CRC_EXPORT inline constexpr auto crc32_cd_rom_edc        {crc<std::uint32_t, 32, 0x8001801B, 0x00000000,  true,  true, 0x00000000>{}}; // academic
CRC_EXPORT inline constexpr auto crc32_cksum             {crc<std::uint32_t, 32, 0x04C11DB7, 0x00000000, false, false, 0xFFFFFFFF>{}}; // attested
CRC_EXPORT inline constexpr auto crc32c                  {crc<std::uint32_t, 32, 0x1EDC6F41, 0xFFFFFFFF,  true,  true, 0xFFFFFFFF>{}}; // attested
CRC_EXPORT inline constexpr auto crc32_iso_hdlc          {crc<std::uint32_t, 32, 0x04C11DB7, 0xFFFFFFFF,  true,  true, 0xFFFFFFFF>{}}; // attested
CRC_EXPORT inline constexpr auto crc32_jamcrc            {crc<std::uint32_t, 32, 0x04C11DB7, 0xFFFFFFFF,  true,  true, 0x00000000>{}}; // confirmed
CRC_EXPORT inline constexpr auto crc32_mef               {crc<std::uint32_t, 32, 0x741B8CD7, 0xFFFFFFFF,  true,  true, 0x00000000>{}}; // attested
CRC_EXPORT inline constexpr auto crc32_mpeg2             {crc<std::uint32_t, 32, 0x04C11DB7, 0xFFFFFFFF, false, false, 0x00000000>{}}; // attested
CRC_EXPORT inline constexpr auto crc32_xfer              {crc<std::uint32_t, 32, 0x000000AF, 0x00000000, false, false, 0x00000000>{}}; // confirmed
CRC_EXPORT inline constexpr auto crc40_gsm               {crc<std::uint64_t, 40, 0x0004820009, 0x0000000000, false, false, 0xFFFFFFFFFF>{}}; // academic
CRC_EXPORT inline constexpr auto crc64_ecma_182          {crc<std::uint64_t, 64, 0x42F0E1EBA9EA3693, 0x0000000000000000, false, false, 0x0000000000000000>{}}; // academic
CRC_EXPORT inline constexpr auto crc64_go_iso            {crc<std::uint64_t, 64, 0x000000000000001B, 0xFFFFFFFFFFFFFFFF,  true,  true, 0xFFFFFFFFFFFFFFFF>{}}; // confirmed
CRC_EXPORT inline constexpr auto crc64_ms                {crc<std::uint64_t, 64, 0x259C84CBA6426349, 0xFFFFFFFFFFFFFFFF,  true,  true, 0x0000000000000000>{}}; // attested
CRC_EXPORT inline constexpr auto crc64_nvme              {crc<std::uint64_t, 64, 0xAD93D23594C93659, 0xFFFFFFFFFFFFFFFF,  true,  true, 0xFFFFFFFFFFFFFFFF>{}}; // attested
CRC_EXPORT inline constexpr auto crc64_redis             {crc<std::uint64_t, 64, 0xAD93D23594C935A9, 0x0000000000000000,  true,  true, 0x0000000000000000>{}}; // academic
CRC_EXPORT inline constexpr auto crc64_we                {crc<std::uint64_t, 64, 0x42F0E1EBA9EA3693, 0xFFFFFFFFFFFFFFFF, false, false, 0xFFFFFFFFFFFFFFFF>{}}; // confirmed
CRC_EXPORT inline constexpr auto crc64_xz                {crc<std::uint64_t, 64, 0x42F0E1EBA9EA3693, 0xFFFFFFFFFFFFFFFF,  true,  true, 0xFFFFFFFFFFFFFFFF>{}}; // attested
// CRC_EXPORT inline constexpr auto crc82_darc {crc<std::uint82_t, 82, std::bitset<82>{"0000000000110000100011000000000100010001000000010001010000000001010001000000010000010001"}, 0x0,  true,  true, >{}}; // attested
// clang-format on

} // namespace crc

#undef CRC_EXPORT
#undef CRC_RETURNS
#undef CRC_STATIC_CALL_OPERATOR
#undef CRC_CONST_CALL_OPERATOR

#endif // CRC_JUST_THE_INCLUDES

#endif // CRC_HPP_INCLUDED
