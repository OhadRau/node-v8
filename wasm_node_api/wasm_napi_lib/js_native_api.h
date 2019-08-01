#ifndef WASM_NODE_API_JS_NATIVE_API_H_
#define WASM_NODE_API_JS_NATIVE_API_H_

// This file needs to be compatible with C compilers.
#include <stddef.h>   // NOLINT(modernize-deprecated-headers)
#include <stdbool.h>  // NOLINT(modernize-deprecated-headers)
#include "js_native_api_types.h"

// Use INT_MAX, this should only be consumed by the pre-processor anyway.
#define NAPI_VERSION_EXPERIMENTAL 2147483647
#ifndef NAPI_VERSION
#ifdef NAPI_EXPERIMENTAL
#define NAPI_VERSION NAPI_VERSION_EXPERIMENTAL
#else
// The baseline version for N-API
#define NAPI_VERSION 4
#endif
#endif

// If you need __declspec(dllimport), either include <node_api.h> instead, or
// define NAPI_EXTERN as __declspec(dllimport) on the compiler's command line.
#ifndef NAPI_EXTERN
  #define NAPI_EXTERN __attribute__((visibility("default"), __import_module__("napi_unstable")))
#endif

#define NAPI_AUTO_LENGTH SIZE_MAX

#ifdef __cplusplus
#define EXTERN_C_START extern "C" {
#define EXTERN_C_END }
#else
#define EXTERN_C_START
#define EXTERN_C_END
#endif

EXTERN_C_START

NAPI_EXTERN napi_status
napi_get_last_error_info(opaque_ptr env,
                         const napi_extended_error_info** result);

// Getters for defined singletons
NAPI_EXTERN napi_status napi_get_undefined(opaque_ptr env, opaque_ptr* result);
NAPI_EXTERN napi_status napi_get_null(opaque_ptr env, opaque_ptr* result);
NAPI_EXTERN napi_status napi_get_global(opaque_ptr env, opaque_ptr* result);
NAPI_EXTERN napi_status napi_get_boolean(opaque_ptr env,
                                         bool value,
                                         opaque_ptr* result);

// Methods to create Primitive types/Objects
NAPI_EXTERN napi_status napi_create_object(opaque_ptr env, opaque_ptr* result);
NAPI_EXTERN napi_status napi_create_array(opaque_ptr env, opaque_ptr* result);
NAPI_EXTERN napi_status napi_create_array_with_length(opaque_ptr env,
                                                      size_t length,
                                                      opaque_ptr* result);
NAPI_EXTERN napi_status napi_create_double(opaque_ptr env,
                                           double value,
                                           opaque_ptr* result);
NAPI_EXTERN napi_status napi_create_int32(opaque_ptr env,
                                          int32_t value,
                                          opaque_ptr* result);
NAPI_EXTERN napi_status napi_create_uint32(opaque_ptr env,
                                           uint32_t value,
                                           opaque_ptr* result);
NAPI_EXTERN napi_status napi_create_int64(opaque_ptr env,
                                          int64_t value,
                                          opaque_ptr* result);
NAPI_EXTERN napi_status napi_create_string_latin1(opaque_ptr env,
                                                  const char* str,
                                                  size_t length,
                                                  opaque_ptr* result);
NAPI_EXTERN napi_status napi_create_string_utf8(opaque_ptr env,
                                                const char* str,
                                                size_t length,
                                                opaque_ptr* result);
NAPI_EXTERN napi_status napi_create_string_utf16(opaque_ptr env,
                                                 const char16_t* str,
                                                 size_t length,
                                                 opaque_ptr* result);
NAPI_EXTERN napi_status napi_create_symbol(opaque_ptr env,
                                           opaque_ptr description,
                                           opaque_ptr* result);
NAPI_EXTERN napi_status napi_create_function(opaque_ptr env,
                                             const char* utf8name,
                                             size_t length,
                                             napi_callback cb,
                                             void* data,
                                             opaque_ptr* result);
NAPI_EXTERN napi_status napi_create_error(opaque_ptr env,
                                          opaque_ptr code,
                                          opaque_ptr msg,
                                          opaque_ptr* result);
NAPI_EXTERN napi_status napi_create_type_error(opaque_ptr env,
                                               opaque_ptr code,
                                               opaque_ptr msg,
                                               opaque_ptr* result);
