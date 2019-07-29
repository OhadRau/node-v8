#ifndef WASM_NODE_API_JS_NATIVE_API_TYPES_H_
#define WASM_NODE_API_JS_NATIVE_API_TYPES_H_

// This file needs to be compatible with C compilers.
// This is a public include file, and these includes have essentially
// became part of it's API.
#include <stddef.h>  // NOLINT(modernize-deprecated-headers)
#include <stdint.h>  // NOLINT(modernize-deprecated-headers)

#if !defined __cplusplus || (defined(_MSC_VER) && _MSC_VER < 1900)
    typedef uint16_t char16_t;
#endif

typedef uint64_t opaque_ptr;

typedef enum {
  napi_default = 0,
  napi_writable = 1 << 0,
  napi_enumerable = 1 << 1,
  napi_configurable = 1 << 2,

  // Used with napi_define_class to distinguish static properties
  // from instance properties. Ignored by napi_define_properties.
  napi_static = 1 << 10,
} napi_property_attributes;

typedef enum {
  // ES6 types (corresponds to typeof)
  napi_undefined,
  napi_null,
  napi_boolean,
  napi_number,
  napi_string,
  napi_symbol,
  napi_object,
  napi_function,
  napi_external,
  napi_bigint,
} napi_valuetype;

typedef enum {
  napi_int8_array,
  napi_uint8_array,
  napi_uint8_clamped_array,
  napi_int16_array,
  napi_uint16_array,
  napi_int32_array,
  napi_uint32_array,
  napi_float32_array,
  napi_float64_array,
  napi_bigint64_array,
  napi_biguint64_array,
} napi_typedarray_type;

typedef enum {
  napi_ok,
  napi_invalid_arg,
  napi_object_expected,
  napi_string_expected,
  napi_name_expected,
  napi_function_expected,
  napi_number_expected,
  napi_boolean_expected,
  napi_array_expected,
  napi_generic_failure,
  napi_pending_exception,
  napi_cancelled,
  napi_escape_called_twice,
  napi_handle_scope_mismatch,
  napi_callback_scope_mismatch,
  napi_queue_full,
  napi_closing,
  napi_bigint_expected,
  napi_date_expected,
} napi_status;

typedef opaque_ptr (*napi_callback)(opaque_ptr env,
                                    opaque_ptr info);
typedef void (*napi_finalize)(opaque_ptr env,
                              void* finalize_data,
                              void* finalize_hint);

typedef struct {
  // One of utf8name or name should be NULL.
  const char* utf8name;
  opaque_ptr name;

  napi_callback method;
  napi_callback getter;
  napi_callback setter;
  opaque_ptr value;

  napi_property_attributes attributes;
  void* data;
} napi_property_descriptor;

typedef struct {
  const char* error_message;
  void* engine_reserved;
  uint32_t engine_error_code;
  napi_status error_code;
} napi_extended_error_info;

#endif  // WASM_NODE_API_JS_NATIVE_API_TYPES_H_
