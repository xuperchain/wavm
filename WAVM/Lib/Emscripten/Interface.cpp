#include "WAVM/Emscripten/Emscripten.h"
#include "WAVM/Inline/Assert.h"
#include "WAVM/Inline/BasicTypes.h"
#include "WAVM/Inline/HashMap.h"
#include "WAVM/Inline/Serialization.h"
#include "WAVM/Runtime/Intrinsics.h"
#include "WAVM/Runtime/Linker.h"
#include "WAVM/Runtime/Runtime.h"
#include "WAVM/Runtime/RuntimeData.h"
#include "WAVM/IR/Module.h"
#include "WAVM/WASM/WASM.h"
#include "WAVM/Inline/CLI.h"
#include "WAVM/Inline/Timing.h"
#include "WAVM/WASTParse/WASTParse.h"
#include "WAVM/WASTPrint/WASTPrint.h"
#include "WAVM/Inline/BasicTypes.h"

#include "WAVM/Emscripten/xchain-service.h"
#include "WAVM/Emscripten/gas-visit-context.h"
#include "WAVM/Emscripten/insert-imported-context.h"
#include "WAVM/Emscripten/Interface.h"

#include <iostream>
#include <errno.h>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <vector>

using namespace WAVM;
using namespace WAVM::IR;
using namespace WAVM::Runtime;
using namespace xchain;

static bool loadModule(const char* filename, IR::Module& outModule) {
    // Read the specified file into an array.
    std::vector<U8> fileBytes;
    if(!loadFile(filename, fileBytes)) {
        return false;
    }

    // If the file starts with the WASM binary magic number, load it as a binary irModule.
    static const U8 wasmMagicNumber[4] = {0x00, 0x61, 0x73, 0x6d};
    if(fileBytes.size() >= 4 && !memcmp(fileBytes.data(), wasmMagicNumber, 4)) {
        return WASM::loadBinaryModule(fileBytes.data(), fileBytes.size(), outModule);
    } else {
        // Make sure the WAST file is null terminated.
        fileBytes.push_back(0);

        // Load it as a text irModule.
        std::vector<WAST::Error> parseErrors;
        if(!WAST::parseModule(
                    (const char*)fileBytes.data(), fileBytes.size(), outModule, parseErrors)) {
            Log::printf(Log::error, "Error parsing WebAssembly text file:\n");
            WAST::reportParseErrors(filename, parseErrors);
            return false;
        }

        return true;
    }
}

bool _loadBinaryModule(const char* codeBytes, int codeSize, IR::Module& outModule) {
    std::vector<U8> fileBytes;
    fileBytes.resize(codeSize);
    fileBytes.assign(codeBytes, codeBytes + codeSize);

    static const U8 wasmMagicNumber[4] = {0x00, 0x61, 0x73, 0x6d};
    if(fileBytes.size() >= 4 && !memcmp(fileBytes.data(), wasmMagicNumber, 4)) {
        return WASM::loadBinaryModule(fileBytes.data(), fileBytes.size(), outModule);
    } else {
        // Make sure the WAST file is null terminated.
        fileBytes.push_back(0);

        // Load it as a text irModule.
        std::vector<WAST::Error> parseErrors;
        if(!WAST::parseModule(
                    (const char*)fileBytes.data(), fileBytes.size(), outModule, parseErrors)) {
            Log::printf(Log::error, "Error parsing WebAssembly text file:\n");
            WAST::reportParseErrors("codeBytes", parseErrors);
            return false;
        }

        return true;
    }
}

ModulePtr loadBinaryModule(const char* codeBytes, int codeSize) {
    IR::Module *module = new IR::Module;
    bool ok = _loadBinaryModule(codeBytes, codeSize, *module);
    if (!ok) {
        return nullptr;
    }

    std::string exportFuncName = "__builtin_add_gas";
    ImportFunctionInsertVisitor importFunctionInsertVisitor(*module, exportFuncName);
    importFunctionInsertVisitor.AddImportedFunc();


    bool found = false;
    Uptr add_gas_func_index = 0;
    for(add_gas_func_index = 0;
            add_gas_func_index < module->functions.imports.size();
            add_gas_func_index ++ ) {
        auto import_func = module->functions.imports[add_gas_func_index];
        if (import_func.exportName == exportFuncName &&
                import_func.moduleName == "env") {
            found = true;
            break;
        }
    }

    if (!found) {
        printf("can not find the add gas function\n");
        return nullptr;
    }

    for (auto& func_def : module->functions.defs)
    {
        GasVisitor gasVisitor(add_gas_func_index, *module, func_def);
        gasVisitor.AddGas();
    }

    return (void*)module;
}

