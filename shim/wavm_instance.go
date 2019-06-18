package shim

import (
	"encoding/json"
	"errors"
	"sync"
	"syscall"

	"github.com/xuperchain/wavm/shim/api"
	"github.com/xuperchain/wavm/shim/inproc"
	"github.com/xuperchain/xuperunion/contract/bridge"
	"github.com/xuperchain/xuperunion/contract/bridge/memrpc"
	"github.com/xuperchain/xuperunion/contract/wasm/vm"
)

var (
	ErrModuleNotFound     = errors.New("smart contract not found")
	ErrInstantiationError = errors.New("failed to Instantiate module")
	ErrLoadError          = errors.New("failed to load module")
	ErrOutOfGas           = errors.New("out of gas")
)

// SafeRT is a thread-safe runtime
type SafeRT struct {
	caller api.ModulePtr
	mutex  sync.Mutex
}

// WavmCreator implements InstanceCreator
type WavmCreator struct {
	config    *vm.InstanceCreatorConfig
	moduleMap sync.Map
	resolver  inproc.PTR
}

// NewWavmCreator makes a new creator
func NewWavmCreator(c *vm.InstanceCreatorConfig) (vm.InstanceCreator, error) {
	ptr := inproc.Init()
	return &WavmCreator{config: c, resolver: ptr}, nil
}

// RemoveCache deletes a creator
func (w *WavmCreator) RemoveCache(name string) {
	if rt, ok := w.moduleMap.Load(name); ok {
		api.Release(rt.(*SafeRT).caller)
		w.moduleMap.Delete(name)
		inproc.Release(w.resolver)
	}
}

// CreateInstance makes a new instance
func (w *WavmCreator) CreateInstance(ctx *bridge.Context, provider vm.ContractCodeProvider) (vm.Instance, error) {
	name := ctx.ContractName
	if _, ok := w.moduleMap.Load(name); ok {
		return &wavmInstance{
			creator:   w,
			gasUsed:   0,
			ctx:       ctx,
			rpcServer: memrpc.NewServer(w.config.SyscallService),
		}, nil
	}
	//get module code
	code, err := provider.GetContractCode(name)
	if err != nil {
		return nil, err
	}

	//instantiate
	ptr := api.Load(code, len(code))
	if ptr == nil {
		return nil, ErrLoadError
	}
	if ec := api.Instantiate(ptr, name); ec != 0 {
		return nil, ErrInstantiationError
	}
	rt := &SafeRT{caller: ptr}
	w.moduleMap.Store(name, rt)
	return &wavmInstance{
		creator:   w,
		gasUsed:   0,
		ctx:       ctx,
		rpcServer: memrpc.NewServer(w.config.SyscallService),
	}, nil
}

// wavmInstance is a instance of runtime
type wavmInstance struct {
	creator   *WavmCreator
	rpcServer *memrpc.Server
	gasUsed   int64
	ctx       *bridge.Context
}

// Exec run the user contract
func (wi *wavmInstance) Exec(method string) error {
	var (
		rt *SafeRT
	)
	inproc.SetCtx(wi.creator.resolver, wi.ctx.ID, wi.rpcServer)
	//lazy load and code cache
	if rti, ok := wi.creator.moduleMap.Load(wi.ctx.ContractName); !ok {
		return ErrModuleNotFound
	} else {
		rt = rti.(*SafeRT)
	}

	rt.mutex.Lock()
	defer rt.mutex.Unlock()
	args := []string{}
	data, _ := json.Marshal(args)

	gasUsed, err := api.Call(rt.caller, wi.ctx.ContractName, method, []string{string(data)}, wi.ctx.GasLimit)
	wi.gasUsed = gasUsed
	if err != nil {
		switch err {
		case syscall.ERANGE:
			return ErrOutOfGas
		}
		return err
	}
	return nil
}

// GasUsed get gas used
func (wi *wavmInstance) GasUsed() int64 {
	return wi.gasUsed
}

// Release releases a session
func (wi *wavmInstance) Release() {
	inproc.RemoveSession(wi.ctx.ID)
}

func init() {
	vm.Register("wavm", NewWavmCreator)
}
