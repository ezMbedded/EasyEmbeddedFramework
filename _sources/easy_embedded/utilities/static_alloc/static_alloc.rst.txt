============================================================
Static Allocation
============================================================

Introduction
============================
This document describes the static allocation component of EasyEmbeddedFramework. It explains the structure, behavior, and data types used in the static memory allocator implementation. The static allocation component provides a malloc/free-like API for managing memory from a user-provided static buffer, suitable for embedded systems without dynamic memory allocation.

The static allocation component allows users to:

- Initialize a memory manager with a static buffer
- Allocate and free memory blocks from the buffer
- Track allocated and free memory blocks
- Query the number of allocated and free blocks
- Print memory usage for debugging

Limitations:

- Not thread-safe by default
- Buffer size is fixed at initialization
- No memory compaction or defragmentation beyond adjacent block merging

Use cases:

- Memory management in bare-metal or RTOS-based embedded systems
- Replacement for malloc/free in environments without heap
- Buffer management for communication stacks or data structures

Component's structure
============================
The static allocation component is composed of the following elements:
- `MemList` (or `ezmMemList`): Main structure managing the static buffer, free list, and allocated list
- `MemBlock`: Record of each chunk, stored inside the managed buffer in front of the chunk's data, used for both free and allocated chunks
- `Node`: Linked list node for managing block lists
- User-provided static memory buffer (uint8_t array)

Mermaid diagram:

.. mermaid::

   classDiagram
     class MemList {
       Node free_list_head
       Node alloc_list_head
       uint8_t* buff
       size_t buff_size
     }
     class MemBlock {
       Node node
       void* buff
       size_t buff_size
     }
     MemList o-- "*" MemBlock : manages
     MemBlock o-- Node : node
     MemList o-- "1" uint8_t : buff

Component's behavior
============================
External behavior:

- Users can initialize the memory manager, allocate memory blocks, free blocks, and query memory usage.
- Memory is allocated from the static buffer; freed blocks are returned to the free list and may be merged with adjacent free blocks.
- Debugging functions allow printing the state of the memory manager.

Internal behavior:

- The memory manager maintains two linked lists: one for free blocks and one for allocated blocks.
- Every chunk in the buffer is laid out as ``[MemBlock record][data]``. When allocating, the manager searches for a free chunk large enough for the request, splits it if the remainder can hold another record plus data, and moves the chunk to the allocated list.
- When freeing, the chunk (record and data) is removed from the allocated list, added to the sorted free list, and adjacent free chunks are merged; a merged neighbour's record becomes part of the data.
- The records are carved out of the user buffer, so each allocation costs ``sizeof(MemBlock)`` (rounded up to the alignment) bytes of overhead. There is no global block pool and no limit on the number of chunks other than the buffer size.

Component's data type
============================
The static allocation component uses the following data types:

- `MemList` / `ezmMemList`: Structure managing the memory buffer and block lists
- `MemBlock`: Structure representing each memory block
- `Node`: Linked list node for block management
- `uint8_t*`: Pointer to the static memory buffer
- `size_t`: For buffer size and block sizes
- `bool`: For status returns

All memory managed by this component is accessed as void pointers, allowing flexible use for different data types.
