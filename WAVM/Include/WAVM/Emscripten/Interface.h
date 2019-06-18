#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
    typedef void* ModulePtr;
    ModulePtr loadBinaryModule(const char* codeBytes, int codeSize);
    ModulePtr loadModule(char *filename);
    int instantiate(ModulePtr ptr,char* debugName);
    uint64_t invokeFunction(ModulePtr ptr,char* debugName,
		char *functionName, uint64_t gasLimit);
    void release(ModulePtr ptr);
#ifdef __cplusplus

}
#endif

