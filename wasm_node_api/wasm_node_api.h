#ifndef WASM_NODE_API_H_
#define WASM_NODE_API_H_

#include "v8.h"

namespace node {
namespace wasm_napi {

void RegisterNapiBuiltins(v8::Isolate* isolate);

}  // namespace wasm_napi
}  // namespace node

#endif  // WASM_NODE_API_H