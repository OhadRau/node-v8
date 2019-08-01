#include <type_traits>
#include <functional>
#include <uv.h>

#define NODE_WANT_INTERNALS 1
#include "js_native_api_v8.h"
#include "wasm_node_api.h"
#include "node_api.h"
#include "v8-wasm.h"
#include "env.h"
#undef NODE_WANT_INTERNALS

namespace node {
namespace wasm_napi {

namespace w = v8::wasm;

using callback = w::Func::callbackType;

template<typename T>
struct is_function_pointer {
  static const bool value =
    std::is_pointer<T>::value ?
      std::is_function<typename std::remove_pointer<T>::type>::value :
      false;
};

template<typename T>
constexpr auto is_function_pointer_v = is_function_pointer<T>::value;

template<typename T>
class Native {
public:
  using type = T;
};

template<typename T>
class Wasm {
public:
  using type = T;
};

template<typename>
struct is_native : public std::false_type {};

template<typename T>
struct is_native<Native<T>> : public std::true_type {};

template <typename T>
constexpr auto is_native_v = is_native<T>::value;

template<typename>
struct is_wasm : public std::false_type {};

template<typename T>
struct is_wasm<Wasm<T>> : public std::true_type {};

template <typename T>
constexpr auto is_wasm_v = is_wasm<T>::value;

template<class K>
using kind = typename K::type;

template<typename T>
constexpr w::ValKind wasmType() {
  if constexpr (std::is_same<kind<T>, float>::value) {
    return w::ValKind::F32;
  } else if constexpr (std::is_same<kind<T>, double>::value) {
    return w::ValKind::F64;
  #if __WORDSIZE == 64
  } else if constexpr (is_native_v<T> && std::is_pointer<kind<T>>::value) {
    return w::ValKind::I64;
  #endif
  } else {
    return w::ValKind::I32;
  }
}

template<w::ValKind... Args>
constexpr auto exportVoid(callback cb) -> w::Func {
  return w::Func(
    w::FuncType({Args...}, {}),
    cb
  );
};

template<w::ValKind Return, w::ValKind... Args>
constexpr auto exportFn(callback cb) -> w::Func {
  return w::Func(
    w::FuncType({Args...}, {Return}),
    cb
  );
};

template<typename Return, typename... Args>
constexpr auto exportNative(callback cb) -> w::Func {
  if constexpr(std::is_void<kind<Return>>::value) {
    return exportVoid<wasmType<Args>()...>(cb);
  } else {
    return exportFn<wasmType<Return>(), wasmType<Args>()...>(cb);
  }
}

template<typename Arg>
constexpr Arg napiCast(const w::Val& v) {
  if constexpr(std::is_same<Arg, float>::value) {
    return (Arg) v.f32();
  } else if constexpr(std::is_same<Arg, double>::value) {
    return (Arg) v.f64();
  } else {
    return (Arg) v.i32();
  }
}

template<typename Result>
constexpr kind<Result> napiUnwrap(const w::Context* ctx, const w::Val& v) {
  if constexpr(is_wasm_v<Result>) {
    if constexpr(/*std::is_same<kind<Result>,
                              v8::MaybeLocal<v8::Function>>::value*/
                 is_function_pointer_v<kind<Result>>) {
      return (kind<Result>) v.i32();/*(ctx->table->get(v.i32()));*/
    } else {
      return (kind<Result>) (&ctx->memory->data()[(uint32_t) v.i32()]);
    }
  #if __WORDSIZE == 64
  } else if constexpr(std::is_pointer<kind<Result>>::value) { // Native pointer
    return (kind<Result>) v.i64();
  #endif
  } else {
    return napiCast<kind<Result>>(v);
  }
};

template<typename Result>
constexpr w::Val napiWrap(kind<Result> v) {
  if constexpr(std::is_same<kind<Result>, float>::value) {
    return w::Val((float) v);
  } else if constexpr (std::is_same<kind<Result>, double>::value) {
    return w::Val((double) v);
  #if __WORDSIZE == 64
  } else if constexpr(std::is_pointer<kind<Result>>::value) { // Native pointer
      return w::Val(static_cast<int64_t>((size_t) v));
  #endif
  } else {
    return w::Val(static_cast<int32_t>((size_t) v));
  }
}

template<typename Result, typename... Args>
struct napiApply {
  template<size_t... Indices>
  static constexpr auto apply(
    kind<Result> (*fn)(kind<Args>...),
    const w::Context* ctx,
    const w::Val args[],
    std::index_sequence<Indices...>
  ) -> kind<Result> {
    if constexpr(std::is_void<kind<Result>>::value) {
      fn(napiUnwrap<Args>(ctx, args[Indices])...);
    } else {
      return fn(napiUnwrap<Args>(ctx, args[Indices])...);
    }
  }
};

template<typename Result, typename... Args>
struct napiCall {
private:
  using fp = kind<Result> (*)(kind<Args>...);
public:
  template<fp Fn>
  static void const call(
    const w::Context* ctx, const w::Val args[], w::Val results[]
  ) {
    if constexpr(std::is_void<kind<Result>>::value) {
      napiApply<Result, Args...>::apply(Fn, ctx, args,
                                  std::index_sequence_for<Args...>{});
    } else {
      results[0] = napiWrap<Result>(
        napiApply<Result, Args...>::apply(Fn, ctx, args,
                                          std::index_sequence_for<Args...>{}));
    }
  }
};

template<typename Result, typename... Args>
struct napiBind {
  template<kind<Result> (*const Fn)(kind<Args>...)>
  static void bind(v8::Isolate* isolate, const char *name) {
    w::Func fn = exportNative<Result, Args...>(
        (callback) napiCall<Result, Args...>::template call<Fn>);
    w::RegisterEmbedderBuiltin(isolate, "napi_unstable", name, fn);
  }
};

// Functions to deal with copying strings
size_t get_string_length(const char* cString) {
  return strlen(cString);
}

void copy_string_to_wasm(const char* cString, char* wasmString) {
  strcpy(wasmString, cString);
}

// Functions for dealing with napi modules
napi_module* create_napi_module() {
  return (napi_module*) malloc(sizeof(napi_module));
}

void set_napi_module_nm_version(napi_module* mod, int nm_version) {
  mod->nm_version = nm_version;
}

void set_napi_module_nm_flags(napi_module* mod, unsigned int nm_flags) {
  mod->nm_flags = nm_flags;
}

void set_napi_module_nm_filename(napi_module* mod, const char* nm_filename) {
  mod->nm_filename = nm_filename;
}

// TODO(ohadrau): Is this a safe operation?
void set_napi_module_nm_register_func(
    napi_module* mod,
    napi_addon_register_func nm_register_func) {
  mod->nm_register_func = nm_register_func;
}

void set_napi_module_nm_modname(napi_module* mod, const char* nm_modname) {
  mod->nm_modname = nm_modname;
}

void set_napi_module_nm_priv(napi_module* mod, void* nm_priv) {
  mod->nm_priv = nm_priv;
}

// Copied from node_api.cc
struct node_napi_env__ : public napi_env__ {
  explicit node_napi_env__(v8::Local<v8::Context> context):
      napi_env__(context) {
    CHECK_NOT_NULL(node_env());
  }

