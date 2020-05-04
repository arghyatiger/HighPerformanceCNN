#pragma once

#include <cuda_runtime.h>
#include <thrust/device_vector.h>
#include <iostream>

#define BLOCK_SIZE 256
#define TILE_SIZE 16

#define CHECK_EQ(val1, val2, message)                              \
  do {                                                             \
    if (val1 != val2) {                                            \
      std::cerr << __FILE__ << "(" << __LINE__ << "): " << message \
                << std::endl;                                      \
      exit(1);                                                     \
    }                                                              \
  } while (0)


#define INIT_STORAGE(storage_ptr, shape)            \
  do {                                              \
    if (storage_ptr.get() == nullptr) {             \
      storage_ptr.reset(new Container(shape));        \
    } else if (storage_ptr->get_shape() != shape) { \
      storage_ptr->resize(shape);                   \
    }                                               \
  } while (0)

#define RAW_PTR(vector) thrust::raw_pointer_cast(vector.data())

#define CUDA_CHECK(condition)                                \
  do                                                         \
  {                                                          \
    cudaError_t error = condition;                           \
    CHECK_EQ(error, cudaSuccess, cudaGetErrorString(error)); \
  } while (0)

#define CUDA_POST_KERNEL_CHECK CUDA_CHECK(cudaPeekAtLastError())

inline __host__ __device__ void index2loc(int index, const int *shape, int dims,
                                          int *loc)
{
  for (int i = dims - 1; i >= 0; i--)
  {
    loc[i] = index % shape[i];
    index /= shape[i];
  }
}

inline __host__ __device__ int loc2index(const int *loc, const int *shape,
                                         int dims)
{
  int index = 0;
  int base = 1;
  for (int i = dims - 1; i >= 0; i--)
  {
    index += base * loc[i];
    base *= shape[i];
  }
  return index;
}
