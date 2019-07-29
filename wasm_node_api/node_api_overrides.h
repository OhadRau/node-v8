#ifndef NODE_API_OVERRIDES_H_
#define NODE_API_OVERRIDES_H_

#include <stdlib.h>
#include <string.h>
#include <node_api.h>

/* // UNUSED: b/c we define our own versions w/out the builtin implementations
napi_status _napi_create_arraybuffer(napi_env env, size_t byte_length,
                                     void **data, napi_value *result);

napi_status _napi_create_buffer(napi_env env, size_t size,
                                void **data, napi_value *result);

napi_status _napi_create_buffer_copy(napi_env env, size_t length,
                                     const void *data,
                                     void **result_data,
                                     napi_value *result); */

napi_property_descriptor* create_property_descriptors(
    size_t property_count);
void set_napi_property_descriptor_utf8name(
    napi_property_descriptor* pd, const char* utf8name);
void set_napi_property_descriptor_name(
    napi_property_descriptor* pd, napi_value name);
void set_napi_property_descriptor_method(
    napi_property_descriptor* pd, napi_callback method);
void set_napi_property_descriptor_getter(
    napi_property_descriptor* pd, napi_callback getter);
void set_napi_property_descriptor_setter(
    napi_property_descriptor* pd, napi_callback setter);
void set_napi_property_descriptor_value(
    napi_property_descriptor* pd, napi_value value);
void set_napi_property_descriptor_attributes(
    napi_property_descriptor* pd, napi_property_attributes attributes);
void set_napi_property_descriptor_data(
    napi_property_descriptor* pd, void* data);

napi_status _napi_define_properties(napi_env env,
                                    napi_value object,
                                    size_t property_count,
                                    const napi_property_descriptor* properties);

napi_status _napi_define_class(napi_env env,
                               const char* utf8name,
                               size_t length,
                               napi_callback constructor,
                               void* data,
                               size_t property_count,
                               const napi_property_descriptor* properties,
                               napi_value* result);

size_t get_string_length(const char* cString);
void copy_string_to_wasm(const char* cString, char* wasmString);

const char* napi_extended_error_info_error_message(
    napi_extended_error_info* eei);
void* napi_extended_error_info_engine_reserved(
    napi_extended_error_info* eei);
uint32_t napi_extended_error_info_engine_error_code(
    napi_extended_error_info* eei);
napi_status napi_extended_error_info_error_code(
    napi_extended_error_info* eei);

napi_extended_error_info* create_napi_extended_error_info();
napi_status _napi_get_last_error_info(napi_env env,
                                      const napi_extended_error_info** result);

#endif  // NODE_API_OVERRIDES_H_