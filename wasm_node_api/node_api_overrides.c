#include <stdlib.h>
#include "node_api_overrides.h"

void finalizeBuffer(napi_env env, void *finalize_data, void *finalize_hint) {
  free(finalize_data);
}

napi_status napi_create_arraybuffer(napi_env env, size_t byte_length,
                                    void **data, napi_value *result) {
  void *buffer = malloc(byte_length);
  if (!buffer) {
    return napi_generic_failure;
  }
  napi_status status = napi_create_external_arraybuffer(env, buffer, byte_length,
                                                        finalizeBuffer, NULL,
                                                        result);
  if (status == napi_ok) {
    *data = buffer;
  }
  return status;
}

napi_status napi_create_buffer(napi_env env, size_t size,
                               void **data, napi_value *result) {
  void *buffer = malloc(size);
  if (!buffer) {
    return napi_generic_failure;
  }
  napi_status status = napi_create_external_buffer(env, size, buffer,
                                                   finalizeBuffer, NULL,
                                                   result);
  if (status == napi_ok) {
    *data = buffer;
  }
  return status;
}

napi_status napi_create_buffer_copy(napi_env env, size_t length,
                                    const void *data,
                                    void **result_data,
                                    napi_value *result) {
  void *buffer = malloc(length);
  if (!buffer) {
    return napi_generic_failure;
  }
  napi_status status = napi_create_external_buffer(env, length, buffer,
                                                   finalizeBuffer,
                                                   NULL, result);
  if (status == napi_ok) {
    memcpy(buffer, data, length);
    *result_data = buffer;
  }
}

napi_property_descriptor* copy_properties(
    size_t property_count, const napi_property_descriptor* properties) {
  napi_property_descriptor* properties_ =
    create_property_descriptors(property_count);
  for (int i = 0; i < property_count; i++) {
    set_napi_property_descriptor_utf8name(
      &properties_[i], properties[i].utf8name);
    set_napi_property_descriptor_name(
      &properties_[i], properties[i].name);
    set_napi_property_descriptor_method(
      &properties_[i], properties[i].method);
    set_napi_property_descriptor_getter(
      &properties_[i], properties[i].getter);
    set_napi_property_descriptor_setter(
      &properties_[i], properties[i].setter);
    set_napi_property_descriptor_value(
      &properties_[i], properties[i].value);
    set_napi_property_descriptor_attributes(
      &properties_[i], properties[i].attributes);
    set_napi_property_descriptor_data(
      &properties_[i], properties[i].data);
  }
  return properties_;
}

napi_status napi_define_properties(
  napi_env env, napi_value object, size_t property_count,
  const napi_property_descriptor* properties
) {
  napi_property_descriptor* properties_ =
      copy_properties(property_count, properties);
  return _napi_define_properties(env, object, property_count, properties_);
}

napi_status napi_define_class(
  napi_env env, const char* utf8name, size_t length,
  napi_callback constructor, void* data, size_t property_count,
  const napi_property_descriptor* properties, napi_value* result
) {
  napi_property_descriptor* properties_ =
      copy_properties(property_count, properties);
  return _napi_define_class(env, utf8name, length, constructor,
                            data, property_count, properties_,
                            result);
}

char* get_c_string(const char* cString) {
  char* wasmString = (char*) malloc(get_string_length(cString));
  copy_string_to_wasm(cString, wasmString);
  return wasmString;
}

napi_status napi_get_last_error_info(
  napi_env env, const napi_extended_error_info** result
) {
  napi_extended_error_info* extended_error_info =
      create_napi_extended_error_info();
  napi_status status =
      _napi_get_last_error_info(env, &extended_error_info);

  napi_extended_error_info* result_ = *result;
  result_->engine_error_code =
      napi_extended_error_info_engine_error_code(extended_error_info);
  result_->engine_reserved =
      napi_extended_error_info_engine_reserved(extended_error_info);
  result_->error_code =
      napi_extended_error_info_error_code(extended_error_info);
  char* error_msg = napi_extended_error_info_error_message(extended_error_info);
  result_->error_message =
      get_c_string(error_msg);

  return status;
}
