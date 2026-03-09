#include <iostream>

#include "../include/essentials.hpp"

// 1. Define a data structure that uses owning_span and implements the visit() pattern
struct some_data {
    uint32_t id;
    essentials::owning_span<int> payload;

    template <typename Visitor>
    void visit(Visitor& visitor) {
        visitor.visit(id);
        visitor.visit(payload);
    }

    template <typename Visitor>
    void visit(Visitor& visitor) const {
        visitor.visit(id);
        visitor.visit(payload);
    }
};

int main() {
    const char* filename = "mmap_test.bin";

    // ==========================================
    // PHASE 1: Create data and serialize to disk
    // ==========================================
    std::cout << "--- PHASE 1: Serialization ---\n";
    {
        some_data original_data;
        original_data.id = 42;

        // Use a vector to initialize the owning_span (Heap-owned model)
        std::vector<int> temp_vec = {10, 20, 30, 40, 50};
        original_data.payload = essentials::owning_span<int>(std::move(temp_vec));

        // Save it using the essentials::saver
        size_t bytes_written = essentials::save(original_data, filename);
        std::cout << "Saved " << bytes_written << " bytes to " << filename << "\n";
    }

    // ==========================================
    // PHASE 2: Memory-Map the file
    // ==========================================
    std::cout << "\n--- PHASE 2: Memory Mapping ---\n";

    {
        some_data mapped_data;
        auto mmap_owner = essentials::mmap(mapped_data, filename);

        // Verify the data
        std::cout << "Loaded ID: " << mapped_data.id << "\n";
        std::cout << "Payload size: " << mapped_data.payload.size() << "\n";
        std::cout << "Payload values: ";
        for (int val : mapped_data.payload) {
            std::cout << val << " ";
        }
        std::cout << "\n";

        std::cout << "payload raw pointer: " << static_cast<const void*>(mapped_data.payload.data())
                  << "\n";

        // The payload pointer should be slightly offset from the mmap_base
        // because the ID (42) and the vector size (5) were saved before the actual array data.
        size_t expected_offset = sizeof(uint32_t) + sizeof(size_t);
        std::cout << "Expected offset:     " << expected_offset << " bytes\n";

    }  // mapped_data goes out of scope here. The mmap_owner ref count drops to 0, triggering the
       // munmap.

    std::cout << "\nProgram complete. Memory safely unmapped.\n";
    std::remove(filename);

    return 0;
}
