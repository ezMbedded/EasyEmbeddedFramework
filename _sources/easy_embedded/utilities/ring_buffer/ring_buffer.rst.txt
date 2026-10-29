============================================================
Ring Buffer
============================================================

Introduction
============================
This document describes the ring buffer component of EasyEmbeddedFramework. It explains the structure, behavior, and data types used in the ring buffer implementation. The ring buffer provides a fixed-size, circular FIFO data structure for efficient data storage and retrieval in embedded systems.

The ring buffer component allows users to:

- Initialize a ring buffer with a user-provided memory buffer
- Push and pop data in FIFO order
- Check if the buffer is full or empty
- Reset the buffer and query available memory

Limitations:

- Not thread-safe by default
- Buffer size is fixed at initialization
- Not suitable for dynamic memory allocation or resizing

Use cases:

- UART/serial communication buffering
- Audio or sensor data streaming
- Producer-consumer scenarios in embedded applications

Component's structure
============================
The ring buffer component is composed of the following elements:
- `RingBuffer`: Main structure holding buffer pointer, capacity, head/tail indices, and byte counters
- User-provided data buffer (uint8_t array)

Mermaid diagram:

.. mermaid::

   classDiagram
     class RingBuffer {
       uint8_t* buff
       uint16_t capacity
       uint16_t head_index
       uint16_t tail_index
       uint16_t written_byte_count
     }
     RingBuffer o-- "1" uint8_t : buff


Component's behavior
============================
External behavior:

- Users can initialize, push to, pop from, reset, and query the ring buffer.
- Data is written to the buffer in FIFO order; when full, no more data can be pushed until space is freed.
- When empty, no data can be popped.

Internal behavior:

- The buffer uses head and tail indices to track where to write and read data.
- Indices wrap around when reaching the end of the buffer (circular behavior).
- The written byte count tracks the number of bytes currently stored.
- Resetting the buffer clears indices and byte count, and optionally zeroes the buffer.

Component's data type
============================
The ring buffer component uses the following data types:

- `RingBuffer`: Structure representing the ring buffer
- `uint8_t*`: Pointer to the user-provided data buffer
- `uint16_t`: For buffer size, indices, and counters
- `bool`: For status returns

All data stored in the buffer is managed as bytes (uint8_t), allowing for flexible use with different data types.