NAPI_EXTERN napi_status napi_create_range_error(opaque_ptr env,
                                                opaque_ptr code,
                                                opaque_ptr msg,
                                                opaque_ptr* result);

// Methods to get the native napi_value from Primitive type
NAPI_EXTERN napi_status napi_typeof(opaque_ptr env,
                                    opaque_ptr value,
                                    napi_valuetype* result);
NAPI_EXTERN napi_status napi_get_value_double(opaque_ptr env,
                                              opaque_ptr value,
                                              double* result);
NAPI_EXTERN napi_status napi_get_value_int32(opaque_ptr env,
                                             opaque_ptr value,
                                             int32_t* result);
NAPI_EXTERN napi_status napi_get_value_uint32(opaque_ptr env,
                                              opaque_ptr value,
                                              uint32_t* result);
NAPI_EXTERN napi_status napi_get_value_int64(opaque_ptr env,
                                             opaque_ptr value,
                                             int64_t* result);
NAPI_EXTERN napi_status napi_get_value_bool(opaque_ptr env,
                                            opaque_ptr value,
                                            bool* result);

// Copies LATIN-1 encoded bytes from a string into a buffer.
NAPI_EXTERN napi_status napi_get_value_string_latin1(opaque_ptr env,
                                                     opaque_ptr value,
                                                     char* buf,
                                                     size_t bufsize,
                                                     size_t* result);

// Copies UTF-8 encoded bytes from a string into a buffer.
NAPI_EXTERN napi_status napi_get_value_string_utf8(opaque_ptr env,
                                                   opaque_ptr value,
                                                   char* buf,
                                                   size_t bufsize,
                                                   size_t* result);

// Copies UTF-16 encoded bytes from a string into a buffer.
NAPI_EXTERN napi_status napi_get_value_string_utf16(opaque_ptr env,
                                                    opaque_ptr value,
                                                    char16_t* buf,
                                                    size_t bufsize,
                                                    size_t* result);

// Methods to coerce values
// These APIs may execute user scripts
NAPI_EXTERN napi_status napi_coerce_to_bool(opaque_ptr env,
                                            opaque_ptr value,
                                            opaque_ptr* result);
NAPI_EXTERN napi_status napi_coerce_to_number(opaque_ptr env,
                                              opaque_ptr value,
                                              opaque_ptr* result);
NAPI_EXTERN napi_status napi_coerce_to_object(opaque_ptr env,
                                              opaque_ptr value,
                                              opaque_ptr* result);
NAPI_EXTERN napi_status napi_coerce_to_string(opaque_ptr env,
                                              opaque_ptr value,
                                              opaque_ptr* result);

// Methods to work with Objects
NAPI_EXTERN napi_status napi_get_prototype(opaque_ptr env,
                                           opaque_ptr object,
                                           opaque_ptr* result);
NAPI_EXTERN napi_status napi_get_property_names(opaque_ptr env,
                                                opaque_ptr object,
                                                opaque_ptr* result);
NAPI_EXTERN napi_status napi_set_property(opaque_ptr env,
                                          opaque_ptr object,
                                          opaque_ptr key,
                                          opaque_ptr value);
NAPI_EXTERN napi_status napi_has_property(opaque_ptr env,
                                          opaque_ptr object,
                                          opaque_ptr key,
                                          bool* result);
NAPI_EXTERN napi_status napi_get_property(opaque_ptr env,
                                          opaque_ptr object,
                                          opaque_ptr key,
                                          opaque_ptr* result);
NAPI_EXTERN napi_status napi_delete_property(opaque_ptr env,
                                             opaque_ptr object,
                                             opaque_ptr key,
                                             bool* result);
NAPI_EXTERN napi_status napi_has_own_property(opaque_ptr env,
                                              opaque_ptr object,
                                              opaque_ptr key,
                                              bool* result);
NAPI_EXTERN napi_status napi_set_named_property(opaque_ptr env,
                                                opaque_ptr object,
                                                const char* utf8name,
                                                opaque_ptr value);
NAPI_EXTERN napi_status napi_has_named_property(opaque_ptr env,
                                                opaque_ptr object,
                                                const char* utf8name,
                                                bool* result);
