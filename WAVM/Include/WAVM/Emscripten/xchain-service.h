#pragma once

#include "WAVM/Inline/HashMap.h"
#include "WAVM/Emscripten/Emscripten.h"

#include "WAVM/Emscripten/xchain-service-client.h"

namespace WAVM { namespace Runtime {
    struct Compartment;
    struct ModuleInstance;
}}

using namespace WAVM;
using namespace WAVM::Runtime;

namespace xchain {
    struct XBridgeChainService {
        Emscripten::Instance* emscriptenInstance;
        Compartment* compartment;
        ModuleInstance* instance;
        std::vector<U8> emscriptenMemoryImage;
    };

    static WAVM::HashMap<IR::Module*, XBridgeChainService*> xbridgeChainServiceMap;
}
