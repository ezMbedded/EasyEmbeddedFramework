============================================================
Assert
============================================================

Introduction
============================
The Assert module provides runtime assertion checks for embedded applications. It helps developers catch programming errors by halting execution when a condition fails, printing the function name, line number, and an optional message. This module is designed for debugging and error detection during development and testing. It is not intended for production error handling or recovery.

Use cases:
 - Detect invalid states or unexpected values during development
 - Debug logic errors by providing context (function, line, message)
 - Prevent silent failures by halting execution on assertion failure

Limitations:
 - Not suitable for graceful error handling
 - Halts execution (infinite loop) on failure
 - Requires logging/printing support

Component's structure
============================
The Assert module consists of:
 - Preprocessor macros for assertion checks:
   - ASSERT(expr): Basic assertion
   - ASSERT_MSG(expr, msg): Assertion with custom message
   - ASSERT_CUST_MSG(expr, fmt, ...): Assertion with formatted message
 - Integration with logging (dbg_print/printf)
 - Conditional compilation based on EZ_ASSERT macro

Mermaid diagram:

.. mermaid::
   graph TD
     A[Caller code] -->|assert macro| B{Assertion check}
     B -->|expr true| C[Continue execution]
     B -->|expr false| D[Print debug info]
     D --> E[Halt (while(1))]

Component's behavior
============================
External behavior:
 - When an assertion macro is used, the expression is evaluated at runtime.
 - If the expression is true, execution continues normally.
 - If the expression is false, debug information is printed (function, line, message), and execution halts in an infinite loop.

Internal behavior:
 - Assertion macros expand to conditional checks and logging calls.
 - Uses dbg_print or printf for output.
 - Infinite loop prevents further execution after failure.

Component's data type
============================
No specific data types are defined. The module operates via macros and uses standard C types for expressions and messages.
