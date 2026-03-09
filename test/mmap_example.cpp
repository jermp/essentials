#include <iostream>

#include "../include/essentials.hpp"

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
    size_t bytes_written = 0;
    size_t bytes_mapped = 0;

    // ==========================================
    // PHASE 1: Create data and serialize to disk
    // ==========================================
    std::cout << "--- PHASE 1: Serialization ---\n";
    {
        some_data original_data;
        original_data.id = 42;
        std::vector<int> temp_vec = {10, 20, 30, 40, 50};
        original_data.payload = essentials::owning_span<int>(std::move(temp_vec));
        bytes_written = essentials::save(original_data, filename);
        std::cout << "Saved " << bytes_written << " bytes to " << filename << "\n";
    }

    // ==========================================
    // PHASE 2: Memory-Map the file
    // ==========================================
    std::cout << "\n--- PHASE 2: Memory Mapping ---\n";

    {
        some_data mapped_data;
        bytes_mapped = essentials::mmap(mapped_data, filename);
        std::cout << "Mapped " << bytes_mapped << " bytes from " << filename << "\n";

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
        std::cout << "Expected offset:     " << expected_offset << " bytes" << std::endl;
    }

    std::remove(filename);

    return 0;
}
