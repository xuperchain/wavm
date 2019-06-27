package inproc

/*
#cgo CXXFLAGS: -I${SRCDIR}/../../WAVM/ThirdParty  -I${SRCDIR}/../../WAVM/Include
#cgo LDFLAGS: -ldl -lpthread -fPIC -O2 -L${SRCDIR}/../../WAVM/output/ThirdParty/bridge -lbridge  -L${SRCDIR}/../../WAVM/output/  -lWAVM
#include "c_resolver.h"
#include <stdlib.h>
*/
import "C"

import (
	"context"
	"fmt"

	"github.com/xuperchain/xuperunion/contract/bridge/memrpc"
)

// PTR is a new type
type PTR C.PTR

type session struct {
	rpcServer   *memrpc.Server
	responseBuf []byte
	ctxId       int64
}

var sessionBuf map[int64]*session = map[int64]*session{}

//export call_method
func call_method(cCtxid C.int64_t, p *C.char, size C.uint, p2 *C.char, size2 C.uint) C.uint {

	ctxid := int64(cCtxid)
	sess := sessionBuf[ctxid]
	if sess == nil {
		panic(fmt.Sprintf("can not find the context, id=%d", ctxid))
	}

	method := C.GoStringN(p, C.int(size))
	args := C.GoStringN(p2, C.int(size2))

	resBuf, err := sess.rpcServer.CallMethod(context.TODO(), ctxid, method, []byte(args))
	if err != nil {
		sess.responseBuf = make([]byte, len(err.Error()))
		copy(sess.responseBuf, []byte(err.Error()))
		return C.uint(len(err.Error()))
	} else {
		sess.responseBuf = make([]byte, len(resBuf))
		copy(sess.responseBuf, resBuf)
		return C.uint(len(resBuf))
	}
}

//export fetch_response
func fetch_response(cCtxid C.int64_t, size C.uint) *C.char {
	ctxid := int64(cCtxid)
	sess := sessionBuf[ctxid]
	if sess == nil {
		fmt.Printf("can not find the context, id=%d\n", ctxid)
		return C.CString(string(""))
	}
	if int(size) != len(sess.responseBuf) {
		return C.CString(string(""))
	}
	return C.CString(string(sess.responseBuf))
}

// Init: we can give a real handler to CallbackBase
func Init() PTR {
	return PTR(C.init())
}

// Release frees a resolver
func Release(p PTR) {
	C.release(C.PTR(p))
}

// RemoveSession releases a session
func RemoveSession(ctxid int64) {
	delete(sessionBuf, ctxid)
}

// SetCtx start a new context
func SetCtx(p PTR, _ctxid int64, rpc *memrpc.Server) {
	C.set_ctxid(C.PTR(p), C.int64_t(_ctxid))
	sessionBuf[_ctxid] = &session{ctxId: _ctxid, rpcServer: rpc}
}
