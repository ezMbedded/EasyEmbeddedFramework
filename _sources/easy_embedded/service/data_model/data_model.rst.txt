============================================================
Data Model
============================================================
Introduction
============================
The `Data Model` component provides a lightweight, in-memory, indexed key/value
store with change-tracking (dirty flags) and simple accessors. It is
designed for embedded systems that need a compact representation of a set of
typed data points, efficient reads/writes, and a mechanism to detect which
values have changed so they can be synchronized or published by other
subsystems.

Important: The `Data Model` does not provide event notification or callback
mechanisms when data changes. It only marks data points as dirty. If your
system requires active notifications or pub/sub behaviour you should compose
the data model with the `Event Bus` component (see
``easy_embedded/service/event_bus/event_bus.rst``) or another notification
mechanism to publish changes.

This document describes the component's structure, runtime behaviour,
data flow, and the public data types and API. The primary implementation can
be found in the `ez_data_model.h` / `ez_data_model.c` pair and exposes functions
such as ``ezDataModel_Initialize``, ``ezDataModel_SetDataPoint``,
``ezDataModel_GetDataPoint``, ``ezDataModel_GetFirstDirty``,
``ezDataModel_ClearDirtyFlags``, and ``ezDataModel_ClearAllDirtyFlags``.

Component's structure
============================
The `Data Model` is composed of three logical parts:

- A statically-provided array of ``ezDataPoint_t`` entries that define the
  index and size of each data point.
- A contiguous memory buffer used to store the actual data bytes for all
  data points (the data model buffer).
- The `ezDataModel_t` descriptor which references the array and buffer and
  provides helper operations.

.. mermaid::

    classDiagram
        class ezDataModel_t {
            +ezDataPoint_t[] data_points
      +DataBuffer data_model_buff
            +size_t num_of_data_points
        }
        class ezDataPoint_t {
            +uint32_t index
            +size_t size
            +void* data
            +bool isDirty
        }

    class DataBuffer {
      +uint8_t[] bytes
      +size_t size
    }

    ezDataModel_t o-- ezDataPoint_t : manages
    ezDataModel_t *-- DataBuffer : contains

Component's behavior
============================
External behaviour
------------------

1. Initialization: The application allocates an array of ``ezDataPoint_t``
   (each describing an index and size) and a contiguous buffer. It calls
   ``ezDataModel_Initialize`` passing the `ezDataModel_t` descriptor, the
   points array, and the buffer. The initializer zeroes the buffer, partitions
   it, assigns each point's ``data`` pointer into that buffer, clears all dirty
   flags, and returns ``true`` on success or ``false`` if the configuration is
   invalid.
2. Setting values: ``ezDataModel_SetDataPoint`` takes the data and its size in
   bytes. The size must equal the size of the data point, otherwise
   ``ezSTATUS_ARG_INVALID`` is returned and nothing is written. On success it
   copies the bytes into the pre-assigned buffer area for the specified index
   and marks the point ``isDirty = true``. ``ezFAIL`` is returned when the
   index does not exist.
3. Reading values: ``ezDataModel_GetDataPoint`` returns a pointer to the
   stored bytes and the data size. No copy is performed on read, so the pointer
   refers to live data. Because every data point is aligned (see below), it can
   be cast to a pointer of the matching type.
4. Dirty scanning: ``ezDataModel_GetFirstDirty`` scans the points and returns
   the index of the first point with ``isDirty == true`` (or
   ``DATA_POINT_INVALID`` when none are dirty).
5. Clearing flags: The application can clear a single point's dirty flag
   with ``ezDataModel_ClearDirtyFlags`` or clear all flags with
   ``ezDataModel_ClearAllDirtyFlags``.

Internal behaviour
------------------

- Allocation: During initialization the component zeroes the buffer, walks the
  provided ``ezDataPoint_t`` array and assigns regions of the contiguous buffer
  to each point in order. Violations of the invariants below are detected at
  run time (not by assertion), logged as errors, and make
  ``ezDataModel_Initialize`` return ``false``.