  inline node::Environment* node_env() const {
    return node::Environment::GetCurrent(context());
  }

  bool can_call_into_js() const override {
    return node_env()->can_call_into_js();
  }
};

typedef node_napi_env__* node_napi_env;

static inline napi_env GetEnv(v8::Local<v8::Context> context) {
  node_napi_env result;

  auto isolate = context->GetIsolate();
  auto global = context->Global();

  // In the case of the string for which we grab the private and the value of
  // the private on the global object we can call .ToLocalChecked() directly
  // because we need to stop hard if either of them is empty.
  //
  // Re https://github.com/nodejs/node/pull/14217#discussion_r128775149
  auto value = global->GetPrivate(context, NAPI_PRIVATE_KEY(context, env))
      .ToLocalChecked();

  if (value->IsExternal()) {
    result = static_cast<node_napi_env>(value.As<v8::External>()->Value());
  } else {
    result = new node_napi_env__(context);

    auto external = v8::External::New(isolate, result);

    // We must also stop hard if the result of assigning the env to the global
    // is either nothing or false.
    CHECK(global->SetPrivate(context, NAPI_PRIVATE_KEY(context, env), external)
        .FromJust());

    // TODO(addaleax): There was previously code that tried to delete the
    // napi_env when its v8::Context was garbage collected;
    // However, as long as N-API addons using this napi_env are in place,
    // the Context needs to be accessible and alive.
    // Ideally, we'd want an on-addon-unload hook that takes care of this
    // once all N-API addons using this napi_env are unloaded.
    // For now, a per-Environment cleanup hook is the best we can do.
    result->node_env()->AddCleanupHook(
        [](void* arg) {
          static_cast<napi_env>(arg)->Unref();
        },
        static_cast<void*>(result));
  }

  return result;
}

struct wasm_napi_module_data {
  napi_module* mod;
  w::Context* ctx;
};

void _napi_module_register_by_symbol(v8::Local<v8::Value> exports,
                                     v8::Local<v8::Value> module,
                                     v8::Local<v8::Context> context,
                                     w::Context* wasmContext,
                                     napi_addon_register_func fn) {
  v8::Isolate* isolate = context->GetIsolate();
  v8::MaybeLocal<v8::Function> initFunc =
      wasmContext->table->get((size_t) fn);
  delete wasmContext;
  if (initFunc.IsEmpty()) {
    node::Environment* node_env = node::Environment::GetCurrent(context);
    CHECK_NOT_NULL(node_env);
    node_env->ThrowError(
        "Module has no declared entry point.");
    return;
  }

  // Create a new napi_env for this module or reference one if a pre-existing
  // one is found.
  napi_env env = GetEnv(context);

  napi_value _exports;
  NapiCallIntoModuleThrow(env, [&]() {
    v8::Local<v8::Value>* exportsPtr =
        new v8::Local<v8::Value>(exports);

    v8::Local<v8::Value> argv[2];
    // TODO(ohadrau): Can we use v8::External here? What would that translate to?
    argv[0] = v8::BigInt::New(isolate, (size_t) env);
    argv[1] = v8::BigInt::New(isolate, (size_t) exportsPtr);

    v8::MaybeLocal<v8::Value> result =
      initFunc.ToLocalChecked()->Call(context, context->Global(), 2, argv);
    if (initFunc.IsEmpty()) {
      node::Environment* node_env = node::Environment::GetCurrent(context);
      CHECK_NOT_NULL(node_env);
      node_env->ThrowError(
          "Module initializer returned void.");
      return;
    }
    v8::Local<v8::Value> resultLocal;
    result.ToLocal(&resultLocal);
    exportsPtr =
        (v8::Local<v8::Value>*) (size_t) resultLocal
                                         ->IntegerValue(context)
                                         .ToChecked();
    _exports = v8impl::JsValueFromV8LocalValue(*exportsPtr);

    delete exportsPtr;
  });

  // If register function returned a non-null exports object different from
  // the exports object we passed it, set that as the "exports" property of
  // the module.
  if (_exports != nullptr &&
      _exports != v8impl::JsValueFromV8LocalValue(exports)) {
    napi_value _module = v8impl::JsValueFromV8LocalValue(module);
    napi_set_named_property(env, _module, "exports", _exports);
  }
}

// Intercepts the Node-V8 module registration callback. Converts parameters
// to NAPI equivalents and then calls the registration callback specified
// by the NAPI module.
static void _napi_module_register_cb(v8::Local<v8::Object> exports,
                                     v8::Local<v8::Value> module,
                                     v8::Local<v8::Context> context,
                                     void* priv) {
  wasm_napi_module_data* data =
      reinterpret_cast<wasm_napi_module_data*>(priv);
  _napi_module_register_by_symbol(exports, module, context,
          data->ctx, data->mod->nm_register_func);
}

// Registers a NAPI module.
void _napi_module_register(w::Context* ctx, const w::Val args[],
                           w::Val results[]) {
  napi_module* mod = (napi_module*) (size_t) args[0].i64();
  w::Context* ctxCopy = new w::Context(*ctx);
  node::node_module* nm = new node::node_module {
    -1,
    mod->nm_flags | NM_F_DELETEME,
    nullptr,
    mod->nm_filename,
    nullptr,
    _napi_module_register_cb,
    mod->nm_modname,
    new wasm_napi_module_data { mod, ctxCopy },  // priv
    nullptr,
  };
  // TODO(ohadrau): Calling this will pass our v8::Context
  // into the module rather than a v8::wasm::Context. This
  // means we won't be able to access WASM memory and function
  // tables within the module register function! (i.e. the
  // whole point of writing this alternate register function)
  node::node_module_register(nm);
}

// Functions for dealing with property descriptors
napi_property_descriptor* create_property_descriptors(
    size_t property_count) {
  size_t total_size = property_count * sizeof(napi_property_descriptor);
  return (napi_property_descriptor*) malloc(total_size);
}

void set_napi_property_descriptor_utf8name(
    napi_property_descriptor* pd, int offset, const char* utf8name) {
  pd[offset].utf8name = utf8name;
}

void set_napi_property_descriptor_name(
    napi_property_descriptor* pd, int offset, napi_value name) {
  pd[offset].name = name;
}

void set_napi_property_descriptor_method(
    napi_property_descriptor* pd, int offset, napi_callback method) {
  pd[offset].method = method;
}

void set_napi_property_descriptor_getter(
    napi_property_descriptor* pd, int offset, napi_callback getter) {
  pd[offset].getter = getter;
}

void set_napi_property_descriptor_setter(
    napi_property_descriptor* pd, int offset, napi_callback setter) {
  pd[offset].setter = setter;
}

void set_napi_property_descriptor_value(
    napi_property_descriptor* pd, int offset, napi_value value) {
  pd[offset].value = value;
}

void set_napi_property_descriptor_attributes(
    napi_property_descriptor* pd, int offset, napi_property_attributes attributes) {
  pd[offset].attributes = attributes;
}

void set_napi_property_descriptor_data(
    napi_property_descriptor* pd, int offset, void* data) {
  pd[offset].data = data;
}

// Functions for dealing with extended error infos
napi_extended_error_info* create_napi_extended_error_info() {
  return (napi_extended_error_info*) malloc(sizeof(napi_extended_error_info));
}

const char* napi_extended_error_info_error_message(
    napi_extended_error_info* eei) {
  return eei->error_message;
}

void* napi_extended_error_info_engine_reserved(napi_extended_error_info* eei) {
  return eei->engine_reserved;
}

uint32_t napi_extended_error_info_engine_error_code(
    napi_extended_error_info* eei) {
  return eei->engine_error_code;
}

napi_status napi_extended_error_info_error_code(napi_extended_error_info* eei) {
  return eei->error_code;
}

void RegisterNapiBuiltins(v8::Isolate* isolate) {
  w::Func fn = exportNative<Native<void>, Native<napi_module*>>(
        (callback) _napi_module_register);
  w::RegisterEmbedderBuiltin(isolate, "napi_unstable", "_napi_module_register", fn);
  napiBind<Native<napi_module*>>
    ::bind<create_napi_module>(isolate, "create_napi_module");
  napiBind<Native<void>, Native<napi_module*>, Native<int>>
    ::bind<set_napi_module_nm_version>(isolate, "set_napi_module_nm_version");
  napiBind<Native<void>, Native<napi_module*>, Native<unsigned int>>
    ::bind<set_napi_module_nm_flags>(isolate, "set_napi_module_nm_flags");
  napiBind<Native<void>, Native<napi_module*>, Wasm<const char*>>
    ::bind<set_napi_module_nm_filename>(isolate, "set_napi_module_nm_filename");
  napiBind<Native<void>, Native<napi_module*>, Wasm<napi_addon_register_func>>
    ::bind<set_napi_module_nm_register_func>(isolate, "set_napi_module_nm_register_func");
  napiBind<Native<void>, Native<napi_module*>, Wasm<const char*>>
    ::bind<set_napi_module_nm_modname>(isolate, "set_napi_module_nm_modname");
  napiBind<Native<void>, Native<napi_module*>, Wasm<void*>>
    ::bind<set_napi_module_nm_priv>(isolate, "set_napi_module_nm_priv");

  napiBind<Native<napi_status>, Native<napi_env>, Wasm<const napi_extended_error_info **>>
    ::bind<napi_get_last_error_info>(isolate, "_napi_get_last_error_info");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>>
    ::bind<napi_throw>(isolate, "napi_throw");
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<const char*>, Wasm<const char *>>
    ::bind<napi_throw_error>(isolate, "napi_throw_error");
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<const char*>, Wasm<const char *>>
    ::bind<napi_throw_type_error>(isolate, "napi_throw_type_error");
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<const char*>, Wasm<const char *>>
    ::bind<napi_throw_range_error>(isolate, "napi_throw_range_error");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<bool *>>
    ::bind<napi_is_error>(isolate, "napi_is_error");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Native<napi_value>, Wasm<napi_value *>>
    ::bind<napi_create_error>(isolate, "napi_create_error");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Native<napi_value>, Wasm<napi_value *>>
    ::bind<napi_create_type_error>(isolate, "napi_create_type_error");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Native<napi_value>, Wasm<napi_value *>>
    ::bind<napi_create_range_error>(isolate, "napi_create_range_error");
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<napi_value *>>
    ::bind<napi_get_and_clear_last_exception>(isolate, "napi_get_and_clear_last_exception");
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<bool *>>
    ::bind<napi_is_exception_pending>(isolate, "napi_is_exception_pending");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>>
    ::bind<napi_fatal_exception>(isolate, "napi_fatal_exception");
  napiBind<Native<void>, Wasm<const char *>, Native<size_t>, Wasm<const char *>, Native<size_t>>
    ::bind<napi_fatal_error>(isolate, "napi_fatal_error");
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<napi_handle_scope *>>
    ::bind<napi_open_handle_scope>(isolate, "napi_open_handle_scope");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_handle_scope>>
    ::bind<napi_close_handle_scope>(isolate, "napi_close_handle_scope");
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<napi_escapable_handle_scope *>>
    ::bind<napi_open_escapable_handle_scope>(isolate, "napi_open_escapable_handle_scope");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_escapable_handle_scope>>
    ::bind<napi_close_escapable_handle_scope>(isolate, "napi_close_escapable_handle_scope");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_escapable_handle_scope>, Native<napi_value>, Wasm<napi_value *>>
    ::bind<napi_escape_handle>(isolate, "napi_escape_handle");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Native<uint32_t>, Wasm<napi_ref *>>
    ::bind<napi_create_reference>(isolate, "napi_create_reference");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_ref>>
    ::bind<napi_delete_reference>(isolate, "napi_delete_reference");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_ref>, Wasm<uint32_t *>>
    ::bind<napi_reference_ref>(isolate, "napi_reference_ref");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_ref>, Wasm<uint32_t *>>
    ::bind<napi_reference_unref>(isolate, "napi_reference_ref");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_ref>, Wasm<napi_value *>>
    ::bind<napi_get_reference_value>(isolate, "napi_get_reference_value");
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<void (*)(void *)>, Wasm<void *>>
    ::bind<napi_add_env_cleanup_hook>(isolate, "napi_add_env_cleanup_hook");
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<void (*)(void *)>, Wasm<void *>>
    ::bind<napi_remove_env_cleanup_hook>(isolate, "napi_remove_env_cleanup_hook");
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<napi_value *>>
    ::bind<napi_create_array>(isolate, "napi_create_array");
  napiBind<Native<napi_status>, Native<napi_env>, Native<size_t>, Wasm<napi_value *>>
    ::bind<napi_create_array_with_length>(isolate, "napi_create_array_with_length");
  #if NAPI_VERSION >= 4 && defined(NAPI_EXPERIMENTAL)
  napiBind<Native<napi_status>, Native<napi_env>, Native<double>, Wasm<napi_value *>>
    ::bind<napi_create_date>(isolate, "napi_create_date");
  #endif
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<void *>, Wasm<napi_finalize>, Wasm<void *>, Wasm<napi_value *>>
    ::bind<napi_create_external>(isolate, "napi_create_external");
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<void *>, Native<size_t>, Wasm<napi_finalize>, Wasm<void *>, Wasm<napi_value *>>
    ::bind<napi_create_external_arraybuffer>(isolate, "napi_create_external_arraybuffer");
  napiBind<Native<napi_status>, Native<napi_env>, Native<size_t>, Wasm<void *>, Wasm<napi_finalize>, Wasm<void *>, Wasm<napi_value *>>
    ::bind<napi_create_external_buffer>(isolate, "napi_create_external_buffer");
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<napi_value *>>
    ::bind<napi_create_object>(isolate, "napi_create_object");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<napi_value *>>
    ::bind<napi_create_symbol>(isolate, "napi_create_symbol");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_typedarray_type>, Native<size_t>, Native<napi_value>, Native<size_t>, Wasm<napi_value *>>
    ::bind<napi_create_typedarray>(isolate, "napi_create_typedarray");
  napiBind<Native<napi_status>, Native<napi_env>, Native<size_t>, Native<napi_value>, Native<size_t>, Wasm<napi_value *>>
    ::bind<napi_create_dataview>(isolate, "napi_create_dataview");
  napiBind<Native<napi_status>, Native<napi_env>, Native<int32_t>, Wasm<napi_value *>>
    ::bind<napi_create_int32>(isolate, "napi_create_int32");
  napiBind<Native<napi_status>, Native<napi_env>, Native<uint32_t>, Wasm<napi_value *>>
    ::bind<napi_create_uint32>(isolate, "napi_create_uint32");
  napiBind<Native<napi_status>, Native<napi_env>, Native<int64_t>, Wasm<napi_value *>>
    ::bind<napi_create_int64>(isolate, "napi_create_int64");
  napiBind<Native<napi_status>, Native<napi_env>, Native<double>, Wasm<napi_value *>>
    ::bind<napi_create_double>(isolate, "napi_create_double");
  #if NAPI_VERSION >= 4 && defined(NAPI_EXPERIMENTAL)
  napiBind<Native<napi_status>, Native<napi_env>, Native<int64_t>, Wasm<napi_value *>>
    ::bind<napi_create_bigint_int64>(isolate, "napi_create_bigint_int64");
  napiBind<Native<napi_status>, Native<napi_env>, Native<uint64_t>, Wasm<napi_value *>>
    ::bind<napi_create_bigint_uint64>(isolate, "napi_create_bigint_uint64");
  napiBind<Native<napi_status>, Native<napi_env>, Native<int>, Native<size_t>, Wasm<const uint64_t *>, Wasm<napi_value *>>
    ::bind<napi_create_bigint_words>(isolate, "napi_create_bigint_words");
  #endif
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<const char *>, Native<size_t>, Wasm<napi_value *>>
    ::bind<napi_create_string_latin1>(isolate, "napi_create_string_latin1");
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<const char16_t *>, Native<size_t>, Wasm<napi_value *>>
    ::bind<napi_create_string_utf16>(isolate, "napi_create_string_utf16");
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<const char *>, Native<size_t>, Wasm<napi_value *>>
    ::bind<napi_create_string_utf8>(isolate, "napi_create_string_utf8");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<uint32_t *>>
    ::bind<napi_get_array_length>(isolate, "napi_get_array_length");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<napi_value *>>
    ::bind<napi_get_prototype>(isolate, "napi_get_prototype");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<napi_typedarray_type *>, Wasm<size_t *>, Wasm<void **>, Wasm<napi_value *>, Wasm<size_t *>>
    ::bind<napi_get_typedarray_info>(isolate, "napi_get_typedarray_info");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<size_t *>, Wasm<void **>, Wasm<napi_value *>, Wasm<size_t *>>
    ::bind<napi_get_dataview_info>(isolate, "napi_get_dataview_info");
  #if NAPI_VERSION >= 4 && defined(NAPI_EXPERIMENTAL)
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<double *>>
    ::bind<napi_get_date_value>(isolate, "napi_get_date_value");
  #endif
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<bool *>>
    ::bind<napi_get_value_bool>(isolate, "napi_get_value_bool");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<double *>>
    ::bind<napi_get_value_double>(isolate, "napi_get_value_double");
  #if NAPI_VERSION >= 4 && defined(NAPI_EXPERIMENTAL)
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<int64_t *>, Wasm<bool *>>
    ::bind<napi_get_value_bigint_int64>(isolate, "napi_get_value_bigint_int64");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<uint64_t *>, Wasm<bool *>>
    ::bind<napi_get_value_bigint_uint64>(isolate, "napi_get_value_bigint_uint64");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<int *>, Wasm<size_t *>, Wasm<uint64_t *>>
    ::bind<napi_get_value_bigint_words>(isolate, "napi_get_value_bigint_words");
  #endif
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<void **>>
    ::bind<napi_get_value_external>(isolate, "napi_get_value_external");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<int32_t *>>
    ::bind<napi_get_value_int32>(isolate, "napi_get_value_int32");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<uint32_t *>>
    ::bind<napi_get_value_uint32>(isolate, "napi_get_value_uint32");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<int64_t *>>
    ::bind<napi_get_value_int64>(isolate, "napi_get_value_int64");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<char *>, Native<size_t>, Wasm<size_t *>>
    ::bind<napi_get_value_string_latin1>(isolate, "napi_get_value_string_latin1");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<char *>, Native<size_t>, Wasm<size_t *>>
    ::bind<napi_get_value_string_utf8>(isolate, "napi_get_value_string_utf8");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<char16_t *>, Native<size_t>, Wasm<size_t *>>
    ::bind<napi_get_value_string_utf16>(isolate, "napi_get_value_string_utf16");
  napiBind<Native<napi_status>, Native<napi_env>, Native<bool>, Wasm<napi_value *>>
    ::bind<napi_get_boolean>(isolate, "napi_get_boolean");
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<napi_value *>>
    ::bind<napi_get_global>(isolate, "napi_get_global");
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<napi_value *>>
    ::bind<napi_get_null>(isolate, "napi_get_null");
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<napi_value *>>
    ::bind<napi_get_undefined>(isolate, "napi_get_undefined");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<napi_value *>>
    ::bind<napi_coerce_to_bool>(isolate, "napi_coerce_to_bool");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<napi_value *>>
    ::bind<napi_coerce_to_number>(isolate, "napi_coerce_to_number");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<napi_value *>>
    ::bind<napi_coerce_to_object>(isolate, "napi_coerce_to_object"),
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<napi_value *>>
    ::bind<napi_coerce_to_string>(isolate, "napi_coerce_to_string");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<napi_valuetype *>>
    ::bind<napi_typeof>(isolate, "napi_typeof");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Native<napi_value>, Wasm<bool *>>
    ::bind<napi_instanceof>(isolate, "napi_instanceof");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<bool *>>
    ::bind<napi_is_array>(isolate, "napi_is_array");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<bool *>>
    ::bind<napi_is_arraybuffer>(isolate, "napi_is_arraybuffer");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<bool *>>
    ::bind<napi_is_buffer>(isolate, "napi_is_buffer");
  #if NAPI_VERSION >= 4 && defined(NAPI_EXPERIMENTAL)
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<bool *>>
    ::bind<napi_is_date>(isolate, "napi_is_date");
  #endif
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<bool *>>
    ::bind<napi_is_error>(isolate, "napi_is_error");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<bool *>>
    ::bind<napi_is_typedarray>(isolate, "napi_is_typedarray");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<bool *>>
    ::bind<napi_is_dataview>(isolate, "napi_is_dataview");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Native<napi_value>, Wasm<bool *>>
    ::bind<napi_strict_equals>(isolate, "napi_strict_equals");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<napi_value *>>
    ::bind<napi_get_property_names>(isolate, "napi_get_property_names");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Native<napi_value>, Native<napi_value>>
    ::bind<napi_set_property>(isolate, "napi_set_property");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Native<napi_value>, Wasm<napi_value *>>
    ::bind<napi_get_property>(isolate, "napi_get_property");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Native<napi_value>, Wasm<bool *>>
    ::bind<napi_has_property>(isolate, "napi_has_property");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Native<napi_value>, Wasm<bool *>>
    ::bind<napi_delete_property>(isolate, "napi_delete_property");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Native<napi_value>, Wasm<bool *>>
    ::bind<napi_has_own_property>(isolate, "napi_has_own_property");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<const char *>, Native<napi_value>>
    ::bind<napi_set_named_property>(isolate, "napi_set_named_property");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<const char *>, Wasm<napi_value *>>
    ::bind<napi_get_named_property>(isolate, "napi_get_named_property");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<const char *>, Wasm<bool *>>
    ::bind<napi_has_named_property>(isolate, "napi_has_named_property");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Native<uint32_t>, Native<napi_value>>
    ::bind<napi_set_element>(isolate, "napi_set_element");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Native<uint32_t>, Wasm<napi_value *>>
    ::bind<napi_get_element>(isolate, "napi_get_element");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Native<uint32_t>, Wasm<bool *>>
    ::bind<napi_has_element>(isolate, "napi_has_element");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Native<uint32_t>, Wasm<bool *>>
    ::bind<napi_delete_element>(isolate, "napi_delete_element");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Native<size_t>, Native<const napi_property_descriptor *>>
    ::bind<napi_define_properties>(isolate, "_napi_define_properties");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Native<napi_value>, Native<size_t>, Wasm<const napi_value *>, Wasm<napi_value *>>
    ::bind<napi_call_function>(isolate, "napi_call_function");
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<const char *>, Native<size_t>, Wasm<napi_callback>, Wasm<void *>, Wasm<napi_value *>>
    ::bind<napi_create_function>(isolate, "napi_create_function");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_callback_info>, Wasm<size_t *>, Wasm<napi_value *>, Wasm<napi_value *>, Wasm<void **>>
    ::bind<napi_get_cb_info>(isolate, "napi_get_cb_info");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_callback_info>, Wasm<napi_value *>>
    ::bind<napi_get_new_target>(isolate, "napi_get_new_target");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Native<size_t>, Wasm<const napi_value *>, Wasm<napi_value *>>
    ::bind<napi_new_instance>(isolate, "napi_new_instance");
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<const char *>, Native<size_t>, Wasm<napi_callback>, Wasm<void *>, Native<size_t>, Wasm<const napi_property_descriptor *>, Wasm<napi_value *>>
    ::bind<napi_define_class>(isolate, "_napi_define_class");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<void *>, Wasm<napi_finalize>, Wasm<void *>, Wasm<napi_ref *>>
    ::bind<napi_wrap>(isolate, "napi_wrap");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<void **>>
    ::bind<napi_unwrap>(isolate, "napi_unwrap");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<void **>>
    ::bind<napi_remove_wrap>(isolate, "napi_remove_wrap");
  #if NAPI_VERSION >= 4 && defined(NAPI_EXPERIMENTAL)
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<void *>, Wasm<napi_finalize>, Wasm<void *>, Wasm<napi_ref *>>
    ::bind<napi_add_finalizer>(isolate, "napi_add_finalizer");
  #endif
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Native<napi_value>, Wasm<napi_async_execute_callback>, Wasm<napi_async_complete_callback>, Wasm<void *>, Wasm<napi_async_work *>>
    ::bind<napi_create_async_work>(isolate, "napi_create_async_work");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_async_work>>
    ::bind<napi_delete_async_work>(isolate, "napi_delete_async_work");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_async_work>>
    ::bind<napi_queue_async_work>(isolate, "napi_queue_async_work");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_async_work>>
    ::bind<napi_cancel_async_work>(isolate, "napi_cancel_async_work");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Native<napi_value>, Wasm<napi_async_context *>>
    ::bind<napi_async_init>(isolate, "napi_async_init");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_async_context>>
    ::bind<napi_async_destroy>(isolate, "napi_async_destroy");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_async_context>, Native<napi_value>, Native<napi_value>, Native<size_t>, Wasm<const napi_value *>, Wasm<napi_value *>>
    ::bind<napi_make_callback>(isolate, "napi_make_callback");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Native<napi_async_context>, Wasm<napi_callback_scope *>>
    ::bind<napi_open_callback_scope>(isolate, "napi_open_callback_scope");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_callback_scope>>
    ::bind<napi_close_callback_scope>(isolate, "napi_close_callback_scope");
  // TODO(ohadrau): Copy the node version struct
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<const napi_node_version **>>
    ::bind<napi_get_node_version>(isolate, "napi_get_node_version");
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<uint32_t *>>
    ::bind<napi_get_version>(isolate, "napi_get_version");
  napiBind<Native<napi_status>, Native<napi_env>, Native<int64_t>, Wasm<int64_t *>>
    ::bind<napi_adjust_external_memory>(isolate, "napi_adjust_external_memory");
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<napi_deferred *>, Wasm<napi_value *>>
    ::bind<napi_create_promise>(isolate, "napi_create_promise");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_deferred>, Native<napi_value>>
    ::bind<napi_resolve_deferred>(isolate, "napi_resolve_deferred");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_deferred>, Native<napi_value>>
    ::bind<napi_reject_deferred>(isolate, "napi_reject_deferred");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<bool *>>
    ::bind<napi_is_promise>(isolate, "napi_is_promise");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Wasm<napi_value *>>
    ::bind<napi_run_script>(isolate, "napi_run_script");
  // TODO(ohadrau): What kind of copying has to be done here?
  napiBind<Native<napi_status>, Native<napi_env>, Wasm<uv_loop_t **>>
    ::bind<napi_get_uv_event_loop>(isolate, "napi_get_uv_event_loop");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_value>, Native<napi_value>, Native<napi_value>, Native<size_t>, Native<size_t>, Wasm<void *>, Wasm<napi_finalize>, Wasm<void *>, Wasm<napi_threadsafe_function_call_js>, Wasm<napi_threadsafe_function *>>
    ::bind<napi_create_threadsafe_function>(isolate, "napi_create_threadsafe_function");
  napiBind<Native<napi_status>, Native<napi_threadsafe_function>, Wasm<void **>>
    ::bind<napi_get_threadsafe_function_context>(isolate, "napi_get_threadsafe_function_context");
  napiBind<Native<napi_status>, Native<napi_threadsafe_function>, Wasm<void *>, Native<napi_threadsafe_function_call_mode>>
    ::bind<napi_call_threadsafe_function>(isolate, "napi_call_threadsafe_function");
  napiBind<Native<napi_status>, Native<napi_threadsafe_function>>
    ::bind<napi_acquire_threadsafe_function>(isolate, "napi_acquire_threadsafe_function");
  napiBind<Native<napi_status>, Native<napi_threadsafe_function>, Native<napi_threadsafe_function_release_mode>>
    ::bind<napi_release_threadsafe_function>(isolate, "napi_release_threadsafe_function");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_threadsafe_function>>
    ::bind<napi_ref_threadsafe_function>(isolate, "napi_ref_threadsafe_function");
  napiBind<Native<napi_status>, Native<napi_env>, Native<napi_threadsafe_function>>
    ::bind<napi_unref_threadsafe_function>(isolate, "napi_unref_threadsafe_function");
  // Extra functions to help write our overloads:
  napiBind<Native<size_t>, Native<const char*>>
    ::bind<get_string_length>(isolate, "get_string_length");
  napiBind<Native<void>, Native<const char*>, Wasm<char*>>
    ::bind<copy_string_to_wasm>(isolate, "copy_string_to_wasm");

  napiBind<Native<napi_property_descriptor*>, Native<size_t>>
    ::bind<create_property_descriptors>(isolate, "create_property_descriptors");
  napiBind<Native<void>, Native<napi_property_descriptor*>, Native<int>, Wasm<const char*>>
    ::bind<set_napi_property_descriptor_utf8name>(isolate, "set_napi_property_descriptor_utf8name");
  napiBind<Native<void>, Native<napi_property_descriptor*>, Native<int>, Native<napi_value>>
    ::bind<set_napi_property_descriptor_name>(isolate, "set_napi_property_descriptor_name");
  napiBind<Native<void>, Native<napi_property_descriptor*>, Native<int>, Wasm<napi_callback>>
    ::bind<set_napi_property_descriptor_method>(isolate, "set_napi_property_descriptor_method");
  napiBind<Native<void>, Native<napi_property_descriptor*>, Native<int>, Wasm<napi_callback>>
    ::bind<set_napi_property_descriptor_getter>(isolate, "set_napi_property_descriptor_getter");
  napiBind<Native<void>, Native<napi_property_descriptor*>, Native<int>, Wasm<napi_callback>>
    ::bind<set_napi_property_descriptor_setter>(isolate, "set_napi_property_descriptor_setter");
  napiBind<Native<void>, Native<napi_property_descriptor*>, Native<int>, Native<napi_value>>
    ::bind<set_napi_property_descriptor_value>(isolate, "set_napi_property_descriptor_value");
  napiBind<Native<void>, Native<napi_property_descriptor*>, Native<int>, Native<napi_property_attributes>>
    ::bind<set_napi_property_descriptor_attributes>(isolate, "set_napi_property_descriptor_attributes");
  // TODO(ohadrau): Is void* really gonna be a WASM pointer?
  napiBind<Native<void>, Native<napi_property_descriptor*>, Native<int>, Wasm<void*>>
    ::bind<set_napi_property_descriptor_data>(isolate, "set_napi_property_descriptor_data");

  napiBind<Native<napi_extended_error_info*>>
    ::bind<create_napi_extended_error_info>(isolate, "create_napi_extended_error_info");
  napiBind<Wasm<const char*>, Native<napi_extended_error_info*>>
    ::bind<napi_extended_error_info_error_message>(isolate, "napi_extended_error_info_error_message");
  napiBind<Native<void*>, Native<napi_extended_error_info*>>
    ::bind<napi_extended_error_info_engine_reserved>(isolate, "napi_extended_error_info_engine_reserved");
  napiBind<Native<uint32_t>, Native<napi_extended_error_info*>>
    ::bind<napi_extended_error_info_engine_error_code>(isolate, "napi_extended_error_info_engine_error_code");
  napiBind<Native<napi_status>, Native<napi_extended_error_info*>>
    ::bind<napi_extended_error_info_error_code>(isolate, "napi_extended_error_info_error_code");
}

}  // namespace wasm_napi
}  // namespace node