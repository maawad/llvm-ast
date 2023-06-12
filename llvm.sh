#!/bin/bash
library_path="/opt/rocm/llvm/lib/"
llvm_libraries=$(ls "${library_path}" | grep '\.a$')
cmake_libraries=$(echo "${llvm_libraries}" | sed 's/^lib//;s/\.a$//')

echo "${cmake_libraries}"
