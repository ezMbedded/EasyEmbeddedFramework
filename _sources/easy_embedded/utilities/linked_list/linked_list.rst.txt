============================================================
Linked List
============================================================

Introduction
============================

Overview
-----------------------------
This document describes the **Linked List** component of the Easy Embedded Framework. 
The linked list is a fundamental data structure that provides dynamic memory organization 
and efficient insertion/removal operations.

What This Component Does
~~~~~~~~~~~~~~~~~~~~~~~~~
The linked list component provides a **doubly-linked circular list implementation** 
inspired by the Linux kernel's list implementation. It enables:

- Dynamic collection of nodes without fixed size constraints
- Forward and backward traversal through the list
- Embedding list nodes directly into user-defined structures
- Generic container for organizing data of any type

What This Component Does NOT Do
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- Does NOT provide automatic memory allocation (nodes must be pre-allocated or managed by user)
- Does NOT include sorting or searching algorithms
- Does NOT provide thread-safety or locking mechanisms
- Does NOT manage data payload directly (only manages the linking structure)


Component's Structure
============================

Architecture Overview
-----------------------------
The linked list component consists of:

1. **Core Data Structure**: ``struct Node`` - metadata holding next and previous pointers
2. **Helper Macros**: Convenience macros for common operations (iteration, insertion, extraction)
3. **Public API Functions**: Core operations like append, insert, unlink, and search
4. **Design Pattern**: Intrusive linked list (nodes embedded in user structures)

The intrusive design allows the list node to be embedded anywhere in a user-defined structure, 
avoiding separate allocation for the list metadata.

Node Structure
~~~~~~~~~~~~~~
The node structure is minimal and efficient:

.. code-block:: c

    struct Node {
        struct Node * next;  // Pointer to next node in the list
        struct Node * prev;  // Pointer to previous node in the list
    };

Circular List Design
~~~~~~~~~~~~~~~~~~~~~
The linked list uses a **circular doubly-linked** design where:

- The list is represented by a head/sentinel node
- The head node's ``next`` points to the first data node
- The head node's ``prev`` points to the last data node
- An empty list has the head pointing to itself
- This eliminates special cases for list boundaries

Visual representation of a circular list with 3 nodes:

.. mermaid::

    graph LR
        Head["Head<br/>(Sentinel Node)"]
        Node1["Node 1"]
        Node2["Node 2"]
        Node3["Node 3"]
        
        Head -->|next| Node1
        Node1 -->|next| Node2
        Node2 -->|next| Node3
        Node3 -->|next| Head
        
        Head -.->|prev| Node3
        Node3 -.->|prev| Node2
        Node2 -.->|prev| Node1
        Node1 -.->|prev| Head


Intrusive Design Pattern
~~~~~~~~~~~~~~~~~~~~~~~~
Instead of wrapping data in a node, the node is embedded in the user's structure:

.. code-block:: c

    // User-defined structure containing a list node
    struct Task {
        uint32_t task_id;
        void (*handler)(void);
        struct Node list_node;  // Embedded node
    };

    // To get the parent structure from a node:
    struct Task *task = EZ_LINKEDLIST_GET_PARENT_OF(node, list_node, struct Task);

This approach provides:

- **Memory Efficiency**: No extra allocation for list metadata
- **Flexibility**: One structure can participate in multiple lists (multiple embedded nodes)
- **Cache Locality**: Data and list metadata reside close together

Component's Behavior
============================

External Behavior
-----------------------------
The component provides the following operations:

**Initialization**

.. code-block:: c

    void ezLinkedList_InitNode(struct Node* node);

Initializes a new node with both ``next`` and ``prev`` pointing to itself (empty single-node state).

**List Operations**

.. code-block:: c

    // Append new_node after node in the list
    bool ezLinkedList_AppendNode(struct Node *new_node, struct Node *node);
    
    // Insert new_node at the head position
    struct Node *ezLinkedList_InsertNewHead(struct Node *current_head, 
                                            struct Node *new_node);
    
    // Remove the head node from the list
    struct Node *ezLinkedList_UnlinkCurrentHead(struct Node *head);

**Query Operations**

.. code-block:: c

    // Get the number of nodes in the list
    uint16_t ezLinkedList_GetListSize(struct Node* list_head);
    
    // Check if a node exists in the list
    bool ezLinkedList_IsNodeInList(struct Node *head, struct Node *searched_node);

**Helper Macros**

