#include "simdjson.h"

#include <iostream>

int main() {
    std::cout << "simdjson v" << SIMDJSON_VERSION << std::endl;
    std::cout << "Detected the best implementation for your machine: "
              << simdjson::get_active_implementation()->name();
    std::cout << " (" << simdjson::get_active_implementation()->description()
              << ")" << std::endl;

    return 0;
}
