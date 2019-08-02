#include <assert.h>
#include <node_api.h>

typedef opaque_ptr napi_env;
typedef opaque_ptr napi_callback_info;
typedef opaque_ptr napi_value;

napi_value Hello(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_value world;
  status = napi_create_string_utf8(env, "world", 5, &world);
  assert(status == napi_ok);
  return world;
}

#define DECLARE_NAPI_METHOD(name, func) \
  { name, 0, func, 0, 0, 0, napi_default, 0 }

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;
  napi_property_descriptor desc = DECLARE_NAPI_METHOD("hello", Hello);
  status = napi_define_properties(env, exports, 1, &desc);
  assert(status == napi_ok);
  return exports;
}

napi_module mod = {
  NAPI_MODULE_VERSION,
  0,
  __FILE__,
  Init,
  "test_wasm_napi",
  NULL,
  {0}
};

int main(int argc, char **argv) {
  napi_module_register(&mod);
  return 0;
}
