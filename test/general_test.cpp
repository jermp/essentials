#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <type_traits>

#include "../include/essentials.hpp"

// ==========================================
// MOCK STRUCTURES FOR TESTING
// ==========================================

struct NonPodStruct {
    std::vector<int> data;
    virtual ~NonPodStruct() = default;  // Makes it strictly non-POD/non-trivial
};

struct PodStruct {
    uint32_t a;
    double b;
};

// ==========================================
// TEST MACROS
// ==========================================

#define RUN_TEST(TestFunc)                                      \
    std::cout << "[TEST] " << #TestFunc << "..." << std::flush; \
    TestFunc();                                                 \
    std::cout << " PASS\n";

#define ASSERT_THROWS(Expr, ExceptionType)                            \
    do {                                                              \
        bool threw = false;                                           \
        try {                                                         \
            Expr;                                                     \
        } catch (ExceptionType const&) {                              \
            threw = true;                                             \
        } catch (...) {                                               \
        }                                                             \
        assert(threw && "Expected exception was not thrown: " #Expr); \
        (void)threw;                                                  \
    } while (0)

// ==========================================
// THE PEDANTIC TESTS
// ==========================================

void test_type_traits() {
    // Check POD trait
    assert(essentials::is_pod<int>::value == true);
    assert(essentials::is_pod<PodStruct>::value == true);
    assert(essentials::is_pod<NonPodStruct>::value == false);
    assert(essentials::is_pod<std::vector<int>>::value == false);

    // Check owning_span trait
    assert(essentials::is_owning_span_v<essentials::owning_span<int>> == true);
    assert(essentials::is_owning_span_v<std::vector<int>> == false);
    assert(essentials::is_owning_span_v<int> == false);
}

void test_advanced_timer() {
    essentials::timer_type t;

    uint64_t ignore = 0;
    for (int i = 0; i < 6; ++i) {
        t.start();
        for (int j = 0; j < 10000000; ++j) ignore ^= j;
        t.stop();
    }
    essentials::do_not_optimize_away(ignore);

    assert(t.runs() == 6);

    // Discard the slowest, fastest, and the first (warmup)
    t.discard_first();
    assert(t.runs() == 5);

    t.discard_max();
    assert(t.runs() == 4);

    t.discard_min();
    assert(t.runs() == 3);

    assert(t.average() > 0.0);

    t.reset();
    assert(t.runs() == 0);
    assert(t.elapsed() == 0.0);
}

void test_owning_span_models() {
    // Model 1: Heap-Owned (Rvalue / Moved)
    {
        std::vector<int> src = {1, 2, 3};
        essentials::owning_span<int> span(std::move(src));
        assert(span.size() == 3);
        assert(span.front() == 1 && span.back() == 3);
        assert(!span.empty());
    }

    // Model 2: Externally-Owned (e.g., mock mmap)
    {
        int raw_data[] = {10, 20, 30, 40};
        bool deleter_called = false;

        {
            std::shared_ptr<void> fake_owner(&raw_data, [&](void*) { deleter_called = true; });
            essentials::owning_span<int> span(raw_data, 4, fake_owner);
            assert(span.size() == 4);
            assert(span[2] == 30);
        }
        // Span and owner went out of scope, deleter should have fired
        assert(deleter_called);
    }

    // Model 3: Unowned (Raw Pointer)
    {
        int raw_data[] = {99, 100};
        essentials::owning_span<int> span(raw_data, 2);
        assert(span.size() == 2);
        assert(span[0] == 99);

        // Test clear and swap
        essentials::owning_span<int> empty_span;
        assert(empty_span.empty());

        span.swap(empty_span);
        assert(span.empty());
        assert(empty_span.size() == 2);

        empty_span.clear();
        assert(empty_span.empty());
        assert(empty_span.data() == nullptr);
    }
}

void test_serialization_edge_cases() {
    const char* file = "test_empty.bin";

    // Test serializing empty containers
    std::vector<int> empty_vec;
    essentials::owning_span<int> empty_span;

    {
        essentials::saver s(file);
        s.visit(empty_vec);
        s.visit(empty_span);
    }

    {
        std::vector<int> loaded_vec;
        essentials::owning_span<int> loaded_span;

        essentials::loader l(file);
        l.visit(loaded_vec);
        l.visit(loaded_span);

        assert(loaded_vec.empty());
        assert(loaded_span.empty());
    }

    std::remove(file);
}

void test_allocator_exceptions() {
    // Test the contiguous_memory_allocator boundary checks
    // We will spoof a visitor by passing a small dummy buffer
    uint8_t dummy_buffer[10];
    const char* dummy_file = "dummy_file.bin";  // Just needs to exist for ifstream

    // Create dummy file
    FILE* f = fopen(dummy_file, "w");
    if (f) {
        fputs("0", f);
        fclose(f);
    }

    essentials::contiguous_memory_allocator::visitor v(dummy_buffer, 10, dummy_file);

    // Consume 4 bytes -> OK (Total: 4/10)
    v.consume(4);
    assert(v.allocated() == 4);

    // Consume 6 bytes -> OK (Total: 10/10)
    v.consume(6);
    assert(v.allocated() == 10);

    // Consume 1 more byte -> MUST THROW
    ASSERT_THROWS(v.consume(1), std::runtime_error);

    std::remove(dummy_file);
}

void test_json_lines_edge_cases() {
    const char* file = "test_json_edge.jsonl";
    {
        essentials::json_lines j;
        j.add("empty_string", "");
        j.add("zero_val", 0);
        j.add("bool_false", false);
        j.save_to_file(file);
    }
    assert(essentials::file_size(file) > 0);
    std::remove(file);
}

int main() {
    std::cout << "Running a simple test suite...\n";
    std::cout << "-------------------------------------------\n";

    RUN_TEST(test_type_traits);
    RUN_TEST(test_advanced_timer);
    RUN_TEST(test_owning_span_models);
    RUN_TEST(test_serialization_edge_cases);
    RUN_TEST(test_allocator_exceptions);
    RUN_TEST(test_json_lines_edge_cases);

    std::cout << "-------------------------------------------\n";
    std::cout << "All tests passed!\n";

    return 0;
}
