SYSROOT=$$HOME/Projects/wasi-sysroot/sysroot
TARGET=wasm32-wasi

COMMA=,

SOURCES=test_napi.c ../wasm_node_api/wasm_napi_lib/node_api_overrides.c

OBJECTS=$(SOURCES:.c=.o)

IMPORTS=$(shell cat import.syms)
EXPORTS=$(shell cat export.syms)

CFLAGS=-I../wasm_node_api/wasm_napi_lib --sysroot=$(SYSROOT) --target=$(TARGET)
IMPORTFLAGS=-allow-undefined-file import.syms
EXPORTFLAGS=$(addprefix --export=, $(EXPORTS)) --export-table
LDFLAGS=--entry=main $(IMPORTFLAGS) $(EXPORTFLAGS) -L$(SYSROOT)/lib/wasm32-wasi -lc

all: test_wasm_napi.node.wasm

clean:
	rm -f $(OBJECTS) test-napi.wasm

test_wasm_napi.wasm: $(OBJECTS)
	wasm-ld $(LDFLAGS) -o $@ $^

%.o: %.c
	clang $(CFLAGS) -o $@ -c $<
