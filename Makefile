-include .env
TARGET := wavm.so

VERSION := 1.0.0
BUILD := $(shell git rev-parse --short HEAD)

export XCHAIN_ROOT=$(shell pwd)/../xuperunion/output
LDFLAGS=-ldflags "-X=main.Version=$(VERSION) -X=main.Build=$(BUILD)"

all: runtime
.PHONY: all

WAVM/output/libWAVM.so: 
	(cd WAVM && mkdir -p output && cd output  && CXX=$(CXX) CC=$(CC) cmake -DLLVM_DIR=$(LLVMDIR) ../ && make -j4)

runtime: WAVM/output/libWAVM.so
	GOPROXY="https://athens.azurefd.net" go build $(LDFLAGS) -buildmode=plugin

clean:
	@rm -rf $(TARGET)
