export module Katline:Syscalls;

export import :SyscallABI;
export import :SyscallKernelContract;
export import :SyscallDispatch;

#define X(name, ...) import :Syscall##name;
#import <Katline/API/SyscallList.def>
#undef X

export import KatlineAPI;
