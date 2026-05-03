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
   points array, and the buffer. The initializer partitions the buffer and
   assigns each point's ``data`` pointer into that buffer.
2. Setting values: ``ezDataModel_SetDataPoint`` copies the caller's bytes
   into the pre-assigned buffer area for the specified index and marks the
   point ``isDirty = true``.
3. Reading values: ``ezDataModel_GetDataPoint`` returns a pointer to the
   stored bytes and the data size. No copy is performed on read.
4. Dirty scanning: ``ezDataModel_GetFirstDirty`` scans the points and returns
   the index of the first point with ``isDirty == true`` (or
   ``DATA_POINT_INVALID`` when none are dirty).
5. Clearing flags: The application can clear a single point's dirty flag
   with ``ezDataModel_ClearDirtyFlags`` or clear all flags with
   ``ezDataModel_ClearAllDirtyFlags``.

Internal behaviour
------------------

- Allocation: During initialization the component walks the provided
  ``ezDataPoint_t`` array and assigns regions of the contiguous buffer to
  each point in order; it asserts on size/ordering violations and ensures
  the buffer is large enough.
- Lookup: Data points are stored in index-sorted order and lookups use a
  binary search (see ``ezDataModel_FindDataPointByIndex``) to locate a
  point by index. This gives O(log N) lookup performance for reads/writes.
- Concurrency: The component is intentionally minimal and does not provide
  locking; the caller must ensure mutual exclusion if the data model is
  accessed from concurrent contexts.

Component invariants and failure modes
-------------------------------------

- The ``ezDataPoint_t`` array must be sorted by ``index`` and each entry
  must have a non-zero ``size``. Initialization asserts these invariants.
- If the buffer is too small for the provided points, initialization fails
  (assertion) and the data model must not be used.

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

- ``ezDataModel_t``: container struct holding:
  - the pointer to the ``ezDataPoint_t`` array,
  - the number of entries,
  - the contiguous buffer pointer and its size.

- Constants and APIs:
  - ``DATA_POINT_INVALID``: returned by ``ezDataModel_GetFirstDirty`` when
    no dirty points are found.
  - Public API functions: ``ezDataModel_Initialize``,
    ``ezDataModel_SetDataPoint``, ``ezDataModel_GetDataPoint``,
    ``ezDataModel_GetFirstDirty``, ``ezDataModel_ClearDirtyFlags``, and
    ``ezDataModel_ClearAllDirtyFlags``.

Implementation notes
============================

- The design favors a static, deterministic memory layout (no dynamic
  allocations at runtime) and predictable lookup time via binary search.
- The component intentionally keeps semantics simple so it can be composed
  with other services such as an event bus or a synchronizer that reacts to
  dirty data points.

See also: ``easy_embedded/service/data_model/ez_data_model.h`` and
``easy_embedded/service/data_model/ez_data_model.c`` for API and implementation
details.