.. code-block:: c

    // Iterate through all nodes in a list
    EZ_LINKEDLIST_FOR_EACH(node, head)
    
    // Initialize a node (compile-time)
    EZ_LINKEDLIST_INIT_NODE(name)
    
    // Add node at head position
    EZ_LINKEDLIST_ADD_HEAD(list_head, node)
    
    // Add node at tail position
    EZ_LINKEDLIST_ADD_TAIL(list_head, node)
    
    // Unlink a node from the list
    EZ_LINKEDLIST_UNLINK_NODE(node)
    
    // Check if list is empty
    IS_LIST_EMPTY(list_head)
    
    // Get the parent structure from an embedded node
    EZ_LINKEDLIST_GET_PARENT_OF(ptr, member, type)

State Diagram
~~~~~~~~~~~~~
A single node's state transitions:

.. mermaid::

    stateDiagram-v2
        [*] --> Initialized: ezLinkedList_InitNode()
        Initialized --> InList: AppendNode()<br/>InsertNewHead()
        InList --> Initialized: UnlinkCurrentHead()<br/>UNLINK_NODE()
        Initialized --> [*]

Operation Sequences
~~~~~~~~~~~~~~~~~~~

**Appending a Node**

.. mermaid::

    sequenceDiagram
        participant New as New Node
        participant Ref as Reference Node
        participant Next as Next Node

        Note over New: new_node->next<br/>new_node->prev
        Note over Ref: ref_node
        Note over Next: next_node

        New->>Ref: Get ref->next
        activate Ref
        Ref-->>New: Returns next_node
        deactivate Ref
        
        Note over New: 1. new_node->next = next_node
        Note over New: 2. new_node->prev = ref_node
        Note over Next: 3. next_node->prev = new_node
        Note over Ref: 4. ref_node->next = new_node

**List Iteration**

.. mermaid::

    flowchart TD
        A["Start: node = head->next"] --> B{"node != head?"}
        B -->|Yes| C["Process node"]
        C --> D["node = node->next"]
        D --> B
        B -->|No| E["End"]


Internal Behavior
-----------------

Circular Structure Advantages
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The circular design eliminates special cases:

1. **No NULL checks for end of list**: The sentinel wraps around to the beginning
2. **Same operation for head and tail**: Both use the same append mechanism
3. **Efficient empty list check**: Simply compare ``head->next == head``

Append Operation Details
~~~~~~~~~~~~~~~~~~~~~~~~
The ``ezLinkedList_AppendNode()`` function inserts ``new_node`` after a given node:

.. code-block:: c

    bool ezLinkedList_AppendNode(struct Node* new_node, struct Node* node)
    {
        if (new_node != NULL && node != NULL)
        {
            new_node->next = node->next;      // 1. new_node points to node's next
            new_node->prev = node;             // 2. new_node's prev points to node
            node->next->prev = new_node;       // 3. old next node's prev points to new_node
            node->next = new_node;             // 4. node's next points to new_node
            return true;
        }
        return false;
    }

This operation is atomic and maintains list integrity.

Component's Data Type
============================

Node Structure Definition
--------------------------

.. code-block:: c

    struct Node {
        struct Node * next;  /**< pointer to the next node in a linked list*/
        struct Node * prev;  /**< pointer to the previous node in a linked list*/
    };

**Properties:**

- **Size**: 2 × pointer size (typically 16 bytes on 64-bit systems)
- **Alignment**: Pointer-aligned
- **Initialization**: Via ``ezLinkedList_InitNode()`` or ``EZ_LINKEDLIST_INIT_NODE()``
- **Lifecycle**: Created by user, managed through API functions

Type Safety Mechanisms
~~~~~~~~~~~~~~~~~~~~~~~
The intrusive design is type-safe through the ``EZ_LINKEDLIST_GET_PARENT_OF`` macro:

.. code-block:: c

    #define OFFSET(type, member) \
        ((char*)&(((type*)0)->member) - (char*)((type*)0))

    #define EZ_LINKEDLIST_GET_PARENT_OF(ptr, member, type) \
        (type*)((char*)ptr - OFFSET(type, member))

This macro:

1. Calculates the byte offset of the member within the type
2. Subtracts this offset from the node pointer to get the parent structure
3. Casts to the correct type
4. Provides compile-time type checking

Configuration
--------------
The linked list is conditionally compiled based on:

.. code-block:: c

    #if (EZ_LINKEDLIST == 1U)
    // Linked list code is compiled
    #endif

Enable linked list support in the target configuration header: ``ez_target_config.h``
