# LLVM Project for [ViDeZZo](https://github.com/HexHive/ViDeZZo)

## Quick start

Follow the instructions below to compile it.

```
git clone https://github.com/cyruscyliu/videzzo-llvm-project.git llvm-project --depth=1
pushd llvm-project && mkdir build-custom && pushd build-custom
cmake -G Ninja -DLLVM_ENABLE_PROJECTS="clang;compiler-rt;lld" \
    -DLLVM_TARGETS_TO_BUILD=X86 -DLLVM_OPTIMIZED_TABLEGEN=ON ../llvm/
ninja clang compiler-rt llvm-symbolizer llvm-profdata llvm-cov \
    llvm-config lld llvm-dis opt
popd && popd
```

The toolchain will be available in `llvm-project/build-custom/bin`.
