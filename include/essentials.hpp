#pragma once

#include <algorithm>
#include <chrono>
#include <fstream>
#include <numeric>
#include <random>
#include <type_traits>
#include <vector>
#include <dirent.h>
#include <cstring>
#include <locale>
#include <unistd.h>
#include <sys/stat.h>
#include <cassert>

namespace essentials {

typedef std::chrono::high_resolution_clock clock_type;
typedef std::chrono::microseconds duration_type;

void logger(std::string const& msg) {
    time_t t = std::time(nullptr);
    std::locale loc;
    const std::time_put<char>& tp = std::use_facet<std::time_put<char>>(loc);
    const char* fmt = "%F %T";
    tp.put(std::cout, std::cout, ' ', std::localtime(&t), fmt,
           fmt + strlen(fmt));
    std::cout << ": " << msg << std::endl;
}

static const uint64_t GB = 1000 * 1000 * 1000;
static const uint64_t GiB = uint64_t(1) << 30;
static const uint64_t MB = 1000 * 1000;
static const uint64_t MiB = uint64_t(1) << 20;
static const uint64_t KB = 1000;
static const uint64_t KiB = uint64_t(1) << 10;

double convert(size_t bytes, uint64_t unit) {
    return static_cast<double>(bytes) / unit;
}

template <typename T>
void check_if_pod() {
    if (!std::is_pod<T>::value) {
        throw std::runtime_error("type must be a POD");
    }
}

template <typename T>
size_t vec_bytes(T const& vec) {
    return vec.size() * sizeof(vec.front()) + sizeof(typename T::size_type);
}

template <typename T>
size_t pod_bytes(T const& pod) {
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
        property(std::string n, std::string v)
            : name(n)
            , value(v) {}

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

    void reset() {
        m_timings.clear();
    }

    double min() const {
        return *std::min_element(m_timings.begin(), m_timings.end());
    }

    double max() const {
        return *std::max_element(m_timings.begin(), m_timings.end());
    }

    void discard_first() {
        if (runs()) {
            m_timings.erase(m_timings.begin());
        }
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

    double elapsed() {
        return std::accumulate(m_timings.begin(), m_timings.end(), 0.0);
    }

    double average() {
        return elapsed() / runs();
    }

private:
    typename ClockType::time_point m_start;
    typename ClockType::time_point m_stop;
    std::vector<double> m_timings;
};

typedef timer<clock_type, duration_type> timer_type;

unsigned get_random_seed() {
    return std::chrono::system_clock::now().time_since_epoch().count();
}

template <typename IntType>
struct uniform_int_rng {
    uniform_int_rng(IntType from, IntType to, unsigned seed = 13)
        : m_rng(seed)
        , m_distr(from, to) {}

    IntType gen() {
        return m_distr(m_rng);
    }

private:
    std::mt19937_64 m_rng;
    std::uniform_int_distribution<IntType> m_distr;
};

#define is_pod(T) typename std::enable_if<std::is_pod<T>::value>::type
#define is_not_pod(T) typename std::enable_if<!std::is_pod<T>::value>::type

struct loader {
    loader(char const* filename)
        : m_is(filename, std::ios::binary) {
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
    is_pod(T) visit(T& val) {
        load_pod(m_is, val);
    }

    template <typename T>
    is_not_pod(T) visit(T& val) {
        val.visit(*this);
    }

    template <typename T>
    is_pod(T) visit(std::vector<T>& vec) {
        load_vec(m_is, vec);
    }

    template <typename T>
    is_not_pod(T) visit(std::vector<T>& vec) {
        size_t n;
        visit(n);
        vec.resize(n);
        for (auto& v : vec) {
            visit(v);
        }
    }

    size_t bytes() {
        return m_is.tellg();
    }

private:
    std::ifstream m_is;
};

struct saver {
    saver(char const* filename)
        : m_os(filename, std::ios::binary) {
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
    is_pod(T) visit(T& val) {
        save_pod(m_os, val);
    }

    template <typename T>
    is_not_pod(T) visit(T& val) {
        val.visit(*this);
    }

    template <typename T>
    is_pod(T) visit(std::vector<T>& vec) {
        save_vec(m_os, vec);
    }

    template <typename T>
    is_not_pod(T) visit(std::vector<T>& vec) {
        size_t n = vec.size();
        visit(n);
        for (auto& v : vec) {
            visit(v);
        }
    }

    size_t bytes() {
        return m_os.tellp();
    }

private:
    std::ofstream m_os;
};

struct sizer {
    sizer(std::string const& root_name = "")
        : m_root(0, 0, root_name)
        , m_current(&m_root) {}

    struct node {
        node(size_t b, size_t d, std::string const& n = "")
            : bytes(b)
            , depth(d)
            , name(n) {}

        size_t bytes;
        size_t depth;
        std::string name;
        std::vector<node> children;
    };

    template <typename T>
    is_pod(T) visit(T& val) {
        node n(pod_bytes(val), m_current->depth + 1, typeid(T).name());
        m_current->children.push_back(n);
        m_current->bytes += n.bytes;
    }

    template <typename T>
    is_not_pod(T) visit(T& val) {
        val.visit(*this);
    }

    template <typename T>
    is_pod(T) visit(std::vector<T>& vec) {
        node n(vec_bytes(vec), m_current->depth + 1,
               typeid(std::vector<T>).name());
        m_current->children.push_back(n);
        m_current->bytes += n.bytes;
    }

    template <typename T>
    is_not_pod(T) visit(std::vector<T>& vec) {
        size_t n = vec.size();
        m_current->bytes += pod_bytes(n);
        node* parent = m_current;
        for (auto& v : vec) {
            node n(0, parent->depth + 1, typeid(T).name());
            parent->children.push_back(n);
            m_current = &parent->children.back();
            visit(v);
            parent->bytes += m_current->bytes;
        }
        m_current = parent;
    }

    template <typename Device>
    void print(node const& n, size_t total_bytes, Device& device) const {
        auto indent = std::string(n.depth * 4, ' ');
        device << indent << "'" << n.name << "' - bytes = " << n.bytes << " ("
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
    visitor.visit(structure);
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
    sizer visitor(typeid(Data).name());
    visitor.visit(structure);
    visitor.print();
}

struct directory {
    struct file_name {
        std::string name;
        std::string fullpath;
        std::string extension;
    };

    ~directory() {
        for (int i = 0; i != items(); ++i) {
            free(m_items_names[i]);
        }
        free(m_items_names);
    }

    directory(std::string const& name)
        : m_name(name) {
        m_n = scandir(m_name.c_str(), &m_items_names, NULL, alphasort);
        if (m_n < 0) {
            throw std::runtime_error("error during scandir");
        }
    }

    std::string const& name() const {
        return m_name;
    }

    int items() const {
        return m_n;
    }

    struct iterator {
        iterator(directory const* d, int i)
            : m_d(d)
            , m_i(i) {}

        file_name operator*() {
            file_name fn;
            fn.name = m_d->m_items_names[m_i]->d_name;
            fn.fullpath = m_d->name() + "/" + fn.name;
            size_t p = fn.name.find_last_of(".");
            fn.extension = fn.name.substr(p + 1);
            return fn;
        }

        void operator++() {
            ++m_i;
        }

        bool operator==(iterator const& rhs) const {
            return m_i == rhs.m_i;
        }

        bool operator!=(iterator const& rhs) const {
            return !(*this == rhs);
        }

    private:
        directory const* m_d;
        int m_i;
    };

    iterator begin() {
        return iterator(this, 0);
    }

    iterator end() {
        return iterator(this, items());
    }

private:
    std::string m_name;
    struct dirent** m_items_names;
    int m_n;
};

bool create_directory(std::string const& name) {
    if (mkdir(name.c_str(), 0777) != 0) {
        if (errno == EEXIST) {
            std::cerr << "directory already exists" << std::endl;
        }
        return false;
    }
    return true;
}

bool remove_directory(std::string const& name) {
    return rmdir(name.c_str()) == 0;
}

}  // namespace essentials
