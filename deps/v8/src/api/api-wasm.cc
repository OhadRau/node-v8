// Copyright 2019 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/api/api-wasm.h"
#include "src/base/memory.h"
#include "src/common/globals.h"
#include "src/api/api-inl.h"
#include "src/objects/contexts.h"
#include "src/objects/lookup.h"
#include "src/execution/isolate.h"

#define IGNORE(X) ((void) (X))

namespace b = v8::base;
namespace i = v8::internal;

namespace v8 {
namespace wasm {

Memory::Memory(size_t pages, uint8_t* data) : pages_(pages), data_(data) {}
Memory::Memory(const Memory& memory)
    : pages_(memory.pages_), data_(memory.data_) {}
size_t Memory::size() { return pages_ * PAGE_SIZE; }
size_t Memory::pages() { return pages_; }
uint8_t* Memory::data() { return data_; }

Table::Table(void* tableObject) {
  i::WasmTableObject* actualTable =
      reinterpret_cast<i::WasmTableObject*>(tableObject);
  tableObject_ = (void*) new i::WasmTableObject(*actualTable);
}
Table::Table(const Table& table) {
  i::WasmTableObject* actualTable =
      reinterpret_cast<i::WasmTableObject*>(table.tableObject_);
  tableObject_ = (void*) new i::WasmTableObject(*actualTable);
}
Table::~Table() { delete reinterpret_cast<i::WasmTableObject*>(tableObject_); }
MaybeLocal<Function> Table::get(int index) const {
  i::Isolate* i_isolate = i::Isolate::TryGetCurrent();
  i::WasmTableObject* table = (i::WasmTableObject*) tableObject_;
  i::Handle<i::WasmTableObject> tableHandle =
      i::Handle<i::WasmTableObject>(*table, i_isolate);
  i::Handle<i::Object> entry =
      i::WasmTableObject::Get(i_isolate, tableHandle, index);

  // TODO(ohadrau): Assumes this is a FUNCREF table
  if (i::WasmExportedFunction::IsWasmExportedFunction(*entry) ||
      i::WasmCapiFunction::IsWasmCapiFunction(*entry)) {
    return Utils::ToLocal(i::Handle<i::JSFunction>::cast(entry));
  } else {
    return MaybeLocal<Function>();
  }
}

Context::Context(Memory* memory, Table* table)
    : isolate(Isolate::GetCurrent()), memory(memory), table(table) {}
Context::Context(Memory* memory, Table* table, Isolate* isolate)
    : isolate(isolate), memory(memory), table(table) {}
Context::Context(const Context& context)
    : isolate(context.isolate) {
  this->memory = new Memory(*context.memory);
  this->table = new Table(*context.table);
}
Context::~Context() {
  delete this->memory;
  delete this->table;
}

Val::Val(ValKind kind, value value) : kind_(kind), value_(value) {}

Val::Val() : kind_(ANYREF) { value_.ref = nullptr; }
Val::Val(Val&& val) : Val(val.kind_, val.value_) {}

Val::Val(int32_t i) : kind_(I32) { value_.i32 = i; }
Val::Val(int64_t i) : kind_(I64) { value_.i64 = i; }
Val::Val(float i) : kind_(F32) { value_.f32 = i; }
Val::Val(double i) : kind_(F64) { value_.f64 = i; }
Val::Val(void* r) : kind_(ANYREF) { value_.ref = r; }

Val& Val::operator=(Val&& val) {
  kind_ = val.kind_;
  value_ = val.value_;
  return *this;
}

ValKind Val::kind() const { return kind_; }
int32_t Val::i32() const { assert(kind_ == I32); return value_.i32; }
int64_t Val::i64() const { assert(kind_ == I64); return value_.i64; }
float Val::f32() const { assert(kind_ == F32); return value_.f32; }
double Val::f64() const { assert(kind_ == F64); return value_.f64; }
void* Val::ref() const {
  assert(kind_ == ANYREF || kind_ == FUNCREF);
  return value_.ref;
}

FuncType::FuncType(
  std::vector<ValKind> params, std::vector<ValKind> results
) : params_(params), results_(results) {}

std::vector<ValKind> FuncType::params() const { return params_; }
std::vector<ValKind> FuncType::results() const { return results_; }

Func::Func(const FuncType funcType, callbackType cb) :
  type_(funcType),
  callback_(cb) {}

const FuncType Func::type() {
  return type_;
}

Func::callbackType Func::callback() {
  return callback_;
}

i::wasm::ValueType valkind_to_v8(ValKind type) {
  switch (type) {
    case I32:
      return i::wasm::kWasmI32;
    case I64:
      return i::wasm::kWasmI64;
    case F32:
      return i::wasm::kWasmF32;
    case F64:
      return i::wasm::kWasmF64;
    default:
      // TODO(wasm+): support new value types
      UNREACHABLE();
  }
}

// TODO(ohadrau): Clean up so that we're not maintaining 2 copies of this
// Taken from c-api.cc -- SignatureHelper::Serialize
// Use an invalid type as a marker separating params and results.
const i::wasm::ValueType kMarker = i::wasm::kWasmStmt;

i::Handle<i::PodArray<i::wasm::ValueType>> Serialize(
    i::Isolate* isolate, const FuncType type) {
  int sig_size =
      static_cast<int>(type.params().size() + type.results().size() + 1);
  i::Handle<i::PodArray<i::wasm::ValueType>> sig =
      i::PodArray<i::wasm::ValueType>::New(isolate, sig_size,
                                           i::AllocationType::kOld);
  int index = 0;
  // TODO(jkummerow): Consider making vec<> range-based for-iterable.
  for (size_t i = 0; i < type.results().size(); i++) {
    sig->set(index++, valkind_to_v8(type.results()[i]));
  }
  // {sig->set} needs to take the address of its second parameter,
  // so we can't pass in the static const kMarker directly.
  i::wasm::ValueType marker = kMarker;
  sig->set(index++, marker);
  for (size_t i = 0; i < type.params().size(); i++) {
    sig->set(index++, valkind_to_v8(type.params()[i]));
  }
  return sig;
}

FuncData::FuncData(i::Isolate* isolate, const FuncType type)
    : isolate(isolate) {
  param_count = type.params().size();
  result_count = type.results().size();
  serialized_sig = *Serialize(isolate, type);
}

i::Address FuncData::v8_callback(
  void* data, size_t memoryPages, uint8_t* memoryBase,
  i::WasmTableObject table, i::Address argv
) {
  FuncData* self = reinterpret_cast<FuncData*>(data);
  i::Isolate* isolate = self->isolate;
  i::HandleScope scope(isolate);

  int num_param_types = self->param_count;
  int num_result_types = self->result_count;

  std::unique_ptr<Val[]> params(new Val[num_param_types]);
  std::unique_ptr<Val[]> results(new Val[num_result_types]);
  i::Address p = argv;
  for (int i = 0; i < num_param_types; ++i) {
    switch (self->serialized_sig.get(i + num_result_types + 1)) {
      case i::wasm::kWasmI32:
        params[i] = Val(b::ReadUnalignedValue<int32_t>(p));
        p += 4;
        break;
      case i::wasm::kWasmI64:
        params[i] = Val(b::ReadUnalignedValue<int64_t>(p));
        p += 8;
        break;
      case i::wasm::kWasmF32:
        params[i] = Val(b::ReadUnalignedValue<float>(p));
        p += 4;
        break;
      case i::wasm::kWasmF64:
        params[i] = Val(b::ReadUnalignedValue<double>(p));
        p += 8;
        break;
      case i::wasm::kWasmAnyRef:
      case i::wasm::kWasmAnyFunc: {
        i::Address raw = b::ReadUnalignedValue<i::Address>(p);
        p += sizeof(raw);
        if (raw == i::kNullAddress) {
          params[i] = Val(nullptr);
        } else {
          params[i] = Val(reinterpret_cast<void*>(raw));
        }
        break;
      }
    }
  }

  Memory* memory = new Memory(memoryPages, memoryBase);
  Table* apiTable = new Table(&table);
  const Context* context = new Context(memory, apiTable);

  self->callback(context, params.get(), results.get());

  delete context;

  if (isolate->has_scheduled_exception()) {
    isolate->PromoteScheduledException();
  }
  if (isolate->has_pending_exception()) {
    i::Object ex = isolate->pending_exception();
    isolate->clear_pending_exception();
    return ex.ptr();
  }

  p = argv;
  for (int i = 0; i < num_result_types; ++i) {
    switch (self->serialized_sig.get(i)) {
      case i::wasm::kWasmI32:
        b::WriteUnalignedValue(p, results[i].i32());
        p += 4;
        break;
      case i::wasm::kWasmI64:
        b::WriteUnalignedValue(p, results[i].i64());
        p += 8;
        break;
      case i::wasm::kWasmF32:
        b::WriteUnalignedValue(p, results[i].f32());
        p += 4;
        break;
      case i::wasm::kWasmF64:
        b::WriteUnalignedValue(p, results[i].f64());
        p += 8;
        break;
      case i::wasm::kWasmAnyRef:
      case i::wasm::kWasmAnyFunc: {
        if (results[i].ref() == nullptr) {
          b::WriteUnalignedValue(p, i::kNullAddress);
        } else {
          b::WriteUnalignedValue(p, results[i].ref());
        }
        p += sizeof(i::Address);
        break;
      }
    }
  }
  return i::kNullAddress;
}

void RegisterEmbedderBuiltin(Isolate* isolate,
                             const char* module_name,
                             const char* name,
                             Func import) {
  i::Isolate* i_isolate = reinterpret_cast<i::Isolate*>(isolate);
  i::HandleScope handle_scope(i_isolate);

  Eternal<i::JSObject> imports = i_isolate->wasm_native_imports();
  if (imports.IsEmpty()) {
    i::Handle<i::JSObject> handle =
        i_isolate->factory()->NewJSObject(i_isolate->object_function());
    Local<i::JSObject> local =
        Utils::Convert<i::JSObject, i::JSObject>(handle);
    imports.Set(isolate, local);
  }

  Local<i::JSObject> imports_local = imports.Get(isolate);
  i::Handle<i::JSObject> imports_handle =
      i::Handle<i::JSObject>(reinterpret_cast<i::Address*>(*imports_local));

  i::Handle<i::String> module_str =
      i_isolate->factory()->NewStringFromAsciiChecked(module_name);
  i::Handle<i::String> name_str =
      i_isolate->factory()->NewStringFromAsciiChecked(name);

  i::Handle<i::JSObject> module_obj;
  i::LookupIterator module_it(i_isolate, imports_handle, module_str,
                              i::LookupIterator::OWN_SKIP_INTERCEPTOR);
  if (i::JSObject::HasProperty(&module_it).ToChecked()) {
    module_obj = i::Handle<i::JSObject>::cast(
        i::Object::GetProperty(&module_it).ToHandleChecked());
  } else {
    module_obj =
        i_isolate->factory()->NewJSObject(i_isolate->object_function());
    IGNORE(i::Object::SetProperty(i_isolate, imports_handle,
                                  module_str, module_obj));
  }

  FuncData* data = new FuncData(i_isolate, import.type());
  data->callback = import.callback();
  i::Handle<i::PodArray<i::wasm::ValueType>> serialized_sig(
      data->serialized_sig, i_isolate);
  i::Handle<i::WasmCapiFunction> callback =
      i::WasmCapiFunction::New(
        i_isolate, reinterpret_cast<i::Address>(&FuncData::v8_callback),
        data, serialized_sig);
  IGNORE(i::Object::SetProperty(i_isolate, module_obj, name_str, callback));

  i_isolate->set_wasm_native_imports(imports);
}

}  // namespace wasm
}  // namespace v8

#undef IGNORE
