#pragma once

#include <algorithm>
#include <chrono>
#include <fstream>
#include <numeric>
#include <random>
#include <type_traits>
#include <vector>

namespace essentials {

typedef std::chrono::high_resolution_clock clock_type;
typedef std::chrono::microseconds duration_type;

static const uint64_t GB = 1000 * 1000 * 1000;
static const uint64_t GiB = uint64_t(1) << 30;
static const uint64_t MB = 1000 * 1000;
static const uint64_t MiB = uint64_t(1) << 20;
static const uint64_t KB = 1000;
static const uint64_t KiB = uint64_t(1) << 10;

#define convert(bytes, UNIT) static_cast<double>(bytes) / UNIT

template <typename T>
void check_if_pod() {
    if (!std::is_pod<T>::value) {
        throw std::runtime_error("type must be a POD");
    }
}

template <typename Vec>
size_t vec_bytes(Vec const& vec) {
    return vec.size() * sizeof(vec.front()) + sizeof(typename Vec::size_type);
}

template <typename T>
size_t pod_bytes(T pod) {
    check_if_pod<T>();
    return sizeof(pod);
}

size_t file_size(char const* filename) {
    std::ifstream is(filename, std::ios::binary | std::ios::ate);
    if (!is.good()) {
        throw std::runtime_error(
            "Error in opening binary "
            "file.");
    }
    size_t bytes = (size_t)is.tellg();
    is.close();
    return bytes;
}

template <typename WordType = uint64_t>
uint64_t words_for(uint64_t bits) {
    uint64_t word_bits = sizeof(WordType) * 8;
    return (bits + word_bits - 1) / word_bits;
}

template <typename T>
inline void do_not_optimize_away(T&& datum) {
    asm volatile("" : "+r"(datum));
}

template <typename T>
void load_pod(std::istream& is, T& val) {
    check_if_pod<T>();
    is.read(reinterpret_cast<char*>(&val), sizeof(T));
}

template <typename T>
void load_vec(std::istream& is, std::vector<T>& vec) {
    size_t n;
    load_pod(is, n);
    vec.resize(n);
    is.read(reinterpret_cast<char*>(vec.data()),
            static_cast<std::streamsize>(sizeof(T) * n));
}

template <typename T>
void save_pod(std::ostream& os, T const& val) {
    check_if_pod<T>();
    os.write(reinterpret_cast<char const*>(&val), sizeof(T));
}

template <typename T>
void save_vec(std::ostream& os, std::vector<T> const& vec) {
    check_if_pod<T>();
    size_t n = vec.size();
    save_pod(os, n);
    os.write(reinterpret_cast<char const*>(vec.data()),
             static_cast<std::streamsize>(sizeof(T) * n));
}

struct json_lines {
    struct property {
        property(std::string n, std::string v) : name(n), value(v) {}

        std::string name;
        std::string value;
    };

    void new_line() {
        m_properties.push_back(std::vector<property>());
    }

    void add(std::string name, std::string value) {
        if (!m_properties.size()) {
            new_line();
        }
        m_properties.back().emplace_back(name, value);
    }

    void save_to_file(char const* filename) const {
        std::ofstream out(filename);
        print_to(out);
        out.close();
    }

    void print() const {
        print_to(std::cerr);
    }

private:
    std::vector<std::vector<property>> m_properties;

    template <typename T>
    void print_to(T& device) const {
        for (auto const& properties : m_properties) {
            device << "{";
            for (uint64_t i = 0; i != properties.size(); ++i) {
                auto const& p = properties[i];
                device << "\"" << p.name << "\": \"" << p.value << "\"";
                if (i != properties.size() - 1) {
                    device << ", ";
                }
            }
            device << "}\n";
        }
    }
};

template <typename ClockType, typename DurationType>
struct timer {
    void start() {
        m_start = ClockType::now();
    }

    void stop() {
        m_stop = ClockType::now();
        auto elapsed =
            std::chrono::duration_cast<DurationType>(m_stop - m_start);
        m_timings.push_back(elapsed.count());
    }

    size_t runs() const {
        return m_timings.size();
    }

    void discard_min_max() {
        if (runs() > 1) {
            m_timings.erase(
                std::max_element(m_timings.begin(), m_timings.end()));
        }

        if (runs() > 1) {
            m_timings.erase(
                std::min_element(m_timings.begin(), m_timings.end()));
        }
    }

    double average() {
        return std::accumulate(m_timings.begin(), m_timings.end(), 0.0) /
               runs();
    }

private:
    typename ClockType::time_point m_start;
    typename ClockType::time_point m_stop;
    std::vector<double> m_timings;
};

typedef timer<clock_type, duration_type> timer_type;

template <typename IntType>
struct uniform_int_rng {
    uniform_int_rng(size_t from, size_t to) : m_distr(from, to) {}

