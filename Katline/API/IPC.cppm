export module KatlineAPI:IPC;

import CommonLib;
import :Types;

export {

	namespace Katline::IPC {

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

	template<typename TypeEnum>
	requires(CL::IsEnumV<TypeEnum>)
	class MessageBuilder {
	public:
		constexpr explicit MessageBuilder(TypeEnum type)
		{
			m_message.type = static_cast<u64>(type);
		}

		constexpr auto id(u64 value) -> MessageBuilder &
		{
			m_message.id = value;
			return *this;
		}

		constexpr auto sender(u64 value) -> MessageBuilder &
		{
			m_message.sender = value;
			return *this;
		}

		constexpr auto reply_to(u64 value) -> MessageBuilder &
		{
			m_message.reply_to = value;
			return *this;
		}

		constexpr auto cookie(u64 value) -> MessageBuilder &
		{
			m_message.cookie = value;
			return *this;
		}

		constexpr auto status(u64 value) -> MessageBuilder &
		{
			m_message.status = value;
			return *this;
		}

		template<usize Index>
		constexpr auto arg(u64 value) -> MessageBuilder &
		{
			static_assert(Index < 6, "IPC message arg index out of bounds");
			m_message.args[Index] = value;
			return *this;
		}

		constexpr auto arg(usize index, u64 value) -> MessageBuilder &
		{
			if (index >= 6)
				CL::panic("IPC message arg index out of bounds");
			m_message.args[index] = value;
			return *this;
		}

		template<usize Index>
		constexpr auto handle(Handle value) -> MessageBuilder &
		{
			static_assert(Index < 4, "IPC message handle index out of bounds");
			m_message.handles[Index] = value;
			return *this;
		}

		constexpr auto handle(usize index, Handle value) -> MessageBuilder &
		{
			if (index >= 4)
				CL::panic("IPC message handle index out of bounds");
			m_message.handles[index] = value;
			return *this;
		}

		constexpr auto build() const -> Message { return m_message; }

	private:
		Message m_message {};
	};

	template<typename TypeEnum>
	requires(CL::IsEnumV<TypeEnum>)
	constexpr auto type_of(Message const &message) -> TypeEnum
	{
		return static_cast<TypeEnum>(message.type);
	}

	template<typename TypeEnum>
	requires(CL::IsEnumV<TypeEnum>)
	constexpr auto make_request(TypeEnum type) -> MessageBuilder<TypeEnum>
	{
		return MessageBuilder<TypeEnum> { type };
	}

	template<typename TypeEnum>
	requires(CL::IsEnumV<TypeEnum>)
	constexpr auto make_reply(Message const &request, TypeEnum type,
	    u64 status = 0) -> MessageBuilder<TypeEnum>
	{
		return MessageBuilder<TypeEnum> { type }
		    .reply_to(request.id)
		    .cookie(request.cookie)
		    .status(status);
	}

	}
}
