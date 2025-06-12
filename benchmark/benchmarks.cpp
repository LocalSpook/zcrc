#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <iostream>
#include <iterator>
#include <mutex>
#include <random>
#include <ranges>
#include <thread>
#include <utility>
#include <vector>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <zcrc/zcrc.hpp>

namespace {

[[nodiscard]] std::vector<std::uint8_t> generate_random_data(
    const std::size_t bytes, std::pair<std::uint8_t, std::uint8_t> range = {0, 255}
) {
    std::mt19937 rng {std::random_device{}()};
    std::uniform_int_distribution<> dist {range.first, range.second};
    std::vector<std::uint8_t> ret {};
    ret.reserve(bytes);
    std::ranges::generate_n(std::back_insert_iterator(ret), static_cast<std::ptrdiff_t>(bytes), [&] { return dist(rng); });
    return ret;
}

[[nodiscard]] std::vector<std::uint8_t> generate_random_cstr(const std::size_t bytes) {
    std::vector<std::uint8_t> ret {generate_random_data(bytes, {1, 255})};
    ret.push_back('\0');
    return ret;
}

template <typename CRC>
[[nodiscard]] constexpr auto compute(const zcrc::algorithm auto algo, const std::size_t threads, auto&& range) {
    const auto it {std::ranges::begin(range)};
    const auto end {std::ranges::end(range)};
    const auto len {static_cast<std::size_t>(end - it)};
    const std::size_t chunk_length {len / threads};

    std::mutex ret_mutex {};
    CRC ret {zcrc::zero_init};
    {
        std::vector<std::jthread> pool;
        pool.reserve(threads);
        for (const auto i : std::views::iota(static_cast<std::size_t>(0), threads)) {
            pool.emplace_back([&, i] () noexcept {
            const auto chunk_begin {(i == 0) ? it : (it + (len % chunk_length) + (i * chunk_length))};
            const auto chunk_end {it + (len % chunk_length) + ((i + 1) * chunk_length)};
                auto temp {zcrc::process_zero_bytes(
                    zcrc::process(
                        algo,
                        (i == 0) ? CRC {} : CRC{zcrc::zero_init},
                        chunk_begin,
                        chunk_end
                    ),
                    end - chunk_end
                )};
                const std::scoped_lock lock {ret_mutex};
                ret = zcrc::combine(ret, temp);
            });
        }
    }
    return zcrc::finalize(ret);
}

}

TEST_CASE("512 MiB parallel CRC32C slice-by-8") {
    const auto random_data {generate_random_data(1 << 29)};
    std::cout << std::format("Hardware threads: {}\n", std::jthread::hardware_concurrency());

    BENCHMARK("sequential") {
        return zcrc::crc32c::compute(zcrc::slice_by<8>, random_data);
    };

    BENCHMARK("zcrc::parallel") {
        return zcrc::crc32c::compute(zcrc::parallel<zcrc::slice_by<8>>, random_data);
    };

    for (const auto i : std::views::iota(2U, std::jthread::hardware_concurrency() + 1)) {
        BENCHMARK(std::format("{} threads", i)) {
            return compute<zcrc::crc32c>(zcrc::slice_by<8>, i, random_data);
        };
    }
}

namespace {

struct null_terminator_sentinel {
    [[nodiscard]] friend constexpr bool operator==(auto it, null_terminator_sentinel) noexcept {
        return *it == '\0';
    }
};

}

TEST_CASE("cstr") {
    for (std::size_t i {8}; i <= 1 << 20; i <<= 1) {
        for (std::size_t j {0}; j < 4; ++j) {
            const std::size_t len {i + (j * (((i << 1) - i) / 4))};

            const auto cstr1 {generate_random_cstr(len)};
            BENCHMARK(std::format("{}: strlen + sized", len)) {
                return zcrc::crc32c::compute(zcrc::slice_by<8>, cstr1.begin(),
                    cstr1.begin() + static_cast<std::ptrdiff_t>(std::strlen(reinterpret_cast<const char *>(cstr1.data()))));
            };

            const auto cstr2 {generate_random_cstr(len)};
            BENCHMARK(std::format("{}: unsized", len)) {
                return zcrc::crc32c::compute(zcrc::slice_by<8>, cstr2.begin(), null_terminator_sentinel {});
            };
        }
    }
}
