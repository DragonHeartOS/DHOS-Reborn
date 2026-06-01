Katline:
    - load module[0] and module[1] as initial processes (excluding the kernel
    process itself) thorugh minimal elf loader
    - load module list from start code
    - load cmdline from start code
    - creates framebuffer memory object (from start)
    - maps pages (`sys_memory_map(Handle memory_object, u64 offset, u64 size, MemoryFlags flags, void **out_addr)`)
    - maps pages of processes (`sys_process_memory_map(Handle process, Handle memory_object, u64 offset, u64 size, MemoryFlags flags, void **out_addr)`)
    - get current process (`sys_process_current() -> Result<Handle>`)
    - creates processes (`sys_process_create() -> Result<Handle>`)
    - creates threads (`sys_thread_create(process_handle, entry, stack) -> Result<Handle>`)
    - start threads (`sys_thread_start(thread_handle)`)
    - handle permissions via IPC
    - processmanager and loader has boot capabilities (Cap::ManageCaps, Cap::BootFramebuffer, Cap::MapMMIO, Cap::ManageProcesses, Cap::BootModules, Cap::BootCmdline, Cap::IOPort)

    caps:
    - ManageCaps: can remove caps of own or give caps (that the current process has) to child process
    - BootFramebfufer: can get boot fb info from kernel
    - MapMMIO: can use map physical/mmio-backed memory objets
    - ManageProcesses: can create/kill processes
    - BootModules: can get boot modules from kernell
    - BootCmdline: can get cmdline from kernel
    - IOPort: can use sys_out, sys_in and reserve io port ranges

    caps arguments:
    - pid
    - cap id

    syscalls:
    - sys_in(port, size) -> Result<u64>
    - sys_out(port, value, size) -> Result<void>

    ipc messages:
    - Msg::GrantIOPortRange(pid, port, length) -> status, void (requires Cap::IOPort)
    - Msg::GetBootFramebuffers(offset) -> status, Handle[4] (to memory object containign BootFramebufferList and actual framebuffer memory objects) (requires Cap::BootFramebuffer) offset for paging (since max 4 handles)
    - Msg::GetBootModules() -> status, Handle (to memory object containing BootModuleList) (requires Cap:BootModules)
    - Msg::GetBootCmdline() -> status, Handle (to memory object containing BootCmdline) (requires Cap::BootCmdline)

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

ProcessManager (module 0):
    - manages services
    - reads the module list from kernel via IPC with the kernel process
    - reads the command line from kernel via the IPC with the kernel process
    - loads (via Loader with IPC) initial boot modules specified in command
    - treats the first module as process manager configuration
    - line from the modules retrieved from the kernel
    - handles binfmts that get registered through IPC

Loader (module 1):
    - loads and parses elf files

basically boot flow:

1. Limine starts Katline
2. Katline starts ProcessManager AND Loader
3. ProcessManager reads cmdline/modules and asks Loader to spwan them
4. Loader loads the shit processmanager wants

for ipc:
    fully async

    syscalls:
    - `sys_ipc_send(u64 endpoint, IPCMessage *message, u64 flags) -> Result<void>` non-blocking send
    - `sys_ipc_recv(IPCMessage *out_message, u64 flags) -> Result<void>` non-blocking receive

    sys_ipc_send can error out if the queue is full

    definitions:
    ```cpp
    struct IPCMessage {
        u64 id;       // filled by kernel
        u64 sender;   // filled by kernel
        u64 reply_to; // 0 means "not a reply"
        u64 cookie;   // cookie that can be set by the user
        u64 type;
        u64 status;
        u64 args[6];
        Handle handles[4]; // u64 array of kerenl mediated handles
    };
    ```

    when the kernel boots, the kernel process get the endpoint of 0, the
    processmanager endpoint 1 and loader 2 (determined by module index)

    after that, a NameServer keeps track of endpoints by registration of
    processes. (see sender field)

for events:
    - `sys_wait_event(Event *out_event, EventFlags event_mask) -> Result<void>`
    - `sys_timer_set(u64 us) -> Result<TimerID>`
    - `sys_timer_cancel(TimerID id) -> Result<void>`

    event masks:
    - `Event::IPC` - IPC queue has messages
    - `Event::Child` - A child changed state
    - `Event::Timer` - A timer expired