- Alignment: Each data point is aligned to the lowest set bit of its size,
  capped at ``EZ_DATA_MODEL_ALIGNMENT`` (default 8 bytes, must be a power of
  two, can be overridden at build time). Since ``sizeof`` of a type is a
  multiple of its alignment, a ``uint16_t`` point is 2-byte aligned, a
  ``uint32_t`` or ``float`` point 4-byte aligned, and a ``double`` or larger
  point 8-byte aligned. The alignment is computed from the real buffer address,
  so the buffer itself does not have to be aligned. Padding bytes count
  against the buffer size, so size the buffer with some slack.
- Lookup: Data points are stored in index-sorted order and lookups use a
  binary search (see ``ezDataModel_FindDataPointByIndex``) to locate a
  point by index. This gives O(log N) lookup performance for reads/writes.
- Dirty scanning: ``ezDataModel_GetFirstDirty`` is a linear scan, O(N) per
  call.
- Concurrency: The component is intentionally minimal and not thread safe.
  Setting a value (copy, then dirty flag) is not atomic, and returned data
  pointers refer to live data. The caller must ensure mutual exclusion if the
  data model is accessed from concurrent tasks or ISRs.

Component invariants and failure modes
-------------------------------------

- The ``ezDataPoint_t`` array must be sorted by strictly increasing ``index``
  (no duplicates), each entry must have a non-zero ``size``, and no index may
  be ``DATA_POINT_INVALID`` (``UINT32_MAX``, reserved).
- The buffer must be large enough for all points including alignment padding.
- If any of these is violated, ``ezDataModel_Initialize`` returns ``false``
  and leaves the model empty (``data_points = NULL``, no data points), so that
  later calls fail safely with an error or ``NULL`` instead of touching memory.
  The application should treat this as a fatal configuration error.

Data Flow
============================

The typical data flow is: the application writes into a data point, the
data point is marked dirty, a synchronizer or publisher scans the data model
for dirty points and reads their bytes to send or persist them, then clears
the dirty flags.

.. mermaid::

    flowchart LR
        AppWrite[Application Write] -->|ezDataModel_SetDataPoint| DM[(Data Model Buffer)]
        DM -->|isDirty=true| DirtyList[Dirty Scan]
        DirtyList -->|ezDataModel_GetDataPoint| Publisher[Publisher / Sync]
        Publisher -->|ezDataModel_ClearDirtyFlags| DM

Component's data type
============================

Key types defined by the component:

- ``ezDataPoint_t``: describes a single data point with the fields:
  - ``index``: numeric identifier for the data point.
  - ``size``: number of bytes allocated for the value.
  - ``data``: pointer into the data model buffer where the bytes reside.
  - ``isDirty``: boolean flag indicating whether the point has changed.
    Cleared by ``ezDataModel_Initialize``.

- ``ezDataModel_t``: container struct holding:
  - the pointer to the ``ezDataPoint_t`` array,
  - the number of entries,
  - the contiguous buffer pointer and its size.

- Constants and APIs:
  - ``DATA_POINT_INVALID``: returned by ``ezDataModel_GetFirstDirty`` when
    no dirty points are found. It is reserved and cannot be used as an index.
  - ``EZ_DATA_MODEL_ALIGNMENT``: maximum alignment of a data point in bytes.
  - Public API functions: ``ezDataModel_Initialize``,
    ``ezDataModel_SetDataPoint``, ``ezDataModel_GetDataPoint``,
    ``ezDataModel_GetFirstDirty``, ``ezDataModel_ClearDirtyFlags``, and
    ``ezDataModel_ClearAllDirtyFlags``.

Implementation notes
============================

- The design favors a static, deterministic memory layout (no dynamic
  allocations at runtime) and predictable lookup time via binary search. Data points are zero-initialized
  and aligned, so scalar values can be read through typed pointers.
- The component intentionally keeps semantics simple so it can be composed
  with other services such as an event bus or a synchronizer that reacts to
  dirty data points.

See also: ``easy_embedded/service/data_model/ez_data_model.h`` and
``easy_embedded/service/data_model/ez_data_model.c`` for API and implementation
details.
