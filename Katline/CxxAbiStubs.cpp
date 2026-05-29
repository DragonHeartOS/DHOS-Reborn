extern "C" int atexit(void (*)(void))
{
    return 0;
}

extern "C" int __cxa_atexit(void (*)(void*), void*, void*)
{
    return 0;
}

extern "C" int __cxa_guard_acquire(unsigned long long* guard)
{
    return *guard == 0;
}

extern "C" void __cxa_guard_release(unsigned long long* guard)
{
    *guard = 1;
}

extern "C" void __cxa_guard_abort(unsigned long long*)
{
}
