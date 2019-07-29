#ifndef WASM_NODE_API_NODE_API_H_
#define WASM_NODE_API_NODE_API_H_

#include "js_native_api.h"
#include "node_api_types.h"

struct uv_loop_s;  // Forward declaration.

#define NAPI_MODULE_EXPORT __attribute__((visibility("default")))

#ifdef __GNUC__
#define NAPI_NO_RETURN __attribute__((noreturn))
#else
#define NAPI_NO_RETURN
#endif

typedef opaque_ptr (*napi_addon_register_func)(opaque_ptr env,
                                               opaque_ptr exports);

typedef struct {
  int nm_version;
  unsigned int nm_flags;
  const char* nm_filename;
  napi_addon_register_func nm_register_func;
  const char* nm_modname;
  void* nm_priv;
  void* reserved[4];
} napi_module;

#define NAPI_MODULE_VERSION  1

#ifdef __cplusplus
#define EXTERN_C_START extern "C" {
#define EXTERN_C_END }
#else
#define EXTERN_C_START
#define EXTERN_C_END
#endif

EXTERN_C_START

NAPI_EXTERN void napi_module_register(napi_module* mod);

NAPI_EXTERN NAPI_NO_RETURN void napi_fatal_error(const char* location,
                                                 size_t location_len,
                                                 const char* message,
                                                 size_t message_len);

// Methods for custom handling of async operations
NAPI_EXTERN napi_status napi_async_init(opaque_ptr env,
                                        opaque_ptr async_resource,
                                        opaque_ptr async_resource_name,
                                        opaque_ptr* result);

NAPI_EXTERN napi_status napi_async_destroy(opaque_ptr env,
                                           opaque_ptr async_context);

NAPI_EXTERN napi_status napi_make_callback(opaque_ptr env,
                                           opaque_ptr async_context,
                                           opaque_ptr recv,
                                           opaque_ptr func,
                                           size_t argc,
                                           opaque_ptr* argv,
                                           opaque_ptr* result);

// Methods to provide node::Buffer functionality with napi types
NAPI_EXTERN napi_status napi_create_buffer(opaque_ptr env,
                                           size_t length,
                                           void** data,
                                           opaque_ptr* result);
NAPI_EXTERN napi_status napi_create_external_buffer(opaque_ptr env,
                                                    size_t length,
                                                    void* data,
                                                    napi_finalize finalize_cb,
                                                    void* finalize_hint,
                                                    opaque_ptr* result);
NAPI_EXTERN napi_status napi_create_buffer_copy(opaque_ptr env,
                                                size_t length,
                                                const void* data,
                                                void** result_data,
                                                opaque_ptr* result);
NAPI_EXTERN napi_status napi_is_buffer(opaque_ptr env,
                                       opaque_ptr value,
                                       bool* result);
NAPI_EXTERN napi_status napi_get_buffer_info(opaque_ptr env,
                                             opaque_ptr value,
                                             void** data,
                                             size_t* length);

// Methods to manage simple async operations
NAPI_EXTERN
napi_status napi_create_async_work(opaque_ptr env,
                                   opaque_ptr async_resource,
                                   opaque_ptr async_resource_name,
                                   napi_async_execute_callback execute,
                                   napi_async_complete_callback complete,
                                   void* data,
                                   opaque_ptr* result);
NAPI_EXTERN napi_status napi_delete_async_work(opaque_ptr env,
                                               opaque_ptr work);
NAPI_EXTERN napi_status napi_queue_async_work(opaque_ptr env,
                                              opaque_ptr work);
NAPI_EXTERN napi_status napi_cancel_async_work(opaque_ptr env,
                                               opaque_ptr work);

// version management
NAPI_EXTERN
napi_status napi_get_node_version(opaque_ptr env,
                                  const napi_node_version** version);

#if NAPI_VERSION >= 2

// Return the current libuv event loop for a given environment
NAPI_EXTERN napi_status napi_get_uv_event_loop(opaque_ptr env,
                                               opaque_ptr** loop);

#endif  // NAPI_VERSION >= 2

#if NAPI_VERSION >= 3

NAPI_EXTERN napi_status napi_fatal_exception(opaque_ptr env, opaque_ptr err);

NAPI_EXTERN napi_status napi_add_env_cleanup_hook(opaque_ptr env,
                                                  void (*fun)(void* arg),
                                                  void* arg);

NAPI_EXTERN napi_status napi_remove_env_cleanup_hook(opaque_ptr env,
                                                     void (*fun)(void* arg),
                                                     void* arg);

NAPI_EXTERN napi_status napi_open_callback_scope(opaque_ptr env,
                                                 opaque_ptr resource_object,
                                                 opaque_ptr context,
                                                 opaque_ptr* result);

NAPI_EXTERN napi_status napi_close_callback_scope(opaque_ptr env,
                                                  opaque_ptr scope);

#endif  // NAPI_VERSION >= 3

#if NAPI_VERSION >= 4

// Calling into JS from other threads
NAPI_EXTERN napi_status
napi_create_threadsafe_function(opaque_ptr env,
                                opaque_ptr func,
                                opaque_ptr async_resource,
                                opaque_ptr async_resource_name,
                                size_t max_queue_size,
                                size_t initial_thread_count,
                                void* thread_finalize_data,
                                napi_finalize thread_finalize_cb,
                                void* context,
                                napi_threadsafe_function_call_js call_js_cb,
                                opaque_ptr* result);

NAPI_EXTERN napi_status
napi_get_threadsafe_function_context(opaque_ptr func,
                                     void** result);

NAPI_EXTERN napi_status
napi_call_threadsafe_function(opaque_ptr func,
                              void* data,
                              napi_threadsafe_function_call_mode is_blocking);

NAPI_EXTERN napi_status
napi_acquire_threadsafe_function(opaque_ptr func);

NAPI_EXTERN napi_status
napi_release_threadsafe_function(opaque_ptr func,
                                 napi_threadsafe_function_release_mode mode);

NAPI_EXTERN napi_status
napi_unref_threadsafe_function(opaque_ptr env, opaque_ptr func);

NAPI_EXTERN napi_status
napi_ref_threadsafe_function(opaque_ptr env, opaque_ptr func);

EXTERN_C_END

#endif  // NAPI_VERSION >= 4

#endif  // WASM_NODE_API_NODE_API_H_
