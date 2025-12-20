#include "cpuinfo.h"

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage_config.h"

#include <iostream>
#include <chrono>
#include <thread>
 
int* reserve_memory(int size_in_mb = 1000) {
    int array_size = size_in_mb * 256 * 1024;

    std::cout << "Reserve " << sizeof(int) * array_size / 1024 / 1024 << " MB." << std::endl;
    
    int *memory = nullptr;

    try {
        memory = new int[array_size];
    } catch(std::bad_alloc& ex) {
        std::cerr << ex.what() << std::endl;
        std::cin.get();
        exit(1);
    }

    for(int i = 0; i < array_size; ++i) {
        memory[i] = i;
    }

    return memory;
}

std::string version_string() { return "BigPod 0.0.1\n"; }

ABSL_FLAG(int, ram_usage, 1, "RAM usage in GB");
ABSL_FLAG(int, ram_hold_time, 10, "Time to reserve RAM in seconds");

int main(int argc, char **argv) {
    absl::FlagsUsageConfig flags_config;
    flags_config.version_string = &version_string;
    absl::SetFlagsUsageConfig(flags_config);
    absl::ParseCommandLine(argc, argv);

    bool result = cpuinfo_initialize();
    if(!result) {
        return 1;
    }

    std::cout << "Running on " << cpuinfo_get_package(0)->name << "CPU" << std::endl;
    const size_t l1_size = cpuinfo_get_processor(0)->cache.l1d->size;
    std::cout << "L1 Cache size " << l1_size << std::endl;

    bool avx_support = cpuinfo_has_x86_avx();
    std::cout << "AVX support: " << avx_support << std::endl;

    cpuinfo_deinitialize();

    using namespace std::chrono_literals;

    int gb_to_reserve = absl::GetFlag(FLAGS_ram_usage);

    int *p[1000] = {nullptr};

    for(int i = 1; i <= gb_to_reserve; i++) {
        p[i] = reserve_memory();
        std::cout << "Total mem: " << i << " GB" << std::endl;
    }
  
    std::cout << "Now sleep..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(absl::GetFlag(FLAGS_ram_hold_time)));
 
    for(int i = 1; i < gb_to_reserve; i++) {
        delete [] p[i];
    }

    std::cout << "Goodbye!" << std::endl;

    return 0;
}
