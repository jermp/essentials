#pragma once

#include <chrono>
#include <algorithm>
#include <numeric>
#include <vector>
#include <fstream>
#include <type_traits>

namespace essentials {

    typedef std::chrono::high_resolution_clock clock_type;
    typedef std::chrono::microseconds duration_type;

    static const uint64_t GB  = 1000 * 1000 * 1000;
    static const uint64_t GiB = uint64_t(1) << 30;
    static const uint64_t MB  = 1000 * 1000;
    static const uint64_t MiB = uint64_t(1) << 20;

    template<typename T>
    void check_if_pod() {
        if (!std::is_pod<T>::value) {
            throw std::runtime_error("type must be a POD");
        }
    }

    template<typename Vec>
    size_t vector_bytes(Vec const& vec) {
        return vec.size() * sizeof(vec.front()) + sizeof(typename Vec::size_type);
    }

    template<typename T>
    size_t pod_bytes(T pod) {
        check_if_pod<T>();
        return sizeof(pod);
    }

    size_t file_size(char const* filename) {
        std::ifstream is(filename, std::ios::binary | std::ios::ate);
        if (!is.good()) {
            throw std::runtime_error("Error in opening binary file.");
        }
        size_t bytes = (size_t)is.tellg();
        is.close();
        return bytes;
    }

    template<typename WordType = uint64_t>
    uint64_t words_for(uint64_t bits) {
        uint64_t word_bits = sizeof(WordType) * 8;
        return (bits + word_bits - 1) / word_bits;
    }

    template<typename T>
    inline void do_not_optimize_away(T&& datum) {
        asm volatile("" : "+r" (datum));
    }

    template<typename T>
    void save_pod(std::ostream& os, T const* val) {
        check_if_pod<T>();
        os.write(reinterpret_cast<char const*>(val), sizeof(T));
    }

    template<typename T>
    void load_pod(std::istream& is, T* val) {
        check_if_pod<T>();
        is.read(reinterpret_cast<char*>(val), sizeof(T));
    }

    template<typename T>
    void save_vec(std::ostream& os, std::vector<T> const& vec) {
        check_if_pod<T>();
        size_t n = vec.size();
        save_pod(os, &n);
        os.write(reinterpret_cast<char const*>(vec.data()),
                 static_cast<std::streamsize>(sizeof(T) * n));
    }

    template<typename T>
    void load_vec(std::istream& is, std::vector<T>& vec) {
        size_t n;
        load_pod(is, &n);
        vec.resize(n);
        is.read(reinterpret_cast<char*>(vec.data()),
                static_cast<std::streamsize>(sizeof(T) * n));
    }

    template<typename T>
    void save(T const& data_structure, char const* output_filename) {
        if (output_filename == nullptr) {
            throw std::runtime_error("You must specify the name of the output file.");
        }
        std::ofstream os(output_filename, std::ios::binary);
        data_structure.save(os);
        os.close();
    }

    template<typename T>
    void load(T& data_structure, char const* binary_filename)
    {
        std::ifstream is(binary_filename, std::ios::binary);
        if (!is.good()) {
            throw std::runtime_error("Error in opening binary file.");
        }
        data_structure.load(is);
        is.close();
    }

    struct json_lines {

        struct property {
            property(std::string n, std::string v)
                : name(n)
                , value(v)
            {}

            std::string name;
            std::string value;
        };

        void new_line() {
            m_properties.push_back(std::vector<property>());
        }

        void add(std::string name, std::string value)
        {
            if (!m_properties.size()) {
                new_line();
            }
            m_properties.back().emplace_back(name, value);
        }

        void save_to_file(char const* filename) const
        {
            std::ofstream out(filename);
            print_to(out);
            out.close();
        }

        void print() const {
            print_to(std::cerr);
        }

    private:
        std::vector<std::vector<property>> m_properties;

        template<typename T>
        void print_to(T& device) const
        {
            for (auto const& properties: m_properties) {
                device << "{";
                for (uint64_t i = 0; i != properties.size(); ++ i) {
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

    template<typename ClockType, typename DurationType>
    struct timer {

        void start() {
            m_start = ClockType::now();
        }

        void stop() {
            m_stop = ClockType::now();
            auto elapsed = std::chrono::duration_cast<DurationType>(m_stop - m_start);
            m_timings.push_back(elapsed.count());
        }

        size_t runs() const {
            return m_timings.size();
        }

        void discard_min_max()
        {
            if (runs() > 1) {
                m_timings.erase(std::max_element(m_timings.begin(),
                                                 m_timings.end()));
            }

            if (runs() > 1) {
                m_timings.erase(std::min_element(m_timings.begin(),
                                                 m_timings.end()));
            }
        }

        double average() {
            return std::accumulate(m_timings.begin(),
                                   m_timings.end(), 0.0) / runs();
        }

    private:
        typename ClockType::time_point m_start;
        typename ClockType::time_point m_stop;
        std::vector<double> m_timings;
    };

    typedef timer<clock_type, duration_type> timer_type;

    // TODO: function to visit a data structure recursively to compute the size breakdowns of its components
}
