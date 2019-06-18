# WAVM for XuperChain

This is a XuperChain WASM Virtual Machine based on [WAVM/WAVM](https://github.com/WAVM/WAVM), most of the changes compared 
to the original repo are that we add gas metering to the wasm code and provide a golang wrapper. 
All the changes are not invasive to the original WAVM.
  
C/C++ code compiled with emscripten can run on WAVM without any other resolvers. For other languages it doesn't support yet.

## Quick start

Requirements:

* GCC 5.x
* LLVM 8.0.0 (recommended)
* XuperUnion v3.1

  
### Standalone 
  If you try to test the new fetures like gas metering, just do as the original repository tells you : [Building WAVM](./WAVM/README.md)

  we also provide a more unix-like way to build:
  
1. install llvm and clang by [here](https://clang.llvm.org/get_started.html), assume your installation directory is ${llvm_install_dir}
2. build

```
cd ${source-to-wavm}
CXX=${llvm_install_dir}/bin/clang++ CC=${llvm_install_dir}/bin/clang LLVMDIR=${llvm_install_dir}/lib/cmake/llvm make
```

Then run this command, you will get the gas used as the debug output.
```
./output/wavm-run -d ../Example/hello.wast
```

### XuperChain
   
XuperChain is pluggable cause almost all the component included is pluggable, so is smart contract virtual machine. There
are three steps to make it work.

1. Compile WAVM. 
```
cd ${source-to-wavm}
CXX=${llvm_install_dir}/bin/clang++ CC=${llvm_install_dir}/bin/clang LLVMDIR=${llvm_install_dir}/lib/cmake/llvm make
```
2. cd ${source-to-wavm}, run `make` to build the plugin, copy it to the plugins directory, then add or update `plugins.conf` with 
```
   "wasm":[{
   "subtype":"wavm",
   "path": "plugins/vm/wavm.so",
   "version": "1.0.0",
   "ondemand": false
 }]
```

3. Add the configure to xchain.yaml:
```
wasm:
  driver: wavm
  external: true
```

`external` means it should be loaded as a plugin.
Be careful that `shim` depends on `xuperunion`(which should be compiled by enabling GO111MODULE=on without -mod=vendor, 
remove the `export GOFLAGS=-mod=vendor` from xuperchain's Makefile),
so make sure that `shim` requires the same version of `xuperunion` who load this plugin, 

or an error as below maybe occur:
```
Warn: plugin open failed!" module=pluginmgr pluginname=wasm 
err="plugin.Open(\"plugins/wavm\"): plugin was built with a different version of package github.com/xuperchain/xuperunion/..."
```


## FAQ

1. libWAVM.so: cannot open shared object file: No such file or directory"

libWAVM.so is in the ${source-to-wavm}/WAVM/output by default. so if you intend to deploy wavm.so to other server, you can copy the
libWAVM.so to any directory and set environment `LD_LIBRARY_PATH` to this directory.

