package api

//#cgo CFLAGS: -I${SRCDIR}/../../WAVM/Include
//#cgo LDFLAGS: -Wl,-rpath,"${SRCDIR}/../../WAVM/output/"
//#cgo LDFLAGS: -L"${SRCDIR}/../../WAVM/output/"  -lWAVM
//#include <stdlib.h>
//#include "WAVM/Emscripten/Interface.h"
import "C"
import (
	"errors"
	"unsafe"
)

// ModulePtr is a module instance
type ModulePtr C.ModulePtr

// Load loads a wasm binary to memory
func Load(codeBytes []byte, codeSize int) ModulePtr {
	cSize := C.int(codeSize)
	cBody := (*C.char)(unsafe.Pointer(&codeBytes[0]))
	return ModulePtr(C.loadBinaryModule(cBody, cSize))
}

// Release release a module instance
func Release(ptr ModulePtr) {
	C.release(C.ModulePtr(ptr))
}

// Instantiate instantiates a module instance
func Instantiate(moduleRef ModulePtr, debugName string) int {
	cDebugName := C.CString(debugName)
	defer C.free(unsafe.Pointer(cDebugName))
	return int(C.instantiate(C.ModulePtr(moduleRef), cDebugName))
}

// Call invokes a function
func Call(moduleRef ModulePtr, debugName string, functionName string, args []string, gaslimit int64) (int64, error) {
	cGasLimit := C.uint64_t(uint64(gaslimit))
	cDebugName := C.CString(debugName)
	defer C.free(unsafe.Pointer(cDebugName))
	cFunctionName := C.CString(functionName)
	defer C.free(unsafe.Pointer(cFunctionName))

	if gaslimit < 0 {
		return 0, errors.New("gas limit should be greater than 0")
	}
	// make sure result is allocated
	gasUsed, err := C.invokeFunction(C.ModulePtr(moduleRef), cDebugName, cFunctionName,
		cGasLimit)
	//get output
	return int64(gasUsed), err
}