NAPI_EXTERN napi_status napi_get_named_property(opaque_ptr env,
                                                opaque_ptr object,
                                                const char* utf8name,
                                                opaque_ptr* result);
NAPI_EXTERN napi_status napi_set_element(opaque_ptr env,
                                         opaque_ptr object,
                                         uint32_t index,
                                         opaque_ptr value);
NAPI_EXTERN napi_status napi_has_element(opaque_ptr env,
                                         opaque_ptr object,
                                         uint32_t index,
                                         bool* result);
NAPI_EXTERN napi_status napi_get_element(opaque_ptr env,
                                         opaque_ptr object,
                                         uint32_t index,
                                         opaque_ptr* result);
NAPI_EXTERN napi_status napi_delete_element(opaque_ptr env,
                                            opaque_ptr object,
                                            uint32_t index,
                                            bool* result);
NAPI_EXTERN napi_status
napi_define_properties(opaque_ptr env,
                       opaque_ptr object,
                       size_t property_count,
                       const napi_property_descriptor* properties);

// Methods to work with Arrays
NAPI_EXTERN napi_status napi_is_array(opaque_ptr env,
                                      opaque_ptr value,
                                      bool* result);
NAPI_EXTERN napi_status napi_get_array_length(opaque_ptr env,
                                              opaque_ptr value,
                                              uint32_t* result);

// Methods to compare values
NAPI_EXTERN napi_status napi_strict_equals(opaque_ptr env,
                                           opaque_ptr lhs,
                                           opaque_ptr rhs,
                                           bool* result);

// Methods to work with Functions
NAPI_EXTERN napi_status napi_call_function(opaque_ptr env,
                                           opaque_ptr recv,
                                           opaque_ptr func,
                                           size_t argc,
                                           const opaque_ptr* argv,
                                           opaque_ptr* result);
NAPI_EXTERN napi_status napi_new_instance(opaque_ptr env,
                                          opaque_ptr constructor,
                                          opaque_ptr argc,
                                          const opaque_ptr* argv,
                                          opaque_ptr* result);
NAPI_EXTERN napi_status napi_instanceof(opaque_ptr env,
                                        opaque_ptr object,
                                        opaque_ptr constructor,
                                        bool* result);

// Methods to work with napi_callbacks

// Gets all callback info in a single call. (Ugly, but faster.)
NAPI_EXTERN napi_status napi_get_cb_info(
    opaque_ptr env,               // [in] NAPI environment handle
    opaque_ptr cbinfo,  // [in] Opaque callback-info handle
    size_t* argc,      // [in-out] Specifies the size of the provided argv array
                       // and receives the actual count of args.
    opaque_ptr* argv,  // [out] Array of values
    opaque_ptr* this_arg,  // [out] Receives the JS 'this' arg for the call
    void** data);          // [out] Receives the data pointer for the callback.

NAPI_EXTERN napi_status napi_get_new_target(opaque_ptr env,
                                            opaque_ptr cbinfo,
                                            opaque_ptr* result);
NAPI_EXTERN napi_status
napi_define_class(opaque_ptr env,
                  const char* utf8name,
                  size_t length,
                  napi_callback constructor,
                  void* data,
                  size_t property_count,
                  const napi_property_descriptor* properties,
                  opaque_ptr* result);

// Methods to work with external data objects
NAPI_EXTERN napi_status napi_wrap(opaque_ptr env,
                                  opaque_ptr js_object,
                                  void* native_object,
                                  opaque_ptr finalize_cb,
                                  void* finalize_hint,
                                  opaque_ptr* result);
NAPI_EXTERN napi_status napi_unwrap(opaque_ptr env,
                                    opaque_ptr js_object,
                                    void** result);
NAPI_EXTERN napi_status napi_remove_wrap(opaque_ptr env,
                                         opaque_ptr js_object,
                                         void** result);
NAPI_EXTERN napi_status napi_create_external(opaque_ptr env,
                                             void* data,
                                             napi_finalize finalize_cb,
                                             void* finalize_hint,
                                             opaque_ptr* result);
NAPI_EXTERN napi_status napi_get_value_external(opaque_ptr env,
                                                opaque_ptr value,
                                                void** result);

