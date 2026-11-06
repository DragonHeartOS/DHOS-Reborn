import CommonLib;
import KatlineAPI;

extern "C" auto dhos_main(CL::Span<CL::StringView>,
    CL::HashMap<CL::StringView, CL::StringView>) -> int
{
	Katline::Syscalls::sys_debug_u64(0x6969, 0x420420);
	for (;;) {
		Katline::Syscalls::sys_yield();
	}
}
