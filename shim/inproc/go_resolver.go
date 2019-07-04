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

type responseDesc struct {
	Body  []byte
	Error bool
}

type session struct {
	rpcServer   *memrpc.Server
	responseBuf responseDesc
	ctxId       int64
}

var sessionBuf map[int64]*session = map[int64]*session{}

//export CallMethod
func CallMethod(cCtxid C.int64_t, p *C.char, size C.uint, p2 *C.char, size2 C.uint) C.uint {

	ctxid := int64(cCtxid)
	sess := sessionBuf[ctxid]
	if sess == nil {
		panic(fmt.Sprintf("can not find the context, id=%d", ctxid))
	}

	method := C.GoStringN(p, C.int(size))
	args := C.GoStringN(p2, C.int(size2))

	resBuf, err := sess.rpcServer.CallMethod(context.TODO(), ctxid, method, []byte(args))

	var respDesc responseDesc
	if err != nil {
		respDesc.Error = true
		respDesc.Body = make([]byte, len(err.Error()))
		copy(respDesc.Body, []byte(err.Error()))
	} else {
		if len(resBuf) > 0 {
			respDesc.Body = make([]byte, len(resBuf))
			copy(respDesc.Body, resBuf)
		}
	}
	sess.responseBuf = respDesc
	sessionBuf[ctxid] = sess
	return C.uint(len(sess.responseBuf.Body))
}

//export FetchResponse
func FetchResponse(cCtxid C.int64_t, size C.uint) *C.char {
	ctxid := int64(cCtxid)
	sess := sessionBuf[ctxid]
	if sess == nil {
		panic(fmt.Sprintf("calling can not find the context, id=%d\n", ctxid))
	}

	if int(size) != len(sess.responseBuf.Body)+1 {
		panic(fmt.Sprintf("calling can not find the context, id=%d\n", ctxid))
	}

	flag := "1"
	if sess.responseBuf.Error {
		flag = "0"
	}
	return C.CString(flag + string(sess.responseBuf.Body))
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
