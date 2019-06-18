#pragma once

#include <string>
#include <map>

typedef uint32_t (*callback_call_method)(int64_t, const char*, uint32_t, const char*, uint32_t);
typedef char* (*callback_fetch_response)(int64_t, uint32_t);

namespace xchain {
class XChainServiceClient{
private:
    callback_call_method _ccm;
    callback_fetch_response _cfr;
    // _ctxid is static so the instance created in libWAVM.so and go_resolver can shared the same value, ensured by the lock of Call
    static int64_t _ctxid;
public:
    XChainServiceClient(callback_call_method, callback_fetch_response);
    ~XChainServiceClient();
    uint32_t call_method(const std::string& method, const std::string& args);
    uint32_t fetch_response(std::string& res, uint32_t res_size);
    void set_ctxid(int64_t);
};
}
