============================================================
Queue
============================================================

Introduction
============================
This document describes the queue component of EasyEmbeddedFramework. It explains the structure, behavior, and data types used in the queue implementation. The queue component provides a flexible, memory-efficient FIFO data structure for embedded systems.

The queue component allows users to:

- Create queues with static memory allocation
- Push and pop elements from both front and back
- Reserve elements for later insertion
- Access front and back elements
- Query queue status and size

Limitations:

- The queue is not thread-safe by default
- Memory must be provided by the user
- Not suitable for real-time or high-frequency operations without additional synchronization

Use cases:

- Buffering sensor data
- Task scheduling
- Event/message passing between components

Component's structure
============================
The queue component is composed of the following sub-components:

- `ezQueue`: Main queue structure containing a linked list of items and a memory list for static allocation
- `ezQueueItem`: Represents each element in the queue, holding data and metadata
- `Node`: Linked list node for queue item management
- `MemList`: Manages memory allocation for queue items and their data

Mermaid diagram:


.. mermaid::

   classDiagram
      class ezQueue {
         Node q_item_list
         MemList mem_list
      }
      class ezQueueItem {
         void* data
         uint32_t data_size
         Node node
      }
      class Node {
         +pointer next
      }
      class MemList {
         +allocate()
         +deallocate()
      }
      ezQueue --> ezQueueItem
      ezQueue --> Node
      ezQueue --> MemList
      ezQueueItem --> Node


Component's behavior
============================
External behavior:

- Users can create a queue, push data, pop elements, reserve memory for elements, and access queue status.
- The queue operates in FIFO mode but supports popping from both front and back.
- Memory management is handled internally via static allocation, but the buffer must be provided by the user.

Internal behavior:

- The queue uses a linked list to manage elements.
- Memory for each element and its data is allocated from a static memory pool.
- Reserved elements are not linked to the queue until explicitly pushed.
- Elements can be released if not needed, freeing their memory.
- The queue tracks the number of elements and readiness status.

Component's data type
============================
The queue component uses the following data types:

- `ezQueue`: Structure representing the queue
- `ezQueueItem`: Structure representing each queue element
- `ezReservedElement`: Pointer to reserved element memory
- `Node`: Linked list node
- `MemList`: Static memory allocation manager
- `ezSTATUS`: Status code (ezSUCCESS, ezFAIL)
- `uint8_t`, `uint32_t`: Standard integer types for data and sizes

Data passed to the queue is managed as void pointers, allowing flexibility for different data types.
