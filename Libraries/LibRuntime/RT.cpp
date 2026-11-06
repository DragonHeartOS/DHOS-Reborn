#include <string.h>

import CommonLib;
import KatlineAPI;

extern "C" [[gnu::weak]]
int main(int argc, char const **argv, char const **envp);

extern "C" [[gnu::weak]]
int dhos_main(CL::Span<CL::StringView> args,
    CL::HashMap<CL::StringView, CL::StringView> environ);

extern "C" void _start()
{
	Katline::Syscalls::sys_debug_u64(0x52540001, 1);
	char const **envp { nullptr };
	CL::ArrayList<char const *> args;

	// TODO: Replace this with actual command line info.
	auto const current_process_res
	    = Katline::Syscalls::sys_get_process_current();
	if (current_process_res.is_err()) {
		Katline::Syscalls::sys_exit(1);
	}
	Katline::Syscalls::sys_debug_u64(0x52540001, 2);
	auto const current_process { current_process_res.get_ok_unsafe() };

	u64 process_info_size_bytes {};
	auto const process_info_size_res {
		Katline::Syscalls::sys_get_process_info(
		    current_process, nullptr, &process_info_size_bytes),
	};
	if (process_info_size_res.is_err()) {
		Katline::Syscalls::sys_exit(1);
	}
	Katline::Syscalls::sys_debug_u64(0x52540001, 3);
	if (process_info_size_bytes < sizeof(Katline::ProcessInfo)
	    || process_info_size_bytes > 512) {
		Katline::Syscalls::sys_exit(1);
	}

	alignas(Katline::ProcessInfo) char process_info_storage[512] {};
	auto *process_info {
		reinterpret_cast<Katline::ProcessInfo *>(process_info_storage),
	};
	auto const process_info_res {
		Katline::Syscalls::sys_get_process_info(
		    current_process, process_info, &process_info_size_bytes),
	};
	if (process_info_res.is_err()) {
		Katline::Syscalls::sys_exit(1);
	}
	Katline::Syscalls::sys_debug_u64(0x52540001, 4);

	auto *name { new char[process_info->name_len + 1] };
	memcpy(name, process_info->name, process_info->name_len);
	name[process_info->name_len] = '\0';
	args.emplace(name);

	int ret {};
	if (dhos_main) {
		Katline::Syscalls::sys_debug_u64(0x52540001, 5);
		CL::ArrayList<CL::StringView> arg_views;
		for (usize i {}; i < args.size(); ++i)
			arg_views.emplace(CL::StringView { args[i] });

		CL::HashMap<CL::StringView, CL::StringView> environ;

		for (char const **entry { envp }; entry && *entry; entry++) {
			CL::StringView var { *entry };

			auto pos {
				var.iter().enumerate().find_if(
				    [](auto const &x) { return x.second.value == '='; }),
			};
			if (pos.is_none())
				continue;

			environ.insert_or_replace(
			    var.substring(0, pos->first), var.substring(pos->first + 1));
		}

		ret = dhos_main(arg_views.span(), environ);
	} else if (main) {
		Katline::Syscalls::sys_debug_u64(0x52540001, 6);
		ret = main(static_cast<int>(args.size()), args.data(), envp);
	} else {
		ret = 1;
	}

	Katline::Syscalls::sys_exit(ret);
}
