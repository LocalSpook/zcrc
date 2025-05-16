// SPDX-License-Identifier: MIT

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <string_view>
#include <type_traits>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

// <ugly_code warning="Watch out!">

#define CRC_JUST_THE_INCLUDES
#include <crc/crc.hpp>
#undef CRC_JUST_THE_INCLUDES
#undef CRC_HPP_INCLUDED

// We want to test both the header and the module.
// If we naively include/import both, we'll get duplicate symbols
// in the crc:: namespace. So, we wrap the header symbols in an
// additional namespace.
namespace header { // NOLINT(modernize-concat-nested-namespaces) (lol)

#include <crc/crc.hpp>

} // namespace header

#define CHECK_HEADER(...) SECTION("header") { \
        namespace crc = header::crc;          \
        static_assert(__VA_ARGS__);           \
        CHECK(__VA_ARGS__);                   \
    }

#define CHECK_MODULE(...) SECTION("module") { \
        static_assert(__VA_ARGS__);           \
        CHECK(__VA_ARGS__);                   \
    }

#ifdef CRC_TEST_MODULE
import crc;
#define CHECK_MATRIX(...) do { CHECK_HEADER(__VA_ARGS__); CHECK_MODULE(__VA_ARGS__); } while (false)
#else
#define CHECK_MATRIX(...) do { CHECK_HEADER(__VA_ARGS__); } while (false)
#endif

// </ugly_code>

using namespace std::literals;

TEST_CASE("compile-time checks") {
    CHECK_MATRIX(crc::algorithm<crc::algorithms::slice_by_t<0xC0FFEE>>);
    CHECK_MATRIX(!crc::algorithm<int>);
    CHECK_MATRIX(std::regular_invocable<decltype(crc::crc32c::compute), std::vector<char>&>);
    CHECK_MATRIX(std::regular_invocable<decltype(crc::crc32c::compute), std::vector<unsigned char>&>);
    CHECK_MATRIX(std::regular_invocable<decltype(crc::crc32c::compute), std::vector<std::byte>&>);
    CHECK_MATRIX(std::regular_invocable<decltype(crc::crc32c::compute), std::vector<char8_t>&>);
    CHECK_MATRIX(!std::regular_invocable<decltype(crc::crc32c::compute), std::vector<bool>&>);
    CHECK_MATRIX(!std::regular_invocable<decltype(crc::crc32c::compute), std::vector<std::uint16_t>&>);
}

// Convenience alias to save characters.
template <std::size_t N>
using c = std::integral_constant<std::size_t, N>;

