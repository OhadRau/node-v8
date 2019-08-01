#ifndef WASM_NODE_API_NODE_API_TYPES_H_
#define WASM_NODE_API_NODE_API_TYPES_H_

#include "js_native_api_types.h"

#if NAPI_VERSION >= 4
typedef enum {
  napi_tsfn_release,
  napi_tsfn_abort
} napi_threadsafe_function_release_mode;

typedef enum {
  napi_tsfn_nonblocking,
  napi_tsfn_blocking
} napi_threadsafe_function_call_mode;
#endif  // NAPI_VERSION >= 4

typedef void (*napi_async_execute_callback)(opaque_ptr env,
                                            void* data);
typedef void (*napi_async_complete_callback)(opaque_ptr env,
                                             napi_status status,
                                             void* data);
#if NAPI_VERSION >= 4
typedef void (*napi_threadsafe_function_call_js)(opaque_ptr env,
                                                 opaque_ptr js_callback,
                                                 void* context,
                                                 void* data);
#endif  // NAPI_VERSION >= 4

typedef struct {
  uint32_t major;
  uint32_t minor;
  uint32_t patch;
  const char* release;
} napi_node_version;

#endif  // WASM_NODE_API_NODE_API_TYPES_H_
