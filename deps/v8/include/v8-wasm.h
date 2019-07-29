// Copyright 2019 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_V8_WASM_H_
#define INCLUDE_V8_WASM_H_

#include <cassert>
#include <cstdint>
#include <cstddef>
#include "v8.h"

namespace v8 {
namespace wasm {

#define PAGE_SIZE 0x10000

class V8_EXPORT Memory {
 private:
  size_t pages_;
  uint8_t* data_;
 public:
  Memory(size_t pages, uint8_t* data);
  Memory(const Memory& memory);
  size_t size();
  size_t pages();
  uint8_t* data();
};

class V8_EXPORT Table {
 private:
  void* tableObject_;
 public:
  Table(void* tableObject);
  Table(const Table& table);
  ~Table();
  MaybeLocal<Function> get(int index) const;
};

class V8_EXPORT Context {
 public:
  Context(Memory* memory, Table* table);
  Context(Memory* memory, Table* table, v8::Isolate* isolate);
  Context(const Context& context);
  ~Context();
  Memory* memory;
  Table* table;
  v8::Isolate* isolate;
};

enum V8_EXPORT ValKind : uint8_t {
  I32,
  I64,
  F32,
  F64,
  ANYREF = 128,
  FUNCREF
};

// TODO(ohadrau): Should we enforce Ref ownership like the C API?
class V8_EXPORT Val {
 private:
  ValKind kind_;
  union value {
    int32_t i32;
    int64_t i64;
    float f32;
    double f64;
    void* ref;
  } value_;

  Val(ValKind kind, value value);

 public:
  Val();
  Val(Val&& val);
  explicit Val(int32_t i);
  explicit Val(int64_t i);
  explicit Val(float i);
  explicit Val(double i);
  explicit Val(void* r);

  Val& operator=(Val&&);

  ValKind kind() const;
  int32_t i32() const;
  int64_t i64() const;
  float f32() const;
  double f64() const;
  void* ref() const;
};

class V8_EXPORT FuncType {
 private:
  std::vector<ValKind> params_, results_;
 public:
  FuncType(std::vector<ValKind> params, std::vector<ValKind> results);

  std::vector<ValKind> params() const;
  std::vector<ValKind> results() const;
};

class V8_EXPORT Func {
 public:
  using callbackType = void (*)(const Context*, const Val[], Val[]);
  Func(const FuncType, callbackType);

  const FuncType type();
  callbackType callback();
 private:
  const FuncType type_;
  callbackType callback_;
};

void RegisterEmbedderBuiltin(Isolate* isolate,
                             const char* module_name,
                             const char* name,
                             Func import);

}  // namespace wasm
}  // namespace v8

#endif  // INCLUDE_V8_WASM_H_
