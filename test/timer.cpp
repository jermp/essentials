#include <iostream>
#include <random>

#include "../include/essentials.hpp"

using namespace essentials;

std::vector<uint64_t> random_sequence(uint64_t n, uint64_t u) {
    std::vector<uint64_t> vec;
    vec.reserve(n);
    std::mt19937_64 rng;
    std::uniform_int_distribution<uint64_t> values_distribution(0, u);
    auto dice = bind(values_distribution, rng);
    for (uint64_t i = 0; i != n; ++i) {
        vec.push_back(dice());
    }
    return vec;
}

int main() {
    timer_type t;

    const uint64_t runs = 50;
    const uint64_t m = 1000000;
    const uint64_t n = 10000000;
    const uint64_t u = 100000000;
    auto sequence = random_sequence(n, u);
    auto queries = random_sequence(m, n);

    for (uint64_t run = 0; run != runs; ++run) {
        t.start();
        for (auto i : queries) {
            do_not_optimize_away(sequence[i]);
        }
        t.stop();
    }

    t.discard_min_max();
    double avg = t.average();

    std::cout << "\tMean per run: " << avg / duration_type::period::ratio::den
              << " [sec]\n";
    std::cout << "\tMean per query: " << avg / m << " [musec]";
    std::cout << std::endl;

    return 0;
}
