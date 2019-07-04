#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "c_resolver.h"

#include "bridge/rpc_inproc.h"
#include "WAVM/Emscripten/xchain-service-client.h"

extern "C" uint32_t CallMethod(int64_t, const char*, uint32_t, const char*, uint32_t);
extern "C" char* FetchResponse(int64_t, uint32_t);

using namespace xchain;

PTR init() {
    XChainServiceClient* p = new XChainServiceClient(CallMethod, FetchResponse);
    xchain::init_client(p);
    return (PTR)p;
}

void release(PTR ptr) {
    delete (XChainServiceClient*)ptr;
}

void set_ctxid(PTR ptr, int64_t id) {
    XChainServiceClient *p = (XChainServiceClient*) ptr;
    p->set_ctxid(id);
}
