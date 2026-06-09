export module Katline:HandleManager;

import CommonLib;
import KatlineAPI;
import :MemoryObject;
import :Thread;
import :Sync;

export {
	namespace Katline::Arch {

	enum class HandleKind : u8 {
		Process,
		Thread,
		MemoryObject,
	};

	class HandleManager {
	public:
		auto open(Process *owner, HandleKind kind, void *object)
		    -> Katline::Handle;
		auto duplicate(Process *owner, Katline::Handle handle)
		    -> Katline::Handle;
		auto close(Process *owner, Katline::Handle handle) -> bool;
		auto release_owned_handles(Process *owner) -> void;

		template<typename T>
		auto resolve(Process *owner, Katline::Handle handle, HandleKind kind)
		    -> T *
		{
			Sync::ScopedIrqSpinLock guard { m_lock };
			auto *entry { find_entry(owner, handle) };
			if (!entry || entry->kind != kind)
				return nullptr;
			return static_cast<T *>(entry->object);
		}

		template<typename T>
		auto resolve_any(Katline::Handle handle, HandleKind kind) -> T *
		{
			Sync::ScopedIrqSpinLock guard { m_lock };
			auto *entry { find_entry(nullptr, handle) };
			if (!entry || entry->kind != kind)
				return nullptr;
			return static_cast<T *>(entry->object);
		}

		// Only call this after Memory::MM::init(); it needs heap allocation.
		static auto the() -> HandleManager &;

	private:
		struct Entry {
			HandleKind kind {};
			void *object {};
			Process *owner {};
		};

		CL::HashMap<u64, Entry> m_entries;
		u64 m_next_handle_id { 1 };
		Sync::SpinLock m_lock;

		auto find_entry(Process const *owner, Katline::Handle handle) const
		    -> Entry const *;
		auto retain_object(HandleKind kind, void *object) -> void;
		auto release_object(HandleKind kind, void *object) -> void;
		auto adjust_object_refcount(HandleKind kind, void *object, i32 delta)
		    -> void;
	};

	}
}

namespace Katline::Arch {

auto HandleManager::adjust_object_refcount(
    HandleKind kind, void *object, i32 delta) -> void
{
	if (!object)
		return;

	switch (kind) {
	case HandleKind::Process: {
		auto *process { static_cast<Process *>(object) };
		if (delta > 0)
			process->handle_count++;
		else if (process->handle_count > 0)
			process->handle_count--;
		break;
	}
	case HandleKind::Thread: {
		auto *thread { static_cast<Thread *>(object) };
		if (delta > 0)
			thread->handle_count++;
		else if (thread->handle_count > 0)
			thread->handle_count--;
		break;
	}
	case HandleKind::MemoryObject:
		if (delta > 0) {
			Memory::retain_memory_object(
			    static_cast<Memory::MemoryObject *>(object));
		} else {
			Memory::release_memory_object(
			    static_cast<Memory::MemoryObject *>(object));
		}
		break;
	}
}

auto HandleManager::retain_object(HandleKind kind, void *object) -> void
{
	adjust_object_refcount(kind, object, 1);
}

auto HandleManager::release_object(HandleKind kind, void *object) -> void
{
	adjust_object_refcount(kind, object, -1);
}

auto HandleManager::find_entry(
    Process const *owner, Katline::Handle handle) const -> Entry const *
{
	if (handle.is_invalid())
		return nullptr;

	auto entry_opt { m_entries.get(handle.id) };
	if (!entry_opt)
		return nullptr;
	auto const &entry { *entry_opt };

	if (owner && entry.owner != owner)
		return nullptr;

	return &entry;
}

auto HandleManager::open(Process *owner, HandleKind kind, void *object)
    -> Katline::Handle
{
	if (!owner || !object)
		return {};

	Sync::ScopedIrqSpinLock guard { m_lock };
	auto const id { m_next_handle_id++ };
	if (id == 0)
		return {};

	retain_object(kind, object);
	m_entries.insert_or_replace(id,
	    {
	        .kind = kind,
	        .object = object,
	        .owner = owner,
	    });
	return { id };
}

auto HandleManager::duplicate(Process *owner, Katline::Handle handle)
    -> Katline::Handle
{
	Sync::ScopedIrqSpinLock guard { m_lock };
	auto const *entry { find_entry(owner, handle) };
	if (!entry)
		return {};

	auto const id { m_next_handle_id++ };
	if (id == 0)
		return {};

	retain_object(entry->kind, entry->object);

	m_entries.insert_or_replace(id,
	    {
	        .kind = entry->kind,
	        .object = entry->object,
	        .owner = owner,
	    });
	return { id };
}

auto HandleManager::close(Process *owner, Katline::Handle handle) -> bool
{
	Sync::ScopedIrqSpinLock guard { m_lock };
	auto const *entry { find_entry(owner, handle) };
	if (!entry)
		return false;

	auto const kind { entry->kind };
	auto const object { entry->object };
	m_entries.remove(handle.id);
	release_object(kind, object);
	return true;
}

auto HandleManager::release_owned_handles(Process *owner) -> void
{
	if (!owner)
		return;

	CL::ArrayList<u64> handles;
	{
		Sync::ScopedIrqSpinLock guard { m_lock };
		m_entries.iter().for_each([&](auto const &entry) {
			if (entry.value.owner == owner)
				handles.push(entry.key);
		});
	}

	for (usize i {}; i < handles.size(); i++)
		close(owner, { handles[i] });
}

auto HandleManager::the() -> HandleManager &
{
	static HandleManager instance {};
	return instance;
}

}
