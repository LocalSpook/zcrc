#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iostream>
#include <iterator>
#include <mutex>
#include <random>
#include <ranges>
#include <thread>
#include <vector>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <crc/crc.hpp>

namespace {

[[nodiscard]] std::vector<std::uint8_t> generate_random_data(const std::size_t bytes) {
    std::mt19937 rng {std::random_device{}()};
    std::uniform_int_distribution<> dist {0, 255};
    std::vector<std::uint8_t> ret {};
    ret.reserve(bytes);
    std::ranges::generate_n(std::back_insert_iterator(ret), static_cast<std::ptrdiff_t>(bytes), [&] { return dist(rng); });
    return ret;
}

template <typename CRC>
[[nodiscard]] constexpr auto compute(const crc::algorithm auto algo, const std::size_t threads, auto&& range) {
    const auto it {std::ranges::begin(range)};
    const auto end {std::ranges::end(range)};
    const auto len {static_cast<std::size_t>(end - it)};
    const std::size_t chunk_length {len / threads};

    std::mutex ret_mutex {};
    CRC ret {crc::zero_init};
    {
        std::vector<std::jthread> pool;
        pool.reserve(threads);
        for (const auto i : std::views::iota(static_cast<std::size_t>(0), threads)) {
            pool.emplace_back([&, i] () noexcept {
            const auto chunk_begin {(i == 0) ? it : (it + (len % chunk_length) + (i * chunk_length))};
            const auto chunk_end {it + (len % chunk_length) + ((i + 1) * chunk_length)};
                auto temp {crc::process_zero_bytes(
                    crc::process(
                        algo,
                        (i == 0) ? CRC {} : CRC{crc::zero_init},
                        chunk_begin,
                        chunk_end
                    ),
                    end - chunk_end
                )};
                const std::scoped_lock lock {ret_mutex};
                ret = crc::combine(ret, temp);
            });
        }
    }
    return crc::finalize(ret);
}

}

TEST_CASE("512 MiB parallel CRC32C slice-by-8") {
    const auto random_data {generate_random_data(1 << 29)};
    std::cout << std::format("Hardware threads: {}\n", std::jthread::hardware_concurrency());

    BENCHMARK("sequential") {
        return crc::crc32c::compute(crc::slice_by<8>, random_data);
    };

    BENCHMARK("crc::parallel") {
        return crc::crc32c::compute(crc::parallel<crc::slice_by<8>>, random_data);
    };

    for (const auto i : std::views::iota(2U, std::jthread::hardware_concurrency() + 1)) {
        BENCHMARK(std::format("{} threads", i)) {
            return compute<crc::crc32c>(crc::slice_by<8>, i, random_data);
        };
    }
}