    IntType gen() {
        return m_distr(m_rng);
    }

private:
    std::mt19937_64 m_rng;
    std::uniform_int_distribution<IntType> m_distr;
};

struct loader {
    loader(char const* filename) : m_is(filename, std::ios::binary) {
        if (!m_is.good()) {
            throw std::runtime_error(
                "Error in opening binary "
                "file.");
        }
    }

    ~loader() {
        m_is.close();
    }

    template <typename T>
    typename std::enable_if<std::is_pod<T>::value, void>::type visit(T& val) {
        load_pod(m_is, val);
    }

    template <typename T>
    typename std::enable_if<!std::is_pod<T>::value, void>::type visit(T& val) {
        val.visit(*this);
    }

    template <typename T>
    typename std::enable_if<std::is_pod<T>::value, void>::type visit(
        std::vector<T>& vec) {
        load_vec(m_is, vec);
    }

    template <typename T>
    typename std::enable_if<!std::is_pod<T>::value, void>::type visit(
        std::vector<T>& vec) {
        size_t n;
        visit(n);
        vec.resize(n);
        for (auto& v : vec) {
            v.visit(*this);
        }
    }

    size_t bytes() {
        return m_is.tellg();
    }

private:
    std::ifstream m_is;
};

struct saver {
    saver(char const* filename) : m_os(filename, std::ios::binary) {
        if (!m_os.good()) {
            throw std::runtime_error(
                "Error in opening binary "
                "file.");
        }
    }

    ~saver() {
        m_os.close();
    }

    template <typename T>
    typename std::enable_if<std::is_pod<T>::value, void>::type visit(T& val) {
        save_pod(m_os, val);
    }

    template <typename T>
    typename std::enable_if<!std::is_pod<T>::value, void>::type visit(T& val) {
        val.visit(*this);
    }

    template <typename T>
    typename std::enable_if<std::is_pod<T>::value, void>::type visit(
        std::vector<T>& vec) {
        save_vec(m_os, vec);
    }

    template <typename T>
    typename std::enable_if<!std::is_pod<T>::value, void>::type visit(
        std::vector<T>& vec) {
        size_t n = vec.size();
        visit(n);
        for (auto& v : vec) {
            v.visit(*this);
        }
    }

    size_t bytes() {
        return m_os.tellp();
    }

private:
    std::ofstream m_os;
};

struct sizer {
    sizer() : m_root(0, 0), m_current(&m_root) {}

    struct node {
        node(size_t b, size_t d) : bytes(b), depth(d) {}

        size_t bytes;
        size_t depth;
        std::vector<node> children;
    };

    template <typename T>
    typename std::enable_if<std::is_pod<T>::value, void>::type visit(T& val) {
        node n(pod_bytes(val), m_current->depth + 1);
        m_current->children.push_back(n);
        m_current->bytes += n.bytes;
    }

    template <typename T>
    typename std::enable_if<!std::is_pod<T>::value, void>::type visit(T& val) {
        val.visit(*this);
    }

    template <typename T>
    typename std::enable_if<std::is_pod<T>::value, void>::type visit(
        std::vector<T>& vec) {
        node n(vec_bytes(vec), m_current->depth + 1);
        m_current->children.push_back(n);
        m_current->bytes += n.bytes;
    }

    template <typename T>
    typename std::enable_if<!std::is_pod<T>::value, void>::type visit(
        std::vector<T>& vec) {
        size_t n = vec.size();
        m_current->bytes += pod_bytes(n);
        node* parent = m_current;
        for (auto& v : vec) {
            node n(0, parent->depth + 1);
            parent->children.push_back(n);
            m_current = &parent->children.back();
            v.visit(*this);
            parent->bytes += m_current->bytes;
        }
    }

    template <typename Device>
    void print(node const& n, size_t total_bytes, Device& device) const {
        auto indent = std::string(n.depth * 4, ' ');
        device << indent << "bytes = " << n.bytes << " ("
               << n.bytes * 100.0 / total_bytes << "%)" << std::endl;
        for (auto const& child : n.children) {
            device << indent;
            print(child, total_bytes, device);
        }
    }

    void print() const {
        print(m_root, bytes(), std::cerr);
    }

    size_t bytes() const {
        return m_root.bytes;
    }

private:
    node m_root;
    node* m_current;
};

template <typename Data, typename Visitor>
size_t visit(Data& structure, char const* filename) {
    Visitor visitor(filename);
    structure.visit(visitor);
    return visitor.bytes();
}

template <typename Data>
size_t load(Data& structure, char const* filename) {
    return visit<Data, loader>(structure, filename);
}

template <typename Data>
size_t save(Data& structure, char const* filename) {
    return visit<Data, saver>(structure, filename);
}

template <typename Data>
void print_size(Data& structure) {
    sizer visitor;
    structure.visit(visitor);
    visitor.print();
}

}  // namespace essentials