// Methods to control object lifespan

// Set initial_refcount to 0 for a weak reference, >0 for a strong reference.
NAPI_EXTERN napi_status napi_create_reference(opaque_ptr env,
                                              opaque_ptr value,
                                              uint32_t initial_refcount,
                                              opaque_ptr* result);

// Deletes a reference. The referenced value is released, and may
// be GC'd unless there are other references to it.
NAPI_EXTERN napi_status napi_delete_reference(opaque_ptr env, opaque_ptr ref);

// Increments the reference count, optionally returning the resulting count.
// After this call the  reference will be a strong reference because its
// refcount is >0, and the referenced object is effectively "pinned".
// Calling this when the refcount is 0 and the object is unavailable
// results in an error.
NAPI_EXTERN napi_status napi_reference_ref(opaque_ptr env,
                                           opaque_ptr ref,
                                           uint32_t* result);

// Decrements the reference count, optionally returning the resulting count.
// If the result is 0 the reference is now weak and the object may be GC'd
// at any time if there are no other references. Calling this when the
// refcount is already 0 results in an error.
NAPI_EXTERN napi_status napi_reference_unref(opaque_ptr env,
                                             opaque_ptr ref,
                                             uint32_t* result);

// Attempts to get a referenced value. If the reference is weak,
// the value might no longer be available, in that case the call
// is still successful but the result is NULL.
NAPI_EXTERN napi_status napi_get_reference_value(opaque_ptr env,
                                                 opaque_ptr ref,
                                                 opaque_ptr* result);

NAPI_EXTERN napi_status napi_open_handle_scope(opaque_ptr env,
                                               opaque_ptr* result);
NAPI_EXTERN napi_status napi_close_handle_scope(opaque_ptr env,
                                                opaque_ptr scope);
NAPI_EXTERN napi_status
napi_open_escapable_handle_scope(opaque_ptr env,
                                 opaque_ptr* result);
NAPI_EXTERN napi_status
napi_close_escapable_handle_scope(opaque_ptr env,
                                  opaque_ptr scope);

NAPI_EXTERN napi_status napi_escape_handle(opaque_ptr env,
                                           opaque_ptr scope,
                                           opaque_ptr escapee,
                                           opaque_ptr* result);

// Methods to support error handling
NAPI_EXTERN napi_status napi_throw(opaque_ptr env, opaque_ptr error);
NAPI_EXTERN napi_status napi_throw_error(opaque_ptr env,
                                         const char* code,
                                         const char* msg);
NAPI_EXTERN napi_status napi_throw_type_error(opaque_ptr env,
                                         const char* code,
                                         const char* msg);
NAPI_EXTERN napi_status napi_throw_range_error(opaque_ptr env,
                                         const char* code,
                                         const char* msg);
NAPI_EXTERN napi_status napi_is_error(opaque_ptr env,
                                      opaque_ptr value,
                                      bool* result);

// Methods to support catching exceptions
NAPI_EXTERN napi_status napi_is_exception_pending(opaque_ptr env, bool* result);
NAPI_EXTERN napi_status napi_get_and_clear_last_exception(opaque_ptr env,
                                                          opaque_ptr* result);

// Methods to work with array buffers and typed arrays
NAPI_EXTERN napi_status napi_is_arraybuffer(opaque_ptr env,
                                            opaque_ptr value,
                                            bool* result);
NAPI_EXTERN napi_status napi_create_arraybuffer(opaque_ptr env,
                                                size_t byte_length,
                                                void** data,
                                                opaque_ptr* result);
NAPI_EXTERN napi_status
napi_create_external_arraybuffer(opaque_ptr env,
                                 void* external_data,
                                 size_t byte_length,
                                 napi_finalize finalize_cb,
                                 void* finalize_hint,
                                 opaque_ptr* result);
NAPI_EXTERN napi_status napi_get_arraybuffer_info(opaque_ptr env,
                                                  opaque_ptr arraybuffer,
                                                  void** data,
                                                  size_t* byte_length);
NAPI_EXTERN napi_status napi_is_typedarray(opaque_ptr env,
                                           opaque_ptr value,
                                           bool* result);