ModulePtr loadModule(char *filename)
{
    IR::Module *module = new IR::Module;
    bool ok = loadModule(filename, *module);
    if (!ok) {
        return nullptr;
    }
    return (void*)module;
}

void release(ModulePtr ptr)
{
    IR::Module* irModule = (IR::Module*)ptr;

    /*
    XBridgeChainService* xchainServiceInstance = *xbridgeChainServiceMap.get(irModule);
    ModuleInstance* moduleInstance = xchainServiceInstance->instance;
    delete moduleInstance;
    Emscripten::Instance* emscriptenInstance = xchainServiceInstance->emscriptenInstance;
    delete emscriptenInstance;
    Compartment* compartment = xchainServiceInstance->compartment;
    delete compartment;
    xbridgeChainServiceMap.remove(irModule);
    */
    delete irModule;
}


struct RootResolver: Resolver
{
    Compartment* compartment;
    HashMap<std::string, ModuleInstance*> moduleNameToInstanceMap;
    RootResolver(Compartment* inCompartment): compartment(inCompartment){}
    bool resolve(const std::string& moduleName,
            const std::string& exportName,
            ExternType type, Object*& outObject) override
    {
        auto namedInstance = moduleNameToInstanceMap.get(moduleName);
        if(namedInstance)
        {
            outObject = getInstanceExport(*namedInstance, exportName);
            if(outObject)
            {
                if(isA(outObject, type)) { return true; }
                else
                {
                    Log::printf(Log::error,
                            "Resolved import %s.%s to a %s, but was expecting %s\n",
                            moduleName.c_str(),exportName.c_str(),
                            asString(getObjectType(outObject)).c_str(),
                            asString(type).c_str());
                    return false;
                }
            }
        }
        Log::printf(Log::error,
                "Generated stub for missing import %s.%s : %s\n",
                moduleName.c_str(),
                exportName.c_str(),
                asString(type).c_str());
        outObject = getStubObject(exportName, type);
        return true;
    }

    Object* getStubObject(const std::string& exportName,ExternType type) const
    {
        switch(type.kind)
        {
            case IR::ExternKind::function:
                {
                    Serialization::ArrayOutputStream codeStream;
                    OperatorEncoderStream encoder(codeStream);
                    encoder.unreachable();
                    encoder.end();
                    IR::Module stubModule;
                    DisassemblyNames stubModuleNames;
                    stubModule.types.push_back(asFunctionType(type));
                    stubModule.functions.defs.push_back({{0}, {}, std::move(codeStream.getBytes()), {}});
                    stubModule.exports.push_back({"importStub", IR::ExternKind::function, 0});
                    stubModuleNames.functions.push_back({"importStub: " + exportName, {}, {}});
                    IR::setDisassemblyNames(stubModule, stubModuleNames);
                    IR::validatePreCodeSections(stubModule);
                    IR::validatePostCodeSections(stubModule);

                    // Instantiate the module and return the stub function instance.
                    auto stub = compileModule(stubModule);
                    auto stubModuleInstance = instantiateModule(compartment, stub, {}, "importStub");
                    return getInstanceExport(stubModuleInstance, "importStub");
                }
            case IR::ExternKind::memory:
                {
                    return asObject(Runtime::createMemory(compartment, asMemoryType(type), std::string(exportName)));
                }
            case IR::ExternKind::table:
                {
                    return asObject(Runtime::createTable(compartment, asTableType(type), std::string(exportName)));
                }
            case IR::ExternKind::global:
                {
                    return asObject(Runtime::createGlobal(
                                compartment,
                                asGlobalType(type)));
                }
            case IR::ExternKind::exceptionType:
                {
                    return asObject(
                            Runtime::createExceptionType(compartment, asExceptionType(type), "importStub"));
                }
            default:  Errors::unreachable();
        };
    }
};


