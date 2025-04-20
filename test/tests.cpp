// SPDX-License-Identifier: MIT

#include <string_view>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_case_sensitive.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <crc/crc.hpp>

#define CHECK_AT_COMPILE_AND_RUN_TIME(...) static_assert(__VA_ARGS__); CHECK(__VA_ARGS__)

TEST_CASE("simple test checks", "[single-file]") {
    static constexpr std::string_view test_data {"123456789"};

    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc3_gsm(test_data) == 0x4);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc3_rohc(test_data) == 0x6);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc4_g_704(test_data) == 0x7);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc4_interlaken(test_data) == 0xB);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc5_epc_c1g2(test_data) == 0x00);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc5_g_704(test_data) == 0x07);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc5_usb(test_data) == 0x19);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc6_cdma2000_a(test_data) == 0x0D);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc6_cdma2000_b(test_data) == 0x3B);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc6_darc(test_data) == 0x26);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc6_g_704(test_data) == 0x06);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc6_gsm(test_data) == 0x13);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc7_mmc(test_data) == 0x75);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc7_rohc(test_data) == 0x53);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc7_umts(test_data) == 0x61);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc8_autosar(test_data) == 0xDF);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc8_bluetooth(test_data) == 0x26);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc8_cdma2000(test_data) == 0xDA);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc8_darc(test_data) == 0x15);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc8_dvb_s2(test_data) == 0xBC);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc8_gsm_a(test_data) == 0x37);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc8_gsm_b(test_data) == 0x94);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc8_hitag(test_data) == 0xB4);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc8_i_432_1(test_data) == 0xA1);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc8_i_code(test_data) == 0x7E);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc8_lte(test_data) == 0xEA);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc8_maxim_dow(test_data) == 0xA1);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc8_mifare_mad(test_data) == 0x99);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc8_nrsc_5(test_data) == 0xF7);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc8_opensafety(test_data) == 0x3E);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc8_rohc(test_data) == 0xD0);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc8_sae_j1850(test_data) == 0x4B);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc8_smbus(test_data) == 0xF4);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc8_tech_3250(test_data) == 0x97);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc8_wcdma(test_data) == 0x25);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc10_atm(test_data) == 0x199);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc10_cdma2000(test_data) == 0x233);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc10_gsm(test_data) == 0x12A);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc11_flexray(test_data) == 0x5A3);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc11_umts(test_data) == 0x061);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc12_cdma2000(test_data) == 0xD4D);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc12_dect(test_data) == 0xF5B);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc12_gsm(test_data) == 0xB34);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc12_umts(test_data) == 0xDAF);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc13_bbc(test_data) == 0x04FA);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc14_darc(test_data) == 0x082D);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc14_gsm(test_data) == 0x30AE);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc15_can(test_data) == 0x059E);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc15_mpt1327(test_data) == 0x2566);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_arc(test_data) == 0xBB3D);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_cdma2000(test_data) == 0x4C06);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_cms(test_data) == 0xAEE7);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_dds_110(test_data) == 0x9ECF);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_dect_r(test_data) == 0x007E);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_dect_x(test_data) == 0x007F);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_dnp(test_data) == 0xEA82);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_en_13757(test_data) == 0xC2B7);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_genibus(test_data) == 0xD64E);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_gsm(test_data) == 0xCE3C);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_ibm_3740(test_data) == 0x29B1);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_ibm_sdlc(test_data) == 0x906E);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_iso_iec_14443_3_a(test_data) == 0xBF05);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_kermit(test_data) == 0x2189);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_lj1200(test_data) == 0xBDF4);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_m17(test_data) == 0x772B);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_maxim_dow(test_data) == 0x44C2);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_mcrf4xx(test_data) == 0x6F91);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_modbus(test_data) == 0x4B37);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_nrsc_5(test_data) == 0xA066);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_opensafety_a(test_data) == 0x5D38);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_opensafety_b(test_data) == 0x20FE);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_profibus(test_data) == 0xA819);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_riello(test_data) == 0x63D0);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_spi_fujitsu(test_data) == 0xE5CC);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_t10_dif(test_data) == 0xD0DB);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_teledisk(test_data) == 0x0FB3);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_tms37157(test_data) == 0x26B1);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_umts(test_data) == 0xFEE8);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_usb(test_data) == 0xB4C8);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc16_xmodem(test_data) == 0x31C3);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc17_can_fd(test_data) == 0x04F03);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc21_can_fd(test_data) == 0x0ED841);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc24_ble(test_data) == 0xC25A56);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc24_flexray_a(test_data) == 0x7979BD);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc24_flexray_b(test_data) == 0x1F23B8);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc24_interlaken(test_data) == 0xB4F3E6);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc24_lte_a(test_data) == 0xCDE703);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc24_lte_b(test_data) == 0x23EF52);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc24_openpgp(test_data) == 0x21CF02);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc24_os_9(test_data) == 0x200FA5);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc30_cdma(test_data) == 0x04C34ABF);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc31_philips(test_data) == 0x0CE9E46C);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc32_aixm(test_data) == 0x3010BF7F);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc32_autosar(test_data) == 0x1697D06A);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc32_base91_d(test_data) == 0x87315576);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc32(test_data) == 0xFC891918);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc32_cd_rom_edc(test_data) == 0x6EC2EDC4);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc32_cksum(test_data) == 0x765E7680);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc32c(test_data) == 0xE3069283);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc32_iso_hdlc(test_data) == 0xCBF43926);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc32_jamcrc(test_data) == 0x340BC6D9);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc32_mef(test_data) == 0xD2C22F51);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc32_mpeg2(test_data) == 0x0376E6E7);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc32_xfer(test_data) == 0xBD0BE338);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc40_gsm(test_data) == 0xD4164FC646);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc64_ecma_182(test_data) == 0x6C40DF5F0B497347);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc64_go_iso(test_data) == 0xB90956C775A41001);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc64_ms(test_data) == 0x75D4B74F024ECEEA);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc64_nvme(test_data) == 0xAE8B14860A799888);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc64_redis(test_data) == 0xE9C6D914C4B8D9CA);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc64_we(test_data) == 0x62EC59E3F1A4F00A);
    CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc64_xz(test_data) == 0x995DC9BBDF1939FA);
    // CHECK_AT_COMPILE_AND_RUN_TIME(crc::crc82_darc(test_data) == std::bitset<82>{"000010011110101010000011111101100010010100000010001110000000000111111101011000010010");
}