NAPI_EXTERN napi_status napi_create_typedarray(opaque_ptr env,
                                               napi_typedarray_type type,
                                               size_t length,
                                               opaque_ptr arraybuffer,
                                               size_t byte_offset,
                                               opaque_ptr* result);
NAPI_EXTERN napi_status napi_get_typedarray_info(opaque_ptr env,
                                                 opaque_ptr typedarray,
                                                 napi_typedarray_type* type,
                                                 size_t* length,
                                                 void** data,
                                                 opaque_ptr* arraybuffer,
                                                 size_t* byte_offset);

NAPI_EXTERN napi_status napi_create_dataview(opaque_ptr env,
                                             size_t length,
                                             opaque_ptr arraybuffer,
                                             size_t byte_offset,
                                             opaque_ptr* result);
NAPI_EXTERN napi_status napi_is_dataview(opaque_ptr env,
                                         opaque_ptr value,
                                         bool* result);
NAPI_EXTERN napi_status napi_get_dataview_info(opaque_ptr env,
                                               opaque_ptr dataview,
                                               size_t* bytelength,
                                               void** data,
                                               opaque_ptr* arraybuffer,
                                               size_t* byte_offset);

// version management
NAPI_EXTERN napi_status napi_get_version(opaque_ptr env, uint32_t* result);

// Promises
NAPI_EXTERN napi_status napi_create_promise(opaque_ptr env,
                                            opaque_ptr* deferred,
                                            opaque_ptr* promise);
NAPI_EXTERN napi_status napi_resolve_deferred(opaque_ptr env,
                                              opaque_ptr deferred,
                                              opaque_ptr resolution);
NAPI_EXTERN napi_status napi_reject_deferred(opaque_ptr env,
                                             opaque_ptr deferred,
                                             opaque_ptr rejection);
NAPI_EXTERN napi_status napi_is_promise(opaque_ptr env,
                                        opaque_ptr promise,
                                        bool* is_promise);

// Running a script
NAPI_EXTERN napi_status napi_run_script(opaque_ptr env,
                                        opaque_ptr script,
                                        opaque_ptr* result);

// Memory management
NAPI_EXTERN napi_status napi_adjust_external_memory(opaque_ptr env,
                                                    int64_t change_in_bytes,
                                                    int64_t* adjusted_value);

#ifdef NAPI_EXPERIMENTAL

// Dates
NAPI_EXTERN napi_status napi_create_date(opaque_ptr env,
                                         double time,
                                         opaque_ptr* result);

NAPI_EXTERN napi_status napi_is_date(opaque_ptr env,
                                     opaque_ptr value,
                                     bool* is_date);

NAPI_EXTERN napi_status napi_get_date_value(opaque_ptr env,
                                            opaque_ptr value,
                                            double* result);

// BigInt
NAPI_EXTERN napi_status napi_create_bigint_int64(opaque_ptr env,
                                                 int64_t value,
                                                 opaque_ptr* result);
NAPI_EXTERN napi_status napi_create_bigint_uint64(opaque_ptr env,
                                                  uint64_t value,
                                                  opaque_ptr* result);
NAPI_EXTERN napi_status napi_create_bigint_words(opaque_ptr env,
                                                 int sign_bit,
                                                 size_t word_count,
                                                 const uint64_t* words,
                                                 opaque_ptr* result);
NAPI_EXTERN napi_status napi_get_value_bigint_int64(opaque_ptr env,
                                                    opaque_ptr value,
                                                    int64_t* result,
                                                    bool* lossless);
NAPI_EXTERN napi_status napi_get_value_bigint_uint64(opaque_ptr env,
                                                     opaque_ptr value,
                                                     uint64_t* result,
                                                     bool* lossless);
NAPI_EXTERN napi_status napi_get_value_bigint_words(opaque_ptr env,
                                                    opaque_ptr value,
                                                    int* sign_bit,
                                                    size_t* word_count,
                                                    uint64_t* words);
NAPI_EXTERN napi_status napi_add_finalizer(opaque_ptr env,
                                           opaque_ptr js_object,
                                           void* native_object,
                                           napi_finalize finalize_cb,
                                           void* finalize_hint,
                                           opaque_ptr* result);
#endif  // NAPI_EXPERIMENTAL

EXTERN_C_END

#endif  // WASM_NODE_API_JS_NATIVE_API_H_
