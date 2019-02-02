#include <iostream>

#include "../include/essentials.hpp"

using namespace essentials;

int main() {
    json_lines jl;

    jl.add("sum", "13");
    double avg = 325.7;
    jl.add("average", std::to_string(avg));

    jl.new_line();
    jl.add("foo", std::to_string(avg));
    jl.add("bar", std::to_string(8894));
    jl.add("baz", "value");

    jl.print();

    return 0;
}
