#include "rpc_inproc.h"
#include <string.h>


namespace xchain{
XChainServiceClient::XChainServiceClient(callback_call_method ccm, callback_fetch_response cfr)
    :_ccm(ccm), _cfr(cfr) {
}

XChainServiceClient::~XChainServiceClient(){
}

int64_t XChainServiceClient::_ctxid = 0;

void XChainServiceClient::set_ctxid(int64_t ctxid) {
    _ctxid = ctxid;
}

uint32_t XChainServiceClient::call_method(const std::string& method, const std::string& args) {
    auto ret_code = _ccm(_ctxid, method.data(), method.length(),
                    args.data(), args.size());
    return ret_code;
}

uint32_t XChainServiceClient::fetch_response(std::string& buffer, uint32_t size) {
    char* res = _cfr(_ctxid, size);
    if (size != strlen(res)) {
        if (res != nullptr) free(res);
        return 1;
    }
    buffer.assign(res, size);
    if (res != nullptr) free(res);
    return 0;
}
}
