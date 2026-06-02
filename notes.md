# Boot Notes

## Katline

- Starts one generic first user process from a bootstrap blob embedded in the
  kernel binary (this is to avoid putting an ELF loader in the kernele, it can
  be embedded with #embed i love c++).
- Copies the embedded bootstrap image to any free address and jumps to its
  entry point.
- Exposes boot data through IPC and memory-object handles.
- Maps pages via `sys_memory_map(Handle memory_object, u64 offset, u64 size,
  MemoryFlags flags, void **out_addr)`.
- Maps pages in another process via `sys_process_memory_map(Handle process,
  Handle memory_object, u64 offset, u64 size, MemoryFlags flags, void
  **out_addr)`.
- Gets current process via `sys_process_current() -> Result<Handle>`.
- Creates processes via `sys_process_create() -> Result<Handle>`.
- Creates threads via `sys_thread_create(process_handle, entry, stack) -> Result<Handle>`.
- Starts threads via `sys_thread_start(thread_handle)`.
- Handles permissions via IPC.
- Grants boot capabilities to the first process:
  - `Cap::ManageCaps`
  - `Cap::BootInfo`
  - `Cap::MapMMIO`
  - `Cap::ManageProcesses`
  - `Cap::IOPort`

### Capabilities

- `ManageCaps`: remove own caps or grant owned caps to child processes.
- `BootFramebuffer`: retrieve boot framebuffer info from kernel.
- `MapMMIO`: map physical/MMIO-backed memory objects.
- `ManageProcesses`: create/kill processes.
- `BootModules`: retrieve boot modules from kernel.
- `BootCmdline`: retrieve command line from kernel.
- `IOPort`: use `sys_out`, `sys_in`, and reserve I/O port ranges.

Capability arguments:

- `pid`
- `cap id`

### Boot Info

- `Msg::GetBootInfo() -> status, Handle`
- Returns a handle to one packed `BootInfoTable`.
- The table contains metadata for modules, command line, and framebuffers.
- Metadata includes IDs used to fetch the actual memory objects later.

### I/O Syscalls

- `sys_in(port, size) -> Result<u64>`
- `sys_out(port, value, size) -> Result<void>`

### IPC Messages

- `Msg::GetBootInfo() -> status, Handle`
  - Returns handle to `BootInfoTable`.
  - Requires `Cap::BootInfo`
- `Msg::GetBootModuleMemoryObjectById(id) -> status, Handle`
  - Returns handle to a memory object containing the module binary.
  - Requires `Cap::BootInfo`.
- `Msg::GetCmdlineMemoryObject() -> status, Handle`
  - Returns handle to a memory object containing `BootCmdline`.
  - Requires `Cap::BootInfo`.
- `Msg::GetFramebufferMemoryObjectById(id) -> status, Handle`
  - Returns handle to a memory object containing framebuffer data.
  - Requires `Cap::BootInfo`.
- `Msg::GrantIOPortRange(pid, port, length) -> status, void`
  - Requires `Cap::IOPort`.

```cpp
struct BootInfoTable {
    u64 total_len;
    u64 modules_offset;
    u64 modules_len;
    u64 cmdline_offset;
    u64 cmdline_len;
    u64 framebuffers_offset;
    u64 framebuffers_len;
};

struct BootCmdline {
    u64 len;
    char data[];
};

struct BootModuleList {
    u64 len;
    u64 entries_offset;
};

struct BootModuleEntry {
    u64 id;
    u64 name_offset;
    u64 name_len;
    u64 size;
    u64 flags;
};

struct BootFramebufferList {
    u64 len;
    u64 entries_offset;
};

struct BootFramebufferEntry { // handle in IPC handles
    u64 id;
    u64 width;
    u64 height;
    u64 pitch;
    u64 bpp;
    u64 format;
    u64 size;
    u64 edid_len;
    u64 edid_offset;
    u64 flags;
};
```

## Bootstrap

- First user process started directly by the kernel.
- Reads `BootInfoTable` from kernel via IPC.
- Uses its own tiny built-in loader to start `Loader`.
- Uses `Loader` to start `ProcessManager`.
- Reads boot modules and command line from the boot info table.
- ~~Dies~~ Exits after it starts PM and Loader.

## Loader

- Loads and parses ELF files.
- Starts after bootstrap loads it from the boot module payloads.

## ProcessManager

- Manages services.
- Reads `BootInfoTable` from kernel via IPC.
- Reads command line, module metadata, and framebuffer metadata from the boot
  info table.
- Loads boot-selected services (via Loader over IPC) specified in command line.
- Handles registered binary formats (`binfmts`) via IPC.

## Boot Flow

1. Limine starts Katline.
2. Katline copies the embedded bootstrap image to a free address and starts it.
3. Bootstrap asks the kernel for `BootInfoTable`.
4. Bootstrap reads cmdline and module metadata.
5. Bootstrap fetches module payloads by ID.
6. Bootstrap starts Loader using its own tiny bootstrap ELF loader.
7. Bootstrap asks Loader to start ProcessManager.
8. ProcessManager becomes the normal service and process orchestrator.

## IPC

- Fully async.
- Boot data is fetched by request.
- Kernel fills `handles[]` in reply messages (if sent by kernel).

IPC syscalls:

- `sys_ipc_send(u64 endpoint, IPCMessage *message, u64 flags) -> Result<void>`
  (non-blocking send)
- `sys_ipc_recv(IPCMessage *out_message, u64 flags) -> Result<void>`
  (non-blocking receive)

Notes:

- `sys_ipc_send` can fail if the queue is full.
- Boot endpoints are an internal bootstrap detail only.
- After boot: a NameServer tracks endpoints by process registration (see
  `sender` field).

```cpp
struct IPCMessage {
    u64 id;       // filled by kernel
    u64 sender;   // filled by kernel
    u64 reply_to; // 0 means "not a reply"
    u64 cookie;   // user-defined cookie
    u64 type;
    u64 status;
    u64 args[6];
    Handle handles[4]; // u64 array of kernel-mediated handles
};
```

## Events

- `sys_wait_event(Event *out_event, EventFlags event_mask) -> Result<void>`
- `sys_timer_set(u64 us) -> Result<TimerID>`
- `sys_timer_cancel(TimerID id) -> Result<void>`

Event masks:

- `Event::IPC`: IPC queue has messages.
- `Event::Child`: a child changed state.
- `Event::Timer`: a timer expired.
