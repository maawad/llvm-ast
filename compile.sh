export PATH=/opt/rocm/llvm/bin/:$PATH
cmake -DCMAKE_BUILD_TYPE=Release -B build/
cmake --build ./build/
