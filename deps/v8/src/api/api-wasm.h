// Copyright 2019 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_API_API_WASM_H_
#define V8_API_API_WASM_H_

#include "include/v8-wasm.h"
#include "src/objects/fixed-array.h"
#include "src/wasm/value-type.h"
#include "src/wasm/wasm-objects.h"

namespace v8 {
namespace wasm {

struct FuncData {
  v8::internal::Isolate* isolate;
  v8::internal::PodArray<v8::internal::wasm::ValueType> serialized_sig;
  int param_count, result_count;
  Func::callbackType callback;

  FuncData(v8::internal::Isolate* isolate, const FuncType type);

  static v8::internal::Address v8_callback(
    void* data, size_t memoryPages, uint8_t* memoryBase,
    i::WasmTableObject table, i::Address argv);
};

}  // namespace wasm
}  // namespace v8

#endif  // V8_API_API_WASM_H_