// Testing slice-by-10 is particularly important; the test data is 9 bytes long,
// so we verify the algorithm works even when the main loop isn't entered.
TEMPLATE_TEST_CASE("basic functionality checks", "", c<1>, c<2>, c<3>, c<4>, c<5>, c<10>) {
    static constexpr std::size_t N {TestType::value};
    static constexpr std::string_view test_data {"123456789"};

    CHECK_MATRIX(crc::crc3_gsm::compute(crc::algorithms::slice_by<N>, test_data) == 0x4);
    CHECK_MATRIX(crc::crc3_rohc::compute(crc::algorithms::slice_by<N>, test_data) == 0x6);
    CHECK_MATRIX(crc::crc4_g_704::compute(crc::algorithms::slice_by<N>, test_data) == 0x7);
    CHECK_MATRIX(crc::crc4_interlaken::compute(crc::algorithms::slice_by<N>, test_data) == 0xB);
    CHECK_MATRIX(crc::crc5_epc_c1g2::compute(crc::algorithms::slice_by<N>, test_data) == 0x00);
    CHECK_MATRIX(crc::crc5_g_704::compute(crc::algorithms::slice_by<N>, test_data) == 0x07);
    CHECK_MATRIX(crc::crc5_usb::compute(crc::algorithms::slice_by<N>, test_data) == 0x19);
    CHECK_MATRIX(crc::crc6_cdma2000_a::compute(crc::algorithms::slice_by<N>, test_data) == 0x0D);
    CHECK_MATRIX(crc::crc6_cdma2000_b::compute(crc::algorithms::slice_by<N>, test_data) == 0x3B);
    CHECK_MATRIX(crc::crc6_darc::compute(crc::algorithms::slice_by<N>, test_data) == 0x26);
    CHECK_MATRIX(crc::crc6_g_704::compute(crc::algorithms::slice_by<N>, test_data) == 0x06);
    CHECK_MATRIX(crc::crc6_gsm::compute(crc::algorithms::slice_by<N>, test_data) == 0x13);
    CHECK_MATRIX(crc::crc7_mmc::compute(crc::algorithms::slice_by<N>, test_data) == 0x75);
    CHECK_MATRIX(crc::crc7_rohc::compute(crc::algorithms::slice_by<N>, test_data) == 0x53);
    CHECK_MATRIX(crc::crc7_umts::compute(crc::algorithms::slice_by<N>, test_data) == 0x61);
    CHECK_MATRIX(crc::crc8_autosar::compute(crc::algorithms::slice_by<N>, test_data) == 0xDF);
    CHECK_MATRIX(crc::crc8_bluetooth::compute(crc::algorithms::slice_by<N>, test_data) == 0x26);
    CHECK_MATRIX(crc::crc8_cdma2000::compute(crc::algorithms::slice_by<N>, test_data) == 0xDA);
    CHECK_MATRIX(crc::crc8_darc::compute(crc::algorithms::slice_by<N>, test_data) == 0x15);
    CHECK_MATRIX(crc::crc8_dvb_s2::compute(crc::algorithms::slice_by<N>, test_data) == 0xBC);
    CHECK_MATRIX(crc::crc8_gsm_a::compute(crc::algorithms::slice_by<N>, test_data) == 0x37);
    CHECK_MATRIX(crc::crc8_gsm_b::compute(crc::algorithms::slice_by<N>, test_data) == 0x94);
    CHECK_MATRIX(crc::crc8_hitag::compute(crc::algorithms::slice_by<N>, test_data) == 0xB4);
    CHECK_MATRIX(crc::crc8_i_432_1::compute(crc::algorithms::slice_by<N>, test_data) == 0xA1);
    CHECK_MATRIX(crc::crc8_i_code::compute(crc::algorithms::slice_by<N>, test_data) == 0x7E);
    CHECK_MATRIX(crc::crc8_lte::compute(crc::algorithms::slice_by<N>, test_data) == 0xEA);
    CHECK_MATRIX(crc::crc8_maxim_dow::compute(crc::algorithms::slice_by<N>, test_data) == 0xA1);
    CHECK_MATRIX(crc::crc8_mifare_mad::compute(crc::algorithms::slice_by<N>, test_data) == 0x99);
    CHECK_MATRIX(crc::crc8_nrsc_5::compute(crc::algorithms::slice_by<N>, test_data) == 0xF7);
    CHECK_MATRIX(crc::crc8_opensafety::compute(crc::algorithms::slice_by<N>, test_data) == 0x3E);
    CHECK_MATRIX(crc::crc8_rohc::compute(crc::algorithms::slice_by<N>, test_data) == 0xD0);
    CHECK_MATRIX(crc::crc8_sae_j1850::compute(crc::algorithms::slice_by<N>, test_data) == 0x4B);
    CHECK_MATRIX(crc::crc8_smbus::compute(crc::algorithms::slice_by<N>, test_data) == 0xF4);
    CHECK_MATRIX(crc::crc8_tech_3250::compute(crc::algorithms::slice_by<N>, test_data) == 0x97);
    CHECK_MATRIX(crc::crc8_wcdma::compute(crc::algorithms::slice_by<N>, test_data) == 0x25);
    CHECK_MATRIX(crc::crc10_atm::compute(crc::algorithms::slice_by<N>, test_data) == 0x199);
    CHECK_MATRIX(crc::crc10_cdma2000::compute(crc::algorithms::slice_by<N>, test_data) == 0x233);
    CHECK_MATRIX(crc::crc10_gsm::compute(crc::algorithms::slice_by<N>, test_data) == 0x12A);
    CHECK_MATRIX(crc::crc11_flexray::compute(crc::algorithms::slice_by<N>, test_data) == 0x5A3);
    CHECK_MATRIX(crc::crc11_umts::compute(crc::algorithms::slice_by<N>, test_data) == 0x061);
    CHECK_MATRIX(crc::crc12_cdma2000::compute(crc::algorithms::slice_by<N>, test_data) == 0xD4D);
    CHECK_MATRIX(crc::crc12_dect::compute(crc::algorithms::slice_by<N>, test_data) == 0xF5B);
    CHECK_MATRIX(crc::crc12_gsm::compute(crc::algorithms::slice_by<N>, test_data) == 0xB34);
    CHECK_MATRIX(crc::crc12_umts::compute(crc::algorithms::slice_by<N>, test_data) == 0xDAF);
    CHECK_MATRIX(crc::crc13_bbc::compute(crc::algorithms::slice_by<N>, test_data) == 0x04FA);
    CHECK_MATRIX(crc::crc14_darc::compute(crc::algorithms::slice_by<N>, test_data) == 0x082D);
    CHECK_MATRIX(crc::crc14_gsm::compute(crc::algorithms::slice_by<N>, test_data) == 0x30AE);
    CHECK_MATRIX(crc::crc15_can::compute(crc::algorithms::slice_by<N>, test_data) == 0x059E);
    CHECK_MATRIX(crc::crc15_mpt1327::compute(crc::algorithms::slice_by<N>, test_data) == 0x2566);
    CHECK_MATRIX(crc::crc16_arc::compute(crc::algorithms::slice_by<N>, test_data) == 0xBB3D);
    CHECK_MATRIX(crc::crc16_cdma2000::compute(crc::algorithms::slice_by<N>, test_data) == 0x4C06);
    CHECK_MATRIX(crc::crc16_cms::compute(crc::algorithms::slice_by<N>, test_data) == 0xAEE7);
    CHECK_MATRIX(crc::crc16_dds_110::compute(crc::algorithms::slice_by<N>, test_data) == 0x9ECF);
    CHECK_MATRIX(crc::crc16_dect_r::compute(crc::algorithms::slice_by<N>, test_data) == 0x007E);
    CHECK_MATRIX(crc::crc16_dect_x::compute(crc::algorithms::slice_by<N>, test_data) == 0x007F);
    CHECK_MATRIX(crc::crc16_dnp::compute(crc::algorithms::slice_by<N>, test_data) == 0xEA82);
    CHECK_MATRIX(crc::crc16_en_13757::compute(crc::algorithms::slice_by<N>, test_data) == 0xC2B7);
    CHECK_MATRIX(crc::crc16_genibus::compute(crc::algorithms::slice_by<N>, test_data) == 0xD64E);
    CHECK_MATRIX(crc::crc16_gsm::compute(crc::algorithms::slice_by<N>, test_data) == 0xCE3C);
    CHECK_MATRIX(crc::crc16_ibm_3740::compute(crc::algorithms::slice_by<N>, test_data) == 0x29B1);
    CHECK_MATRIX(crc::crc16_ibm_sdlc::compute(crc::algorithms::slice_by<N>, test_data) == 0x906E);
    CHECK_MATRIX(crc::crc16_iso_iec_14443_3_a::compute(crc::algorithms::slice_by<N>, test_data) == 0xBF05);
    CHECK_MATRIX(crc::crc16_kermit::compute(crc::algorithms::slice_by<N>, test_data) == 0x2189);
    CHECK_MATRIX(crc::crc16_lj1200::compute(crc::algorithms::slice_by<N>, test_data) == 0xBDF4);
    CHECK_MATRIX(crc::crc16_m17::compute(crc::algorithms::slice_by<N>, test_data) == 0x772B);
    CHECK_MATRIX(crc::crc16_maxim_dow::compute(crc::algorithms::slice_by<N>, test_data) == 0x44C2);
    CHECK_MATRIX(crc::crc16_mcrf4xx::compute(crc::algorithms::slice_by<N>, test_data) == 0x6F91);
    CHECK_MATRIX(crc::crc16_modbus::compute(crc::algorithms::slice_by<N>, test_data) == 0x4B37);
    CHECK_MATRIX(crc::crc16_nrsc_5::compute(crc::algorithms::slice_by<N>, test_data) == 0xA066);
    CHECK_MATRIX(crc::crc16_opensafety_a::compute(crc::algorithms::slice_by<N>, test_data) == 0x5D38);
    CHECK_MATRIX(crc::crc16_opensafety_b::compute(crc::algorithms::slice_by<N>, test_data) == 0x20FE);
    CHECK_MATRIX(crc::crc16_profibus::compute(crc::algorithms::slice_by<N>, test_data) == 0xA819);
    CHECK_MATRIX(crc::crc16_riello::compute(crc::algorithms::slice_by<N>, test_data) == 0x63D0);
    CHECK_MATRIX(crc::crc16_spi_fujitsu::compute(crc::algorithms::slice_by<N>, test_data) == 0xE5CC);
    CHECK_MATRIX(crc::crc16_t10_dif::compute(crc::algorithms::slice_by<N>, test_data) == 0xD0DB);
    CHECK_MATRIX(crc::crc16_teledisk::compute(crc::algorithms::slice_by<N>, test_data) == 0x0FB3);
    CHECK_MATRIX(crc::crc16_tms37157::compute(crc::algorithms::slice_by<N>, test_data) == 0x26B1);
    CHECK_MATRIX(crc::crc16_umts::compute(crc::algorithms::slice_by<N>, test_data) == 0xFEE8);
    CHECK_MATRIX(crc::crc16_usb::compute(crc::algorithms::slice_by<N>, test_data) == 0xB4C8);
    CHECK_MATRIX(crc::crc16_xmodem::compute(crc::algorithms::slice_by<N>, test_data) == 0x31C3);
    CHECK_MATRIX(crc::crc17_can_fd::compute(crc::algorithms::slice_by<N>, test_data) == 0x04F03);
    CHECK_MATRIX(crc::crc21_can_fd::compute(crc::algorithms::slice_by<N>, test_data) == 0x0ED841);
    CHECK_MATRIX(crc::crc24_ble::compute(crc::algorithms::slice_by<N>, test_data) == 0xC25A56);
    CHECK_MATRIX(crc::crc24_flexray_a::compute(crc::algorithms::slice_by<N>, test_data) == 0x7979BD);
    CHECK_MATRIX(crc::crc24_flexray_b::compute(crc::algorithms::slice_by<N>, test_data) == 0x1F23B8);
    CHECK_MATRIX(crc::crc24_interlaken::compute(crc::algorithms::slice_by<N>, test_data) == 0xB4F3E6);
    CHECK_MATRIX(crc::crc24_lte_a::compute(crc::algorithms::slice_by<N>, test_data) == 0xCDE703);
    CHECK_MATRIX(crc::crc24_lte_b::compute(crc::algorithms::slice_by<N>, test_data) == 0x23EF52);
    CHECK_MATRIX(crc::crc24_openpgp::compute(crc::algorithms::slice_by<N>, test_data) == 0x21CF02);
    CHECK_MATRIX(crc::crc24_os_9::compute(crc::algorithms::slice_by<N>, test_data) == 0x200FA5);
    CHECK_MATRIX(crc::crc30_cdma::compute(crc::algorithms::slice_by<N>, test_data) == 0x04C34ABF);
    CHECK_MATRIX(crc::crc31_philips::compute(crc::algorithms::slice_by<N>, test_data) == 0x0CE9E46C);
    CHECK_MATRIX(crc::crc32_aixm::compute(crc::algorithms::slice_by<N>, test_data) == 0x3010BF7F);
    CHECK_MATRIX(crc::crc32_autosar::compute(crc::algorithms::slice_by<N>, test_data) == 0x1697D06A);
    CHECK_MATRIX(crc::crc32_base91_d::compute(crc::algorithms::slice_by<N>, test_data) == 0x87315576);
    CHECK_MATRIX(crc::crc32::compute(crc::algorithms::slice_by<N>, test_data) == 0xFC891918);
    CHECK_MATRIX(crc::crc32_cd_rom_edc::compute(crc::algorithms::slice_by<N>, test_data) == 0x6EC2EDC4);
    CHECK_MATRIX(crc::crc32_cksum::compute(crc::algorithms::slice_by<N>, test_data) == 0x765E7680);
    CHECK_MATRIX(crc::crc32c::compute(crc::algorithms::slice_by<N>, test_data) == 0xE3069283);
    CHECK_MATRIX(crc::crc32_iso_hdlc::compute(crc::algorithms::slice_by<N>, test_data) == 0xCBF43926);
    CHECK_MATRIX(crc::crc32_jamcrc::compute(crc::algorithms::slice_by<N>, test_data) == 0x340BC6D9);
    CHECK_MATRIX(crc::crc32_mef::compute(crc::algorithms::slice_by<N>, test_data) == 0xD2C22F51);
    CHECK_MATRIX(crc::crc32_mpeg2::compute(crc::algorithms::slice_by<N>, test_data) == 0x0376E6E7);
    CHECK_MATRIX(crc::crc32_xfer::compute(crc::algorithms::slice_by<N>, test_data) == 0xBD0BE338);
    CHECK_MATRIX(crc::crc40_gsm::compute(crc::algorithms::slice_by<N>, test_data) == 0xD4164FC646);
    CHECK_MATRIX(crc::crc64_ecma_182::compute(crc::algorithms::slice_by<N>, test_data) == 0x6C40DF5F0B497347);
    CHECK_MATRIX(crc::crc64_go_iso::compute(crc::algorithms::slice_by<N>, test_data) == 0xB90956C775A41001);
    CHECK_MATRIX(crc::crc64_ms::compute(crc::algorithms::slice_by<N>, test_data) == 0x75D4B74F024ECEEA);
    CHECK_MATRIX(crc::crc64_nvme::compute(crc::algorithms::slice_by<N>, test_data) == 0xAE8B14860A799888);
    CHECK_MATRIX(crc::crc64_redis::compute(crc::algorithms::slice_by<N>, test_data) == 0xE9C6D914C4B8D9CA);
    CHECK_MATRIX(crc::crc64_we::compute(crc::algorithms::slice_by<N>, test_data) == 0x62EC59E3F1A4F00A);
    CHECK_MATRIX(crc::crc64_xz::compute(crc::algorithms::slice_by<N>, test_data) == 0x995DC9BBDF1939FA);
    // CHECK_MATRIX(crc::crc82_darc::compute(crc::algorithms::slice_by<N>, test_data) == std::bitset<82>{"000010011110101010000011111101100010010100000010001110000000000111111101011000010010");

    static constexpr std::array<std::string_view, 4> random_messages {
        "3682BBD37BE6475E08320602B656AF65",
        "9D928182DE7241013877A3850C9BF532",
        "82D17BCB653429E0AEDEC081B9F66BE3",
        "7C38927DDB83DBD3BB4504E1F31A8009",
    };

    CHECK_MATRIX(std::ranges::equal(
        random_messages | std::views::transform(crc::crc32c::compute),
        std::views::iota(0U, random_messages.size()) | std::views::transform([&] (const auto i) {
            return crc::crc32c::compute(random_messages[i]);
        })
    ));

#ifdef __cpp_lib_ranges_fold
    static constexpr std::array<std::array<char, 3>, 3> noncontiguous_test_data {{
        {'7', '8', '9'}, {'4', '5', '6'}, {'1', '2', '3'}
    }};
    CHECK_MATRIX(
        crc::crc32c::compute(crc::algorithms::slice_by<N>, test_data) ==
        crc::finalize(std::ranges::fold_left(noncontiguous_test_data | std::views::reverse, crc::crc32c {}, crc::process))
    );
#endif
}

