============================================================
Remote Procedure Call (RPC)
============================================================

Introduction
============================
The Remote Procedure Call (RPC) component enables communication between distributed components in an embedded system or between a device and a host. It abstracts the underlying transport layer (UART, SPI, USB, etc.) and provides a request-response mechanism for executing functions remotely.

Key features include:
- Asynchronous communication via non-blocking queues.
- Serialization and deserialization (Marshaling/Unmarshaling) of messages.
- Error detection via CRC (Cyclic Redundancy Check).
- Support for encryption flags (logic to be implemented by user).
- Flexible command dispatching via a service table.

Component Structure
============================
The RPC component follows a modular design centered around the ``ezRpc`` instance structure. It interacts with the following sub-components:

1. **Communication Interface**:
   Defines the ``ezRpcCommInterface`` struct with ``transmit`` and ``receive`` function pointers. This adapter pattern allows the RPC component to run on any stream-oriented physical interface.

2. **message Queues**:
   Uses ``ezQueue`` instances for buffering:
   - ``tx_msg_queue``: valid RPC messages ready to be sent to the physical link.
   - ``rx_msg_queue``: valid RPC messages received from the link, waiting to be processed by simple handlers.

3. **Marshaler (Serializer)**:
   Constructs the RPC frame (Header + Payload + CRC) from parameters and pushes it to the transmit queue.

4. **Unmarshaler (Deserializer)**:
   A state machine that processes the incoming byte stream from the receive interface, validates the framing, and reconstructs the message.

5. **Dispatcher**:
   Locates the appropriate callback function from the ``ezRpcCommandEntry`` table based on the Command ID and executes it.

Component Behavior
============================
The component operates in a polling mode or event-driven mode usually triggered via the ``ezRPC_Run`` function.

Initialization
--------------
The user must initialize the instance with memory for queues and a command table mapping IDs to handler functions. The communication interface (transmit/receive functions) and CRC handler must be registered before starting.

Runtime Loop (ezRPC_Run)
------------------------
This function creates the heartbeat of the component:
1. **Receive Phase**: Reads bytes from the ``receive`` interface and feeds them into the Unmarshaler state machine.
2. **Process Phase**: Checks the ``rx_msg_queue``. If a message is complete and valid, it looks up the command ID and triggers the registered callback.
3. **Transmit Phase**: Checks the ``tx_msg_queue``. If data is pending, it calls the ``transmit`` interface to send data to the hardware.
4. **Timeout Check**: Monitors pending requests for timeouts (if the record tracking is active).

Unmarshaling State Machine
--------------------------
The deserializer parses the incoming stream through the following states:

1. ``STATE_SYNC``: Waiting for the synchronization bytes (0xCAFE).
2. ``STATE_UUID``: Reading the 2-byte unique message ID.
3. **Header Parsing**:
   - ``STATE_MSG_TYPE``: Request, Response, or Event.
   - ``STATE_ENCRYPT_FLAG``: Encryption status.
   - ``STATE_CMD_ID``: The command identifier.
   - ``STATE_PAYLOAD_SIZE``: The length of the data payload.
4. ``STATE_PAYLOAD``: Reading the payload data byte-by-byte.
5. ``STATE_CRC``: verifying the checksum (if enabled).

Data Flow
============================

Receive Path
------------
.. mermaid::

  flowchart TD
     A[Physical Link] --> B[ezRpcCommInterface.receive]
     B --> C[Unmarshaler State Machine]
     C --> D[Valid Check<br/>CRC/Header]
     D --> E[rx_msg_queue]
     E --> F[Dispatcher]
     F --> G[Command Handler]

Transmit Path
-------------
.. mermaid::

  flowchart TD
     A[User App] --> B[ezRPC_CreateRpcRequest/Response]
     B --> C[Marshaler]
     C --> D[tx_msg_queue]
     D --> E[ezRpcCommInterface.transmit]
     E --> F[Physical Link]

Component Data Types
============================

RPC Message Header Layout
-------------------------
Every message starts with a fixed-size header followed by the payload.

.. list-table:: ezRpcMsgHeader
   :widths: 15 10 75
   :header-rows: 1

   * - Field
     - Size
     - Description
   * - Sync Bytes
     - 2 Bytes
     - Fixed value ``0xCAFE`` to identify start of frame.
   * - UUID
     - 2 Bytes
     - Unique ID to match requests with responses.
   * - Type
     - 1 Byte
     - ``RPC_MSG_REQ`` (0), ``RPC_MSG_RESP`` (1), etc.
   * - Encrypt
     - 1 Byte
     - Flag indicating payload encryption.
   * - Cmd ID
     - 2 Bytes
     - Identifier matching the function to execute.
   * - Payload Size
     - 4 Bytes
     - Size of the data following the header.

Total Header Size: 12 Bytes.

CRC and Payload
-------------------------
If CRC is enabled, a CRC checksum (size defined by handler, typically 2 or 4 bytes) is appended immediately after the payload.
