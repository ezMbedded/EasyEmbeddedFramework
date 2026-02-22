EasyEmbedded Framework Architecture
==================================================

Introduction
############
This document describes the architecture of the EasyEmbedded Frameworks, including
its components and their interactions.

System Context
##############

From the architecture point of view, the EasyEmbedded Framework acts as glue code between the
application, which handles the business logic, and the hardware-dependent code. The framework
gives the advantage of modularity, where change in the application or hardware does not affect
the counter parts.

The framework is devided into four blocks:

- Service: provides common services for the application, such as command line interface, data model or event bus.
- Middleware: provides common interface to interact with the underlying RTOS, file system or network stack. 
- HAL: provides common interface to interact with hardware
- Utilities: provides common utilities such as logging, assert, data structure, etc.

.. image:: _resources/architecture/ezArchitecture_system.svg
   :align: center
   :alt: EasyEmbedded Framework Architecture - System architecture

The functionality of each block will be described in the following sections.

System Containers
#################

As mentioned in the overview, the framework is divided into four blocks/layers stacking on top of each other:

.. image:: _resources/architecture/ezArchitecture_container.svg
   :align: center
   :alt: EasyEmbedded Framework Architecture - System container

Service block
~~~~~~~~~~~~~~

Service block locates on the top of the stack and provides the common services
for the application to implement its business logic. Service block includes:

- `Command line interface (CLI) <easy_embedded/service/cli/cli.html>`_: simple way for user to interact with the system.
- `Data model <easy_embedded/service/data_model/data_model.html>`_: a way to manage data in key-value pair manner.
- `Event bus <easy_embedded/service/event_bus/event_bus.html>`_: a way for tasks, processes to communicate with each other in decoupled way using pub/sub pattern.
- `State machine <easy_embedded/service/state_machine/state_machine.html>`_: a implementation of hierarchical state machine. This service ensures that every state machine
  is implmeneted in a consistent way.
- `Remote procedure call (RPC) <easy_embedded/service/rpc/rpc.html>`_: a way for client communicate with the server (device) in request-respoinse manner.
- `Inter-process communication (IPC) <easy_embedded/service/ipc/ipc.html>`_: a way for processes to communicate with each other in decoupled way using mail boxes.


Middlewares block
~~~~~~~~~~~~~~~~~

Middlewares block provides the following functionalities:

- `OSAL <easy_embedded/middleware/osal/osal.html>`_: a unified API for task, semaphore, timer, and event management across different RTOSes (e.g., FreeRTOS, ThreadX).
- `File system abstraction layer <easy_embedded/middleware/fs_abstraction/fs_abstraction.html>`_: a unified API for file system operations across different file systems (TBD).
- `Network stack abstraction layer <easy_embedded/middleware/network_stack_abstraction/network_stack_abstraction.html>`_: a unified API for network operations across different network stacks (TBD, expected lwIP).


HAL block
~~~~~~~~~~~~~~

HAL provides unified API to interact with the hardware, e.g. GPIO, I2C, SPI, ADC, etc. 
The HAL only spcifies the API, the actual implementation must be provided by the user.


Utilities block
~~~~~~~~~~~~~~~~
Utilities provides basic infrastructure for the system such as:

- `Logging <easy_embedded/utility/logging/logging.html>`_: a simple logging library with different log levels and output options.
- `Assert <easy_embedded/utility/assert/assert.html>`_: a simple assert library with different assert levels and output options.
- `Linked list <easy_embedded/utility/linked_list/linked_list.html>`_: Doubly cicular linked list data structure
- `Ring buffer <easy_embedded/utility/ring_buffer/ring_buffer.html>`_: a simple ring buffer implementation for byte stream data.
- `Queue <easy_embedded/utility/queue/queue.html>`_: a simple queue implementation for generic data.
- `Static alloc <easy_embedded/utility/static_alloc/static_alloc.html>`_: a simple static memory allocator for fixed-size memory blocks.


System Components
#################

Following figure describes detail the components in the framework and their interactions.
Basically, if a component is depends on a changable component, e.g. RTOS, network stack, or 
hardware-dependent code, it should not call that component directly but through an interface.
The user must implement the interface (in form of function pointers).

.. image:: _resources/architecture/ezArchitecture_component.svg
   :align: center
   :alt: EasyEmbedded Framework Architecture - System component
