#include <iostream>

#include "../include/essentials.hpp"

using namespace essentials;

template <typename T>
struct basic {
    static_assert(std::is_pod<T>::value);

    basic() {}

    struct builder {
        builder() {}
        void reserve(size_t n) { m_data.reserve(n); }
        void push_back(T const& val) { m_data.push_back(val); }
        void build(basic& b) { b.m_data = std::move(m_data); }

    private:
        std::vector<T> m_data;
    };

    size_t size() const { return m_data.size(); }

    template <typename Visitor>
    void visit(Visitor& visitor) {
        visit(visitor, *this);
    }

    template <typename Visitor>
    void visit(Visitor& visitor) const {
        visit(visitor, *this);
    }

private:
    int x = 10;

    /* can use both... */
    owning_span<T> m_data;
    // std::vector<T> m_data;

    template <typename Visitor, typename F>
    static void visit(Visitor& visitor, F&& t) {
        visitor.visit(t.x);
        visitor.visit(t.m_data);
    }
};

template <typename T>
struct collection {
    collection() {
        int n = 13;
        {
            basic<uint32_t>::builder b;
            b.reserve(n);
            for (int i = 0; i != n; ++i) {
                b.push_back(i);
            }
            b.build(m_x);
        }
        m_data.resize(n);
        m_data2.resize(n);
        for (auto& d : m_data2) {
            d.resize(5, 0);
        }
    }

    size_t size() const { return m_data.size(); }

    template <typename Visitor>
    void visit(Visitor& visitor) {
        visit(visitor, *this);
    }

    template <typename Visitor>
    void visit(Visitor& visitor) const {
        visit(visitor, *this);
    }

private:
    basic<uint32_t> m_x;
    std::vector<basic<T>> m_data;
    std::vector<std::vector<uint64_t>> m_data2;

    template <typename Visitor, typename F>
    static void visit(Visitor& visitor, F&& t) {
        visitor.visit(t.m_x);
        visitor.visit(t.m_data);
        visitor.visit(t.m_data2);
    }
};

int main() {
    std::cout << "max resident set size: " << essentials::maxrss_in_bytes() << " bytes\n";

    const size_t universe = 1000;

    {
        uniform_int_rng<uint64_t> r(0, universe);

        collection<uint64_t> my_ds;

        // basic<uint64_t> my_ds;
        // {
        //     const size_t n = 10;
        //     basic<uint64_t>::builder my_ds_builder;
        //     my_ds_builder.reserve(n);
        //     for (size_t i = 0; i != n; ++i) {
        //         my_ds_builder.push_back(r.gen());
        //     }
        //     my_ds_builder.build(my_ds);
        // }

        char const* output_filename = "./my_ds.bin";
        size_t written_bytes = save(my_ds, output_filename);
        std::cout << "written bytes = " << written_bytes << std::endl;
    }

    {
        collection<uint64_t> my_ds;
        // basic<uint64_t> my_ds;
        char const* input_filename = "./my_ds.bin";
        size_t read_bytes = load(my_ds, input_filename);
        std::cout << "read bytes = " << read_bytes << std::endl;
        std::cout << convert(read_bytes, KB) << " [KB]" << std::endl;
        std::cout << convert(read_bytes, KiB) << " [KiB]" << std::endl;
        std::remove(input_filename);
        size_t bytes = print_size(my_ds, std::cout);
        std::cout << "computed bytes during parsing = " << bytes << std::endl;
    }

    std::cout << "max resident set size: " << essentials::maxrss_in_bytes() << " bytes\n";

    return 0;
}
