#ifndef NODE_API_OVERRIDES_H_
#define NODE_API_OVERRIDES_H_

#include <stdlib.h>
#include <string.h>

#include "node_api.h"

NAPI_EXTERN void _napi_module_register(opaque_ptr mod);
NAPI_EXTERN opaque_ptr create_napi_module();
NAPI_EXTERN void NAPI_EXTERN set_napi_module_nm_version(opaque_ptr mod, int nm_version);
NAPI_EXTERN void set_napi_module_nm_flags(opaque_ptr mod, unsigned int nm_flags);
NAPI_EXTERN void set_napi_module_nm_filename(opaque_ptr mod, const char* nm_filename);
NAPI_EXTERN void set_napi_module_nm_register_func(opaque_ptr mod, napi_addon_register_func nm_register_func);
NAPI_EXTERN void set_napi_module_nm_modname(opaque_ptr mod, const char* nm_modname);
NAPI_EXTERN void set_napi_module_nm_priv(opaque_ptr mod, void* nm_priv);

NAPI_EXTERN opaque_ptr create_property_descriptors(
    size_t property_count);
NAPI_EXTERN void set_napi_property_descriptor_utf8name(
    opaque_ptr pd, int offset, const char* utf8name);
NAPI_EXTERN void set_napi_property_descriptor_name(
    opaque_ptr pd, int offset, opaque_ptr name);
NAPI_EXTERN void set_napi_property_descriptor_method(
    opaque_ptr pd, int offset, napi_callback method);
NAPI_EXTERN void set_napi_property_descriptor_getter(
    opaque_ptr pd, int offset, napi_callback getter);
NAPI_EXTERN void set_napi_property_descriptor_setter(
    opaque_ptr pd, int offset, napi_callback setter);
NAPI_EXTERN void set_napi_property_descriptor_value(
    opaque_ptr pd, int offset, opaque_ptr value);
NAPI_EXTERN void set_napi_property_descriptor_attributes(
    opaque_ptr pd, int offset, napi_property_attributes attributes);
NAPI_EXTERN void set_napi_property_descriptor_data(
    opaque_ptr pd, int offset, void* data);

NAPI_EXTERN napi_status _napi_define_properties(opaque_ptr env,
                                                opaque_ptr object,
                                                size_t property_count,
                                                opaque_ptr properties);

NAPI_EXTERN napi_status _napi_define_class(opaque_ptr env,
                                           const char* utf8name,
                                           size_t length,
                                           napi_callback constructor,
                                           void* data,
                                           size_t property_count,
                                           opaque_ptr properties,
                                           opaque_ptr* result);

NAPI_EXTERN size_t get_string_length(opaque_ptr cString);
NAPI_EXTERN void copy_string_to_wasm(opaque_ptr cString, char* wasmString);

NAPI_EXTERN const char* napi_extended_error_info_error_message(
    opaque_ptr eei);
NAPI_EXTERN void* napi_extended_error_info_engine_reserved(
    opaque_ptr eei);
NAPI_EXTERN uint32_t napi_extended_error_info_engine_error_code(
    opaque_ptr eei);
NAPI_EXTERN napi_status napi_extended_error_info_error_code(
    opaque_ptr eei);

NAPI_EXTERN opaque_ptr create_napi_extended_error_info();
NAPI_EXTERN napi_status _napi_get_last_error_info(opaque_ptr env,
                                      opaque_ptr* result);

#endif  // NODE_API_OVERRIDES_H_