# Easy Embedded Framework (ez): LLM Guide

> Read this entire file before writing code. Keep code consistent with it.

## 1. What this framework is

This framework provides utilities (logging), data structures (linked list, queue, ring buffer), design patterns (event bus, state machine), and a platform to bundle components together so that the application firmware can  running directly on top without having to touch the lower layer components. This framework is designed for resource-constrained devices (MCU) using RTOS or running bare metal.

## 2. Hard rules (never violate)

1. No dynamic allocation (`malloc`/`free`/`new`) anywhere.
2. Never call a function from an ISR unless its docs say `ISR-safe: yes`.
   Use the `FromISR` variant.
3. Never block (waits, delays, unbounded loops) in an ISR or while holding a lock.
4. Every return value must be checked. Never cast it to `(void)`.
5. Never access container internals directly. Use only the public API in `inc/<layer>/<module>/`
   (layers: see section 3).
6. Do not invent APIs. If a function is not listed in section 5, it does not exist.
   Ask or propose an addition instead.

## 3. Layers

Headers live in `inc/<layer>/<module>/`. A layer may only include headers from layers below it.

| Layer         | Purpose                                                                   | May depend on                  |
|---------------|---------------------------------------------------------------------------|--------------------------------|
| `platform`    | C++ glue bundling services (event-driven task, sync, platform entry)      | `service`, `utilities`         |
| `service`     | cli, data_model, event_bus, ipc, rpc, state_machine, task_worker          | `middlewares`, `utilities`     |
| `middlewares` | `osal`: unified RTOS API (FreeRTOS, ThreadX)                              | `utilities`                    |
| `hal`         | `gpio`, `i2c`, `spi`, `uart`: API only, implementation is supplied by user | `utilities`                    |
| `utilities`   | assert, logging, linked_list, queue, ring_buffer, static_alloc, ...       | nothing                        |

## 4. Conventions

- **Naming:** `ez<Module>_<Verb><Noun>`, types `ez<Module>_t`, macros `EZ_<MODULE>_*`.
- **Follow the module's existing verbs**.
- **Ownership:** containers copy items in and out unless the doc says "by reference".
  Caller owns all memory passed to `Init*` or `Initialize*` .
- **Context suffix:** `FromISR` = callable from interrupt context only.
- Blocking API shall have the suffix `Sync`, or `ez<Module>_<Verb><Noun><Sync>`. e.g. `ezSpi_ReceiveSync`
- Non-Blocking API shall have the suffix `Async`, or `ez<Module>_<Verb><Noun><Async>`. e.g. `ezSpi_ReceiveAsync`

## 5. API quick reference

One full cheat sheet per module: types and macros, then every function with its
signature, description, parameters, return values, pre/post conditions, warnings,
`ISR-safe` / `Blocking` flags and an example. Generated from Doxygen; do not hand-edit.
- `docs_llm/api/<module>.md`: one module
- `docs_llm/api/INDEX.md`: module table, read this first to find the module

## 6. Where to look

- Examples (compile and run on host): `samples/`
- Recipes: `docs_llm/recipes/`
- Tests as usage reference: `tests/`
- Doxygen document in every header files in: `inc/<layer>/<module>/*.h`

## 7. Workflow for LLM-generated code

1. Identify needed modules; read their cheat sheets in `docs_llm/api/`.
2. Start from the closest example in `samples/` or check the corresponding tests in `tests/`.
3. Build using the CMake preset `linux_test_debug` in `CMakePresets.json`
4. Run `cmake --preset linux_test_debug`, `cmake --build --preset linux_test_debug`, `ctest --preset=linux_test_debug`; and code coverage with `cmake --build --preset linux_test_debug --target coverage`
5. Self-check against section 2 before finishing.

## 8. Adding to or changing the framework

- Don't rename public symbols (stability matters).
- New API must follow section 4 and ship with Doxygen, an example, and a unit test.
- Update this guide.