int instantiate(void* modulePtr, char* debugName) {
    IR::Module* irModule = (IR::Module*)modulePtr;
    Runtime::ModuleRef module = compileModule(*irModule);

    Compartment* compartment = Runtime::createCompartment();
    RootResolver rootResolver(compartment);
    Emscripten::Instance* emscriptenInstance = Emscripten::instantiate(compartment, *irModule);
    XBridgeChainService* xchainServiceInstance = new XBridgeChainService;
    xchainServiceInstance->compartment = compartment;
    xchainServiceInstance->emscriptenInstance = emscriptenInstance;

    rootResolver.moduleNameToInstanceMap.set("env", emscriptenInstance->env);
//    rootResolver.moduleNameToInstanceMap.set("asm2wasm", emscriptenInstance->asm2wasm);
    rootResolver.moduleNameToInstanceMap.set("global", emscriptenInstance->global);
    LinkResult linkResult = linkModule(*irModule, rootResolver);
    if (!linkResult.success)
    {
        Log::printf(Log::error,"link failed");
        return EXIT_FAILURE;
    }

    ModuleInstance* moduleInstance = instantiateModule(
            compartment, module, std::move(linkResult.resolvedImports), debugName);

    if (!moduleInstance) { return EXIT_FAILURE; }

    xchainServiceInstance->instance = moduleInstance;

    Emscripten::getMemoryImage(emscriptenInstance,
            xchainServiceInstance->emscriptenMemoryImage);

    xbridgeChainServiceMap.add(irModule, xchainServiceInstance);
    return EXIT_SUCCESS;
}

U64 invokeFunction(void* modulePtr,char* debugName, char *functionName, U64 gasLimit)
{
    Timing::Timer invokeFunctionTimer;
    IR::Module* irModule = (IR::Module*)modulePtr;
    wavmAssert(irModule != nullptr);
    XBridgeChainService* xchainServiceInstance = *xbridgeChainServiceMap.get(irModule);
    ModuleInstance* moduleInstance = xchainServiceInstance->instance;
    Compartment* compartment = xchainServiceInstance->compartment;
    Emscripten::Instance* emscriptenInstance = xchainServiceInstance->emscriptenInstance;
    wavmAssert(emscriptenInstance);
    Emscripten::resetMemories(emscriptenInstance, *irModule,
            xchainServiceInstance->emscriptenMemoryImage);

    if (!moduleInstance) { errno = EINVAL; return 0; }
    Context* context          = Runtime::createContext(compartment);
    WAVM::Emscripten::setGasLimit(emscriptenInstance, gasLimit);
    // Call the module start function, if it has one.
    Function* startFunction = getStartFunction(moduleInstance);
    if(startFunction) { invokeFunctionChecked(context, startFunction, {}); }

    std::vector<Value> invokeArgs;
    /*
    std::vector<const char*> argStrings;
    argStrings.push_back(debugName);
    for (int i = 0;i < argc; ++i) {
        argStrings.push_back(argv[i]);
    };
    */
    //屏蔽信号
    Runtime::catchRuntimeExceptions(
    [&] {
        Emscripten::initializeGlobals(emscriptenInstance, context, *irModule, moduleInstance);
        WAVM::Emscripten::setGasLimit(emscriptenInstance, gasLimit);
        Function* functionInstance
        = asFunctionNullable(getInstanceExport(moduleInstance, functionName));
        if (!functionInstance) {
            throwException(Runtime::ExceptionTypes::invalidArgument);
        }
//        Emscripten::injectCommandArgs(emscriptenInstance, argStrings, invokeArgs);
        IR::ValueTuple functionResults;
        functionResults = invokeFunctionChecked(context, functionInstance, invokeArgs);
        errno = 0;
        },
    [](Runtime::Exception* exception){
        if (Runtime::ExceptionTypes::outOfGas != exception->type) {
            fprintf(stderr,"caugth exception: %s\n", Runtime::describeException(exception).c_str());
            errno = EINVAL;
        } else {
            errno = ERANGE;
        }
        destroyException(exception);
    });

    //std::string res = asString(functionResults);
    Timing::logTimer("invoke function", invokeFunctionTimer);
    return WAVM::Emscripten::getGasUsed(emscriptenInstance);
}
