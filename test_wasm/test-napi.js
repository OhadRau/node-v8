const fs = require('fs')
const buf = fs.readFileSync('./test_wasm_napi.node.wasm')
const mod = new WebAssembly.Module(buf)

let imports = WebAssembly.embedderBuiltins()
imports["wasi_unstable"] = {}
imports["wasi_unstable"]["fd_close"] = function(m) { console.log('fd_close', m) }
imports["wasi_unstable"]["fd_write"] = function(m) { /*console.log('fd_write', m)*/ }
imports["wasi_unstable"]["fd_seek"] = function(m) { console.log('fd_seek', m) }
imports["wasi_unstable"]["fd_fdstat_get"] = function(m) { console.log('fd_fdstat_get', m) }

imports["env"] = {}
imports["env"]["memory"] = new WebAssembly.Memory({initial:8})
imports["env"]["__indirect_function_table"] = new WebAssembly.Table({initial:7, element:'anyfunc'})

const inst = new WebAssembly.Instance(mod, imports)
console.log('Result: ' + inst.exports.main())

const test_module = require("./test_wasm_napi")
