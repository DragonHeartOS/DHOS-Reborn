export module Katline:SyscallDebugU64;

import CommonLib;
import :Debug;
import :SyscallKernelContract;

namespace Katline::Syscalls {

template<> struct Spec<SyscallNumber::DebugU64> {
	static auto call(u64 tag, u64 value) -> Result<void>
	{
		Debug::print_formatted("[Debug] tag=0x%016llx value=0x%016llx\n",
		    static_cast<unsigned long long>(tag),
		    static_cast<unsigned long long>(value));
		return Result<void>::Ok();
	}
};

}
