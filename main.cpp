#include "CLI/App.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

#include <cstddef>
#include <immintrin.h>
#include <type_traits>

#include <algorithm>
#include <iterator>
#include <numeric>

#include <random>

#include <CLI/CLI.hpp>

#if defined(WIN32)
#include <windows.h>
#endif

enum BenchmarkType { Sort, Vector };

void sum_double(double src0[], double src1[], double dst[], size_t len) {
  // floating-point, double precision
  __m128d lhs_vec, rhs_vec;
  for (size_t i = 0; i < len; i += 2) {
    // loading of two double values
    lhs_vec = _mm_loadu_pd(src0);
    rhs_vec = _mm_loadu_pd(src1);
    // adding
    lhs_vec = _mm_add_pd(lhs_vec, rhs_vec);
    // write
    _mm_storeu_pd(dst, lhs_vec);
  }
}

void sort(int *arr, size_t size) {
  for (int i = 0; i < size - 1; i++) {
    for (int j = i + 1; j < size; j++) {
      if (arr[i] > arr[j]) {
        std::swap(arr[i], arr[j]);
      }
    }
  }
}

std::vector<int> generate_sort_data(int n) {
  std::vector<int> arr(n);
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_int_distribution<std::mt19937::result_type> dist6(1, 9999);

  std::generate(arr.begin(), arr.end(),
                [&dist6, &rng]() { return dist6(rng); });
  return arr;
}

std::vector<double> generate_double(int n) {
  std::vector<double> arr(n);
  std::random_device dev;
  std::mt19937 rng(dev());
  std::uniform_real_distribution<> dist6(1, 9999);

  std::generate(arr.begin(), arr.end(),
                [&dist6, &rng]() { return dist6(rng); });
  return arr;
}

std::chrono::high_resolution_clock::duration do_work(BenchmarkType type,
                                                     int n) {
  if (type == Sort) {
    auto sort_data = generate_sort_data(n);

    auto start = std::chrono::high_resolution_clock::now();
    sort(sort_data.data(), n);
    auto end = std::chrono::high_resolution_clock::now();

    return end - start;
  } else {
    auto lhs = generate_double(2);
    auto rhs = generate_double(2);
    std::vector<double> dst(2);

    auto start = std::chrono::high_resolution_clock::now();
    sum_double(lhs.data(), rhs.data(), dst.data(), n);
    auto end = std::chrono::high_resolution_clock::now();

    return end - start;
  }
}

void setup_affinity(std::thread &thread, int cpu_id) {
#if defined(__linux__)
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu_id, &cpuset);
  int rc = pthread_setaffinity_np(thread.native_handle(), sizeof(cpu_set_t),
                                  &cpuset);
#elif defined(WIN32)
  DWORD_PTR dw = SetThreadAffinityMask(thread.native_handle(), DWORD_PTR(1) << i);
#endif
}

int main(int argc, char *argv[]) {
  CLI::App app;

  uint64_t n;
  BenchmarkType type;

  app.add_option("-n", n, "Size of data");
  app.add_flag_callback("--sort", [&type] { type = Sort; });
  app.add_flag_callback("--vec", [&type] { type = Vector; });

  CLI11_PARSE(app, argc, argv);

  std::thread working_thread([n, type] {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "Start" << std::endl;
    auto elapsed = do_work(type, n);

    auto us =
        std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    std::cout << "Result: " << us / 1000.0 << " ms\n";
  });

  setup_affinity(working_thread, 1);

  working_thread.join();
}