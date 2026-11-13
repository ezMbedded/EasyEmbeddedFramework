============================================================
Operating System Abstraction Layer (OSAL)
============================================================

Introduction
============================
This document describes the architecture of the Operating System Abstraction Layer (OSAL) component.
OSAL provides a unified API for task, semaphore, timer, and event management across different RTOSes (e.g., FreeRTOS, ThreadX).
It allows application code to be portable and independent of the underlying OS.
OSAL does not implement OS primitives itself, but delegates to the selected RTOS backend.

Use cases:

- Writing portable embedded applications across multiple RTOSes
- Simplifying migration between FreeRTOS, ThreadX, or other supported OSes
- Abstracting OS-specific details from application logic

Component's structure
============================
The OSAL component is composed of:
- **ezOsal_TaskHandle_t**: Task handle abstraction
- **ezOsal_SemaphoreHandle_t**: Semaphore handle abstraction
- **ezOsal_TimerHandle_t**: Timer handle abstraction
- **ezOsal_EventHandle_t**: Event group handle abstraction
- **ezOsal_Interfaces_t**: Function pointers for OS operations (init, create, delete, etc.)
- **Backend implementations**: FreeRTOS and ThreadX specific files provide actual OS calls

.. mermaid::
   :align: center

   graph TD
       A[Application Code]
       B[ez_osal.h API]
       C[ezOsal_Interfaces_t]
       D1[ez_osal_freertos.c/h]
       D2[ez_osal_threadx.c/h]
       A --> B
       B --> C
       C --> D1
       C --> D2

Component's behavior
============================

External Behavior
-----------------
- Application calls OSAL API (e.g., create task, semaphore, timer, event)
- OSAL dispatches calls to the selected backend (FreeRTOS or ThreadX)
- Backend performs the requested OS operation and returns status

Internal Behavior
-----------------
- OSAL uses a function pointer table (`ezOsal_Interfaces_t`) to abstract backend implementations
- Backend is set at initialization (via `ezOsal_SetInterface`)
- All OSAL API calls check if the interface is set and implemented before dispatching

Component's data type
============================
- **ezOsal_TaskHandle_t**: Structure with task name, priority, stack size, function pointer, argument, handle, static resource
- **ezOsal_SemaphoreHandle_t**: Structure with max count, static resource, handle
- **ezOsal_TimerHandle_t**: Structure with timer name, period, callback, argument, handle, static resource
- **ezOsal_EventHandle_t**: Structure with handle, static resource
- **ezOsal_Interfaces_t**: Struct of function pointers for all supported OS operations

Component's Data flow
============================
.. mermaid::
   :align: center

   flowchart TD
       App[Application]
       OSAL[OSAL API]
       IF[ezOsal_Interfaces_t]
       RTOS[RTOS Backend FreeRTOS/ThreadX]
       App --> OSAL
       OSAL --> IF
       IF --> RTOS
       RTOS --> IF
       IF --> OSAL
       OSAL --> App