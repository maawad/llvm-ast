#include <hip/hip_runtime.h>

__device__ int addNumbers(int num1, int num2) {
  auto sum = num1 + num2;
  return sum;
}

__global__ void kernel() {
  int x = blockIdx.x;
  int y = blockDim.x;

  x = __shfl_up(x, 0);
  auto res = addNumbers(x, y);
}
int main() {
  kernel<<<1, 1>>>();
  return 0;
}
