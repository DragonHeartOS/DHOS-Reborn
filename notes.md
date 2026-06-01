# Boot Notes

## Katline

- Loads `module[0]` and `module[1]` as initial processes (excluding the kernel
  process) through a minimal ELF loader.
- Loads module list from start code.
- Loads command line from start code.
- Creates framebuffer memory object (from start info).
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
- Grants boot capabilities to ProcessManager and Loader:
  - `Cap::ManageCaps`
  - `Cap::BootFramebuffer`
  - `Cap::MapMMIO`
  - `Cap::ManageProcesses`
  - `Cap::BootModules`
  - `Cap::BootCmdline`
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

### I/O Syscalls

- `sys_in(port, size) -> Result<u64>`
- `sys_out(port, value, size) -> Result<void>`

### IPC Messages

- `Msg::GrantIOPortRange(pid, port, length) -> status, void`
  - Requires `Cap::IOPort`.
- `Msg::GetBootFramebuffers(offset) -> status, Handle[4]`
  - Returns handles to a memory object containing `BootFramebufferList` and
    framebuffer memory objects.
  - Uses `offset` for paging (max 4 handles per message).
  - Requires `Cap::BootFramebuffer`.
- `Msg::GetBootModules() -> status, Handle`
  - Returns handle to memory object containing `BootModuleList`.
  - Requires `Cap::BootModules`.
- `Msg::GetBootCmdline() -> status, Handle`
  - Returns handle to memory object containing `BootCmdline`.
  - Requires `Cap::BootCmdline`.

```cpp
struct BootCmdline {
    u64 len;
    char data[];
};

struct BootFramebufferList {
    u64 total_len;
    u64 base_index;
    u64 len;
    u64 entries_offset;
};

struct BootFramebufferEntry { // handle in IPC handles
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

struct BootModuleList {
    u64 len;
    u64 entries_offset;
};

struct BootModuleEntry {
    u64 name_offset;
    u64 name_len;
    u64 data_offset;
    u64 data_len;
};
```

## ProcessManager (`module[0]`)

- Manages services.
- Reads module list from kernel via IPC.
- Reads command line from kernel via IPC.
- Loads initial boot modules (via Loader over IPC) specified in command line.
- Treats the first module as ProcessManager configuration.
- Handles registered binary formats (`binfmts`) via IPC.

## Loader (`module[1]`)

- Loads and parses ELF files.

## Boot Flow

1. Limine starts Katline.
2. Katline starts ProcessManager and Loader.
3. ProcessManager reads cmdline/modules and asks Loader to spawn processes.
4. Loader loads the requested processes.

## IPC

- Fully async.

IPC syscalls:

- `sys_ipc_send(u64 endpoint, IPCMessage *message, u64 flags) -> Result<void>`
  (non-blocking send)
- `sys_ipc_recv(IPCMessage *out_message, u64 flags) -> Result<void>`
  (non-blocking receive)

Notes:

- `sys_ipc_send` can fail if the queue is full.
- At boot: kernel process endpoint is `0`, ProcessManager is `1`, Loader is `2`
  (by module index).
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

