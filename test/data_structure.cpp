#include <iostream>

#include "../include/essentials.hpp"

using namespace essentials;

template <typename T>
struct basic {
    static_assert(std::is_pod<T>::value, "");

    basic() {}

    void reserve(size_t n) {
        m_data.reserve(n);
    }

    void push_back(T const& val) {
        m_data.push_back(val);
    }

    void shrink_to_size() {
        m_data.shrink_to_size();
    }

    size_t size() const {
        return m_size;
    }

    void save(std::ostream& os) const {
        save_pod(os, &m_size);
        save_vec(os, m_data);
    }

    void load(std::istream& is) {
        load_pod(is, &m_size);
        load_vec(is, m_data);
    }

private:
    size_t m_size;
    std::vector<T> m_data;
};

template <typename T>
struct collection {
    collection() : m_size(0) {}

    void resize(size_t n) {
        m_data.resize(n);
        m_size = m_data.size();
    }

    void reserve(uint32_t index, size_t n) {
        assert(index < size());
        m_data[index].reserve(n);
    }

    void push_back(uint32_t index, T const& val) {
        assert(index < size());
        m_data[index].push_back(val);
    }

    void shrink_to_size() {
        for (auto& d : m_data) {
            d.shrink_to_size();
        }
    }

    size_t size() const {
        return m_size;
    }

    void save(std::ostream& os) const {
        save_pod(os, &m_size);
        for (auto const& d : m_data) {
            d.save(os);
        }
    }

    void load(std::istream& is) {
        load_pod(is, &m_size);
        resize(m_size);
        for (auto& d : m_data) {
            d.load(is);
        }
    }

private:
    size_t m_size;
    std::vector<basic<T>> m_data;
};

int main() {
    const size_t universe = 1000;
    const size_t n = 10;

    {
        collection<uint64_t> my_ds;
        uniform_int_rng<uint64_t> r(0, universe);
        my_ds.resize(n);

        for (size_t i = 0, size = 4; i != n; ++i, size *= 2) {
            my_ds.reserve(i, size);
            for (size_t k = 0; k != size; ++k) {
                my_ds.push_back(i, r.gen());
            }
        }

        char const* output_filename = "./my_ds.bin";
        save(my_ds, output_filename);
    }

    {
        collection<uint64_t> my_ds;
        char const* input_filename = "./my_ds.bin";
        size_t read_bytes = load(my_ds, input_filename);
        std::cout << "read bytes = " << read_bytes << std::endl;
        std::cout << convert(read_bytes, KB) << " [KB]" << std::endl;
        std::cout << convert(read_bytes, KiB) << " [KiB]" << std::endl;
        std::remove(input_filename);
    }

    return 0;
}