TEST_CASE("equality comparison") {
    CHECK_MATRIX(crc::crc10_atm {} == crc::crc10_atm {});
    CHECK_MATRIX(!(crc::crc10_atm {} != crc::crc10_atm {}));

    CHECK_MATRIX(crc::crc10_atm {} == crc::process(crc::crc10_atm {}, "\0\0\0\0\0"sv));

    // The LHS message is just zeroes, and the RHS message is the CRC10-ATM generator
    // polynomial, 0x633. They produce the same checksums, but different garbage bits at the
    // top of the shift register, so this test ensures that operator== ignores those bits.
    CHECK_MATRIX(
        crc::process(crc::algorithms::slice_by<1>, crc::crc10_atm {}, "\x00\x00"sv) ==
        crc::process(crc::algorithms::slice_by<1>, crc::crc10_atm {}, "\x06\x33"sv)
    );

    // This is just the example from the README.
    CHECK_MATRIX([] {
        crc::crc64_xz crc {};

        crc = crc::process(crc, "Some data"sv);
        crc = crc::process(crc, u8" processed in "sv);
        crc = crc::process(crc, std::vector<std::uint8_t> {'p', 'a', 'r', 't', 's'});

        const std::uint64_t result {crc::finalize(crc)};
        return result;
    }() == crc::crc64_xz::compute("Some data processed in parts"sv));
}

TEST_CASE("is_valid") {
    CHECK_MATRIX(crc::crc32c::is_valid("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x4E\x79\xDD\x46"sv));
    CHECK_MATRIX(crc::crc16_arc::is_valid("\x33\x22\x55\xAA\xBB\xCC\xDD\xEE\xFF\x98\xAE"sv));
}
