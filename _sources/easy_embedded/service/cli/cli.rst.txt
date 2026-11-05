============================================================
Command Line Interface (CLI)
============================================================

Introduction
============================
This document describes the architecture of the Command Line Interface (CLI) component.  
The CLI provides a simple, extensible interface for interacting with embedded applications via text commands.  
It parses commands, arguments, and values, dispatches them to registered handlers, and returns responses.  
The CLI is not responsible for transport (UART, USB, etc.) or command implementation logic.

Use cases:

- Debugging and testing embedded systems
- Configuring device parameters at runtime
- Providing a user interface for headless devices

Component Structure
============================
The CLI component is composed of the following sub-components:

- **ezCli_t**: Main CLI instance, holds state, buffers, and interfaces.
- **ezCliCmdList_t**: List of supported commands.
- **ezCliCommand_t**: Definition of a single command, its arguments, and callback.
- **ezCliBuff_t**: Buffer for incoming command text.
- **ezCliInterface_t**: Abstraction for receiving and sending characters.

.. mermaid::
   :align: center

   graph TD
      A[ezCli_t (CLI Instance)]
      B[ezCliCmdList_t (Command List)]
      C[ezCliCommand_t (Command)]
      D[ezCliBuff_t (Buffer)]
      E[ezCliInterface_t (I/O Interface)]
      A --> B
      B --> C
      A --> D
      A --> E

Component Behavior
============================

External Behavior
-----------------
- Receives characters via `GetCharCallback`.
- Parses input into commands, arguments, and values.
- Matches input to registered commands.
- Executes the corresponding callback.
- Sends responses via `SendCharsCallback`.

Internal Behavior
-----------------
The CLI uses a state machine with the following states:

- **STATE_COMMAND**: Parsing the command name.
- **STATE_ARGUMENT**: Parsing command arguments.
- **STATE_VALUE**: Parsing argument values (supports quoted strings).
- **STATE_ERROR**: Handles errors and resets state.

Transitions occur based on received characters (space, newline, etc.) and parsing progress.

.. mermaid::
   :align: center

   stateDiagram-v2
      [*] --> STATE_COMMAND : Reset/Start
      STATE_COMMAND --> STATE_ARGUMENT : Command with argument detected
      STATE_COMMAND --> STATE_ERROR : Unknown command / Error
      STATE_COMMAND --> STATE_COMMAND : Command without argument (loop)
      STATE_ARGUMENT --> STATE_VALUE : Argument with value detected
      STATE_ARGUMENT --> STATE_COMMAND : End of line (no value)
      STATE_ARGUMENT --> STATE_ERROR : Invalid argument
      STATE_VALUE --> STATE_ARGUMENT : Value parsed, next argument
      STATE_VALUE --> STATE_COMMAND : End of line (all values parsed)
      STATE_VALUE --> STATE_ERROR : Value parse error
      STATE_ERROR --> STATE_COMMAND : On reset/newline

Component Data Types
============================
- **ezCli_t**: Main CLI context (state, buffers, command list, etc.)
- **ezCliCmdList_t**: Array of `ezCliCommand_t` and count.
- **ezCliCommand_t**: Command name, description, argument lists, callback.
- **ezCliBuff_t**: Buffer pointer, size, current index.
- **ezCliInterface_t**: Function pointers for input/output.
- **CLI_NOTIFY_CODE**: Return codes for command execution (OK, ERR, BAD_ARG).
- **CLI_STATE**: State machine states.
