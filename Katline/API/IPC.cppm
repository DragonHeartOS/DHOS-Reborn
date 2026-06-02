export module KatlineAPI:IPC;

import CommonLib;

export {

	namespace Katline::IPC {

	struct Handle {
		u64 id;
	};

	struct Message {
		u64 id;
		u64 sender;
		u64 reply_to; // 0 means "not a reply"
		u64 cookie;
		u64 type;
		u64 status;
		u64 args[6];
		Handle handles[4];
	};

	}
}
