export module Katline:Interrupts;

import CommonLib;
import :IDT;

export {
	namespace Katline {

	namespace Interrupts {

	struct interrupt_frame;

	using Handler = void (*)();

	auto init_defaults(u32 lapic_id) -> void;
	auto register_isr(u8 vector, Handler handler) -> void;

	}

	}
}

namespace Katline::Interrupts {

auto init_defaults(u32 lapic_id) -> void { IDT::init(lapic_id); }

auto register_isr(u8 vector, Handler handler) -> void
{
	IDT::set_handler(vector, handler);
}

}
