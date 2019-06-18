## Motivation

Metering the resoure cost of a wasm module instruction by instruction. The main idea is adding a metering function call
at the beginning of each [basic block](https://en.wikipedia.org/wiki/Basic_block) as below:

```
i64.const ${gas_counter}
call $add_gas_func
```

And add_gas_func accept one parameter gas_counter which is the sum of all the instruction's gas cost in currerent basic block, 

declared as below:
```
type (;1;) (func (param i32 i32))
(import "env" "_add_gas" (func (;0;) (type 1)))
```

Then, walk all the branch instructions ([block, if, else, loop, br, br_if, br_table, loop, return, end]) per defined funtion in wasm module,
and insert metering instructions behind.

## Implementation

Gas cost table refers to [ewasm project](https://github.com/ewasm/design/blob/master/determining_wasm_gas_costs.md), but swappable. Cost table support MVP only now.

The main process is like below:

```
insert add_gas function -> compile c/c++ to wasm -> insert metering instructions -> set gas limit -> run wasm module 
```

* insert add_gas function:  `add_gas` imported function should not be accessed by module developer, so to avoid adjust the index space of import section,
we insert the `add_gas` after the c/c++ source file dynamically like this:

```
#define IMPORT __attribute__((used)) __attribute__ ((visibility ("default")))
extern "C" void add_gas(uint64_t );
IMPORT void dummy_func() {
	add_gas(0);
}
```

`dummy_func` will never be called.
`used` attribute makes sure the compiler will not eliminate the unused function while optimizing. and we can mangle the name `add_gas` and `dummy_func` dynamically before developer call it.
if `add_gas` was mangled, make sure the instrisic function being changed correspondingly.

* compile c/c++ to wasm:  we can use [emscripten](https://emscripten.org/docs/introducing_emscripten/index.html) to compile the c/c++ source file to wasm format. 

For example:
```
emcc gas.c -Oz -s EXPORTED_FUNCTIONS='["_main","_add"]' -o gas.js
```
`Oz` option will do full optimization, so the memory section probably is omitted, so transfer wasm to wast and add a memory section.

* insert metering instructions : insert add_gas call after branch instruction. call GasVisitor::addGas. code is [here](https://github.com/duanbing/WAVM/blob/master/Programs/wavm-run/GasVisitContext.h)

* set gas limit: call Emscripten::setGasLimit before invokeFunction. 

* run the module:  

```
./bin/wavm-run -d ../Examples/gas.wast
```
* get gas used : call Emscripten::getGasUsed after invokeFunction.

## TODO

* I64 supporting in native wasm
* Gas cost for post-MVP
