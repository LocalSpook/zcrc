// SPDX-License-Identifier: MIT

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <ranges>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

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

#define CHECK_COMPILE_AND_RUN_TIME(...) [&] { \
        static_assert(__VA_ARGS__);       \
        CHECK(__VA_ARGS__); \
    }()

#define CHECK_HEADER(...) SECTION("header") {   \
        namespace crc = header::crc;            \
        CHECK_COMPILE_AND_RUN_TIME(__VA_ARGS__); \
    }

#define CHECK_MODULE(...) SECTION("module") {   \
        CHECK_COMPILE_AND_RUN_TIME(__VA_ARGS__); \
    }

#ifdef CRC_TEST_MODULE
import crc;
#define CHECK_MATRIX(...) [&] { CHECK_HEADER(__VA_ARGS__); CHECK_MODULE(__VA_ARGS__); }()
#else
#define CHECK_MATRIX(...) [&] { CHECK_HEADER(__VA_ARGS__); }()
#endif

using namespace std::literals;

TEST_CASE("compile-time checks") {
    CHECK_MATRIX(crc::algorithm<crc::slice_by_t<0xC0FFEE>>);
    CHECK_MATRIX(crc::algorithm<crc::parallel_t<crc::slice_by_t<0xC0FFEE>>>);
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

TEMPLATE_TEST_CASE("basic functionality checks", "", c<1>, c<2>, c<3>, c<4>, c<5>) {
    static constexpr std::size_t N {TestType::value};
    static constexpr std::string_view test_data {"123456789"};

    CHECK_MATRIX(crc::crc3_gsm::compute(crc::slice_by<N>, test_data) == 0x4);
    CHECK_MATRIX(crc::crc3_rohc::compute(crc::slice_by<N>, test_data) == 0x6);
    CHECK_MATRIX(crc::crc4_g_704::compute(crc::slice_by<N>, test_data) == 0x7);
    CHECK_MATRIX(crc::crc4_interlaken::compute(crc::slice_by<N>, test_data) == 0xB);
    CHECK_MATRIX(crc::crc5_epc_c1g2::compute(crc::slice_by<N>, test_data) == 0x00);
    CHECK_MATRIX(crc::crc5_g_704::compute(crc::slice_by<N>, test_data) == 0x07);
    CHECK_MATRIX(crc::crc5_usb::compute(crc::slice_by<N>, test_data) == 0x19);
    CHECK_MATRIX(crc::crc6_cdma2000_a::compute(crc::slice_by<N>, test_data) == 0x0D);
    CHECK_MATRIX(crc::crc6_cdma2000_b::compute(crc::slice_by<N>, test_data) == 0x3B);
    CHECK_MATRIX(crc::crc6_darc::compute(crc::slice_by<N>, test_data) == 0x26);
    CHECK_MATRIX(crc::crc6_g_704::compute(crc::slice_by<N>, test_data) == 0x06);
    CHECK_MATRIX(crc::crc6_gsm::compute(crc::slice_by<N>, test_data) == 0x13);
    CHECK_MATRIX(crc::crc7_mmc::compute(crc::slice_by<N>, test_data) == 0x75);
    CHECK_MATRIX(crc::crc7_rohc::compute(crc::slice_by<N>, test_data) == 0x53);
    CHECK_MATRIX(crc::crc7_umts::compute(crc::slice_by<N>, test_data) == 0x61);
    CHECK_MATRIX(crc::crc8_autosar::compute(crc::slice_by<N>, test_data) == 0xDF);
    CHECK_MATRIX(crc::crc8_bluetooth::compute(crc::slice_by<N>, test_data) == 0x26);
    CHECK_MATRIX(crc::crc8_cdma2000::compute(crc::slice_by<N>, test_data) == 0xDA);
    CHECK_MATRIX(crc::crc8_darc::compute(crc::slice_by<N>, test_data) == 0x15);
    CHECK_MATRIX(crc::crc8_dvb_s2::compute(crc::slice_by<N>, test_data) == 0xBC);
    CHECK_MATRIX(crc::crc8_gsm_a::compute(crc::slice_by<N>, test_data) == 0x37);
    CHECK_MATRIX(crc::crc8_gsm_b::compute(crc::slice_by<N>, test_data) == 0x94);
    CHECK_MATRIX(crc::crc8_hitag::compute(crc::slice_by<N>, test_data) == 0xB4);
    CHECK_MATRIX(crc::crc8_i_432_1::compute(crc::slice_by<N>, test_data) == 0xA1);
    CHECK_MATRIX(crc::crc8_i_code::compute(crc::slice_by<N>, test_data) == 0x7E);
    CHECK_MATRIX(crc::crc8_lte::compute(crc::slice_by<N>, test_data) == 0xEA);
    CHECK_MATRIX(crc::crc8_maxim_dow::compute(crc::slice_by<N>, test_data) == 0xA1);
    CHECK_MATRIX(crc::crc8_mifare_mad::compute(crc::slice_by<N>, test_data) == 0x99);
    CHECK_MATRIX(crc::crc8_nrsc_5::compute(crc::slice_by<N>, test_data) == 0xF7);
    CHECK_MATRIX(crc::crc8_opensafety::compute(crc::slice_by<N>, test_data) == 0x3E);
    CHECK_MATRIX(crc::crc8_rohc::compute(crc::slice_by<N>, test_data) == 0xD0);
    CHECK_MATRIX(crc::crc8_sae_j1850::compute(crc::slice_by<N>, test_data) == 0x4B);
    CHECK_MATRIX(crc::crc8_smbus::compute(crc::slice_by<N>, test_data) == 0xF4);
    CHECK_MATRIX(crc::crc8_tech_3250::compute(crc::slice_by<N>, test_data) == 0x97);
    CHECK_MATRIX(crc::crc8_wcdma::compute(crc::slice_by<N>, test_data) == 0x25);
    CHECK_MATRIX(crc::crc10_atm::compute(crc::slice_by<N>, test_data) == 0x199);
    CHECK_MATRIX(crc::crc10_cdma2000::compute(crc::slice_by<N>, test_data) == 0x233);
    CHECK_MATRIX(crc::crc10_gsm::compute(crc::slice_by<N>, test_data) == 0x12A);
    CHECK_MATRIX(crc::crc11_flexray::compute(crc::slice_by<N>, test_data) == 0x5A3);
    CHECK_MATRIX(crc::crc11_umts::compute(crc::slice_by<N>, test_data) == 0x061);
    CHECK_MATRIX(crc::crc12_cdma2000::compute(crc::slice_by<N>, test_data) == 0xD4D);
    CHECK_MATRIX(crc::crc12_dect::compute(crc::slice_by<N>, test_data) == 0xF5B);
    CHECK_MATRIX(crc::crc12_gsm::compute(crc::slice_by<N>, test_data) == 0xB34);
    CHECK_MATRIX(crc::crc12_umts::compute(crc::slice_by<N>, test_data) == 0xDAF);
    CHECK_MATRIX(crc::crc13_bbc::compute(crc::slice_by<N>, test_data) == 0x04FA);
    CHECK_MATRIX(crc::crc14_darc::compute(crc::slice_by<N>, test_data) == 0x082D);
    CHECK_MATRIX(crc::crc14_gsm::compute(crc::slice_by<N>, test_data) == 0x30AE);
    CHECK_MATRIX(crc::crc15_can::compute(crc::slice_by<N>, test_data) == 0x059E);
    CHECK_MATRIX(crc::crc15_mpt1327::compute(crc::slice_by<N>, test_data) == 0x2566);
    CHECK_MATRIX(crc::crc16_arc::compute(crc::slice_by<N>, test_data) == 0xBB3D);
    CHECK_MATRIX(crc::crc16_cdma2000::compute(crc::slice_by<N>, test_data) == 0x4C06);
    CHECK_MATRIX(crc::crc16_cms::compute(crc::slice_by<N>, test_data) == 0xAEE7);
    CHECK_MATRIX(crc::crc16_dds_110::compute(crc::slice_by<N>, test_data) == 0x9ECF);
    CHECK_MATRIX(crc::crc16_dect_r::compute(crc::slice_by<N>, test_data) == 0x007E);
    CHECK_MATRIX(crc::crc16_dect_x::compute(crc::slice_by<N>, test_data) == 0x007F);
    CHECK_MATRIX(crc::crc16_dnp::compute(crc::slice_by<N>, test_data) == 0xEA82);
    CHECK_MATRIX(crc::crc16_en_13757::compute(crc::slice_by<N>, test_data) == 0xC2B7);
    CHECK_MATRIX(crc::crc16_genibus::compute(crc::slice_by<N>, test_data) == 0xD64E);
    CHECK_MATRIX(crc::crc16_gsm::compute(crc::slice_by<N>, test_data) == 0xCE3C);
    CHECK_MATRIX(crc::crc16_ibm_3740::compute(crc::slice_by<N>, test_data) == 0x29B1);
    CHECK_MATRIX(crc::crc16_ibm_sdlc::compute(crc::slice_by<N>, test_data) == 0x906E);
    CHECK_MATRIX(crc::crc16_iso_iec_14443_3_a::compute(crc::slice_by<N>, test_data) == 0xBF05);
    CHECK_MATRIX(crc::crc16_kermit::compute(crc::slice_by<N>, test_data) == 0x2189);
    CHECK_MATRIX(crc::crc16_lj1200::compute(crc::slice_by<N>, test_data) == 0xBDF4);
    CHECK_MATRIX(crc::crc16_m17::compute(crc::slice_by<N>, test_data) == 0x772B);
    CHECK_MATRIX(crc::crc16_maxim_dow::compute(crc::slice_by<N>, test_data) == 0x44C2);
    CHECK_MATRIX(crc::crc16_mcrf4xx::compute(crc::slice_by<N>, test_data) == 0x6F91);
    CHECK_MATRIX(crc::crc16_modbus::compute(crc::slice_by<N>, test_data) == 0x4B37);
    CHECK_MATRIX(crc::crc16_nrsc_5::compute(crc::slice_by<N>, test_data) == 0xA066);
    CHECK_MATRIX(crc::crc16_opensafety_a::compute(crc::slice_by<N>, test_data) == 0x5D38);
    CHECK_MATRIX(crc::crc16_opensafety_b::compute(crc::slice_by<N>, test_data) == 0x20FE);
    CHECK_MATRIX(crc::crc16_profibus::compute(crc::slice_by<N>, test_data) == 0xA819);
    CHECK_MATRIX(crc::crc16_riello::compute(crc::slice_by<N>, test_data) == 0x63D0);
    CHECK_MATRIX(crc::crc16_spi_fujitsu::compute(crc::slice_by<N>, test_data) == 0xE5CC);
    CHECK_MATRIX(crc::crc16_t10_dif::compute(crc::slice_by<N>, test_data) == 0xD0DB);
    CHECK_MATRIX(crc::crc16_teledisk::compute(crc::slice_by<N>, test_data) == 0x0FB3);
    CHECK_MATRIX(crc::crc16_tms37157::compute(crc::slice_by<N>, test_data) == 0x26B1);
    CHECK_MATRIX(crc::crc16_umts::compute(crc::slice_by<N>, test_data) == 0xFEE8);
    CHECK_MATRIX(crc::crc16_usb::compute(crc::slice_by<N>, test_data) == 0xB4C8);
    CHECK_MATRIX(crc::crc16_xmodem::compute(crc::slice_by<N>, test_data) == 0x31C3);
    CHECK_MATRIX(crc::crc17_can_fd::compute(crc::slice_by<N>, test_data) == 0x04F03);
    CHECK_MATRIX(crc::crc21_can_fd::compute(crc::slice_by<N>, test_data) == 0x0ED841);
    CHECK_MATRIX(crc::crc24_ble::compute(crc::slice_by<N>, test_data) == 0xC25A56);
    CHECK_MATRIX(crc::crc24_flexray_a::compute(crc::slice_by<N>, test_data) == 0x7979BD);
    CHECK_MATRIX(crc::crc24_flexray_b::compute(crc::slice_by<N>, test_data) == 0x1F23B8);
    CHECK_MATRIX(crc::crc24_interlaken::compute(crc::slice_by<N>, test_data) == 0xB4F3E6);
    CHECK_MATRIX(crc::crc24_lte_a::compute(crc::slice_by<N>, test_data) == 0xCDE703);
    CHECK_MATRIX(crc::crc24_lte_b::compute(crc::slice_by<N>, test_data) == 0x23EF52);
    CHECK_MATRIX(crc::crc24_openpgp::compute(crc::slice_by<N>, test_data) == 0x21CF02);
    CHECK_MATRIX(crc::crc24_os_9::compute(crc::slice_by<N>, test_data) == 0x200FA5);
    CHECK_MATRIX(crc::crc30_cdma::compute(crc::slice_by<N>, test_data) == 0x04C34ABF);
    CHECK_MATRIX(crc::crc31_philips::compute(crc::slice_by<N>, test_data) == 0x0CE9E46C);
    CHECK_MATRIX(crc::crc32_aixm::compute(crc::slice_by<N>, test_data) == 0x3010BF7F);
    CHECK_MATRIX(crc::crc32_autosar::compute(crc::slice_by<N>, test_data) == 0x1697D06A);
    CHECK_MATRIX(crc::crc32_base91_d::compute(crc::slice_by<N>, test_data) == 0x87315576);
    CHECK_MATRIX(crc::crc32::compute(crc::slice_by<N>, test_data) == 0xFC891918);
    CHECK_MATRIX(crc::crc32_cd_rom_edc::compute(crc::slice_by<N>, test_data) == 0x6EC2EDC4);
    CHECK_MATRIX(crc::crc32_cksum::compute(crc::slice_by<N>, test_data) == 0x765E7680);
    CHECK_MATRIX(crc::crc32c::compute(crc::slice_by<N>, test_data) == 0xE3069283);
    CHECK_MATRIX(crc::crc32_iso_hdlc::compute(crc::slice_by<N>, test_data) == 0xCBF43926);
    CHECK_MATRIX(crc::crc32_jamcrc::compute(crc::slice_by<N>, test_data) == 0x340BC6D9);
    CHECK_MATRIX(crc::crc32_mef::compute(crc::slice_by<N>, test_data) == 0xD2C22F51);
    CHECK_MATRIX(crc::crc32_mpeg2::compute(crc::slice_by<N>, test_data) == 0x0376E6E7);
    CHECK_MATRIX(crc::crc32_xfer::compute(crc::slice_by<N>, test_data) == 0xBD0BE338);
    CHECK_MATRIX(crc::crc40_gsm::compute(crc::slice_by<N>, test_data) == 0xD4164FC646);
    CHECK_MATRIX(crc::crc64_ecma_182::compute(crc::slice_by<N>, test_data) == 0x6C40DF5F0B497347);
    CHECK_MATRIX(crc::crc64_go_iso::compute(crc::slice_by<N>, test_data) == 0xB90956C775A41001);
    CHECK_MATRIX(crc::crc64_ms::compute(crc::slice_by<N>, test_data) == 0x75D4B74F024ECEEA);
    CHECK_MATRIX(crc::crc64_nvme::compute(crc::slice_by<N>, test_data) == 0xAE8B14860A799888);
    CHECK_MATRIX(crc::crc64_redis::compute(crc::slice_by<N>, test_data) == 0xE9C6D914C4B8D9CA);
    CHECK_MATRIX(crc::crc64_we::compute(crc::slice_by<N>, test_data) == 0x62EC59E3F1A4F00A);
    CHECK_MATRIX(crc::crc64_xz::compute(crc::slice_by<N>, test_data) == 0x995DC9BBDF1939FA);
    // CHECK_MATRIX(crc::crc82_darc::compute(crc::slice_by<N>, test_data) == std::bitset<82>{"000010011110101010000011111101100010010100000010001110000000000111111101011000010010");

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
        crc::crc32c::compute(crc::slice_by<N>, test_data) ==
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
        crc::process(crc::slice_by<1>, crc::crc10_atm {}, "\x00\x00"sv) ==
        crc::process(crc::slice_by<1>, crc::crc10_atm {}, "\x06\x33"sv)
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

TEMPLATE_TEST_CASE("process_zero_bytes and parallel", "",
    header::crc::crc3_gsm, header::crc::crc3_rohc, header::crc::crc4_g_704, header::crc::crc4_interlaken,
    header::crc::crc5_epc_c1g2, header::crc::crc5_g_704, header::crc::crc5_usb, header::crc::crc6_cdma2000_a,
    header::crc::crc6_cdma2000_b, header::crc::crc6_darc, header::crc::crc6_g_704, header::crc::crc6_gsm,
    header::crc::crc7_mmc, header::crc::crc7_rohc, header::crc::crc7_umts, header::crc::crc8_autosar,
    header::crc::crc8_bluetooth, header::crc::crc8_cdma2000, header::crc::crc8_darc, header::crc::crc8_dvb_s2,
    header::crc::crc8_gsm_a, header::crc::crc8_gsm_b, header::crc::crc8_hitag, header::crc::crc8_i_432_1,
    header::crc::crc8_i_code, header::crc::crc8_lte, header::crc::crc8_maxim_dow, header::crc::crc8_mifare_mad,
    header::crc::crc8_nrsc_5, header::crc::crc8_opensafety, header::crc::crc8_rohc, header::crc::crc8_sae_j1850,
    header::crc::crc8_smbus, header::crc::crc8_tech_3250, header::crc::crc8_wcdma, header::crc::crc10_atm,
    header::crc::crc10_cdma2000, header::crc::crc10_gsm, header::crc::crc11_flexray, header::crc::crc11_umts,
    header::crc::crc12_cdma2000, header::crc::crc12_dect, header::crc::crc12_gsm, header::crc::crc12_umts,
    header::crc::crc13_bbc, header::crc::crc14_darc, header::crc::crc14_gsm, header::crc::crc15_can,
    header::crc::crc15_mpt1327, header::crc::crc16_arc, header::crc::crc16_cdma2000, header::crc::crc16_cms,
    header::crc::crc16_dds_110, header::crc::crc16_dect_r, header::crc::crc16_dect_x, header::crc::crc16_dnp,
    header::crc::crc16_en_13757, header::crc::crc16_genibus, header::crc::crc16_gsm, header::crc::crc16_ibm_3740,
    header::crc::crc16_ibm_sdlc, header::crc::crc16_iso_iec_14443_3_a, header::crc::crc16_kermit, header::crc::crc16_lj1200,
    header::crc::crc16_m17, header::crc::crc16_maxim_dow, header::crc::crc16_mcrf4xx, header::crc::crc16_modbus,
    header::crc::crc16_nrsc_5, header::crc::crc16_opensafety_a, header::crc::crc16_opensafety_b, header::crc::crc16_profibus,
    header::crc::crc16_riello, header::crc::crc16_spi_fujitsu, header::crc::crc16_t10_dif, header::crc::crc16_teledisk,
    header::crc::crc16_tms37157, header::crc::crc16_umts, header::crc::crc16_usb, header::crc::crc16_xmodem,
    header::crc::crc17_can_fd, header::crc::crc21_can_fd, header::crc::crc24_ble, header::crc::crc24_flexray_a,
    header::crc::crc24_flexray_b, header::crc::crc24_interlaken, header::crc::crc24_lte_a, header::crc::crc24_lte_b,
    header::crc::crc24_openpgp, header::crc::crc24_os_9, header::crc::crc30_cdma, header::crc::crc31_philips,
    header::crc::crc32_aixm, header::crc::crc32_autosar, header::crc::crc32_base91_d, header::crc::crc32,
    header::crc::crc32_cd_rom_edc, header::crc::crc32_cksum, header::crc::crc32c, header::crc::crc32_iso_hdlc,
    header::crc::crc32_jamcrc, header::crc::crc32_mef, header::crc::crc32_mpeg2, header::crc::crc32_xfer
    // , header::crc::crc40_gsm,
    // , header::crc::crc64_ecma_182,
    // , header::crc::crc64_go_iso,
    // , header::crc::crc64_ms,
    // , header::crc::crc64_nvme,
    // , header::crc::crc64_redis,
    // , header::crc::crc64_we,
    // , header::crc::crc64_xz
    // , header::crc::crc82_darc
) {
    // Ensure process_zero_bytes runs in logarithmic time.
    CHECK_COMPILE_AND_RUN_TIME(((void)header::crc::process_zero_bytes(TestType {}, std::numeric_limits<std::size_t>::max()), true));

    []<std::size_t... N>(std::index_sequence<N...>){
        (CHECK_COMPILE_AND_RUN_TIME(
            header::crc::process(header::crc::slice_by<1>, TestType {}, std::array<std::uint8_t, N> {}) ==
            header::crc::process_zero_bytes(TestType {}, N)
        ), ...);
    }(std::make_index_sequence<8>{});

    static constexpr std::string_view long_message {
        "E6899E53E69E7C413A1CD5A21CC4324652DB349834B17A7B1AD1575E2F4FA3A03DC3FD09D9E1439708F60DBF861098AE7EC6D41D614FD3BCCE032221C8433334"
        "D222E03BC576084700C37571D8CB13C4A459799663EBCE7AAAB32338727A111E97B5F049BADDE8667CEFB1C9A56076E4243E238E4B596F32A92A0E79AF1DD544"
        "71A9E2D1D1A706C1EEDAF4C7E7D68A4EDFC753C95DCA622B92BC9DF09F02B9F1A262A17B8701855EF11DED2C4D3565E434D051F92A5600A76A0D73916FA15E17"
        "6123CF68CCA08BAADE99D83D228339DE7ED58F6080C88D3448E8B97920C12FA7F22A44B273ADC94B5D4097B673BE235F1436251CF9BE2B64EF053D538E62A59D"
        "8CF078DB2D93AC2532E0FB69F984BCC41FD9E2DC1F0A8E8673FED6876994585075901DA12FCAC6549B09B8647535889DB03BD48757E8B2927FF833A2DE3DCEED"
        "314D8D6D8655709AA9F99BEA2317D935016247D19E6DB423D19D35AE94D8D4D5FE890D9618417151EF8567EEF05CB1314B2E32F0165F165464576370B2D7529D"
        "A084EB4A796E9AF1FEB7FD32A74599681F66D4F22BF09F8157A169FAA188ADF197445B75CCEA02D270AB662B2968058404ECA3A78A14963D77FA479FB6CB5364"
        "1F7761DA5C63ECE2CD52CC960D045D1114C151F417FA46613E3425704C09A3CDA1020F3AD6036CF14D9A0A11BEDE09C7EAF5F6CE13FF04A34FA187EA7A67A14E"
        "3F4746A80C4FC62AF5CBB964F3E25A778F6E11578B8A8033948944F5D459AF55A36E0270A2210557BC9DE52B60D23BF3E3082E791EA5A9D1BD52BCC78CEDE62C"
        "2D8BE2B12D584E30A948585B054650308462C558249535166146EE2397B853B10D70A7FD23AF3C8A7AE5DA8B27E7C7604F6DBD9CA14C4D869CEF9AC5E35EF337"
        "17777B8D28502A783FE829D0D46D326ADA17421D9DC2AFCB7BEDF3BAE6266AC9512A166A388AC24029E8416D0C1609965FEE44C622D47CF8CDBD3A2B60118B28"
        "A68A494FCBD2C4114C959BBDE567B8A7DF3AE84047E4646D805DA2E60763ABDC8BDA70B7877A0C3EB5C3D95EEACF32BE8E14FAD70C2E3F3CD1E497ABB1FDF169"
        "AED404530C0EAD359F420FE479AE5D1B4031714AE2797E42BEE0B3AD54255C785A480ADED4A05A97E758D870E9307ECF090E2E2A78D62AE057809A8BCAF7C912"
        "923FB03F83C1BC9DF736B3DB863399DB6C975DFDBB0897F0ABAFF1ADCDDBAFDF5B796A64527B2171A2FD044CB7581BECB99D2DADF0BC656FE80698FCD283D41E"
        "56FC35A556BE5DCDFC227C4066CB3EEF8993ACEB255AD3AE9D3C8E5765EA717E7F158FC9FB380675FEDF60F05695600F83BDD31D11B47D22D92B1D11751C58FC"
        "647655FB6163006C9439FA3E7C550A92D7DBC0942F12094A1D4A34787B189741DE8712379084B52FF6261F4A386CC547D57941382938BFC5BF7B9031DC140ECC"
        "1B28F0B0DAC3678DAC60C149C8B68AA44F42F2635FC42279594453A965CB36B6FB1B1DF9BEBC4616629FC644150A9B30DB255ED3FE5DD5A13B0F869FECBDD49A"
        "C2D23A06EEBB921E709BDCAE63EE3472F6722EB8730837296EC839E4EE8B0E8047E26472BE2C21E1636F20153A97A489B7A909D4480003E6A6CE3A997798C935"
        "EC61A3F304247A7CF606B0DC04E59620BA4987D99F711BAC9329FAAF78171C3D28B1FB46E1A6CFADAE320AB45BEE4B0FA2A141F6D7F3A369C73F411551A41EA6"
        "0DD3E786FE184EDACFEE216435867EF40C73944E34BD776D7FFF12390F78B6993278195D623C1EE8DF2092DDBD57C3E205C585D4E47715D6AF711307F71EF637"
    };

    CHECK_COMPILE_AND_RUN_TIME(
        header::crc::process(header::crc::parallel<header::crc::slice_by<1>>, TestType {}, long_message) ==
        header::crc::process(header::crc::slice_by<1>, TestType {}, long_message)
    );
}
