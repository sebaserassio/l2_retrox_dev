#include "stdafx.h"
#include "xstd.h"

namespace xstd {

namespace impl {

void*(*new1)(const size_t) = 0;
void*(*new2)(void*, const size_t, const bool, const wchar_t*, const int) = 0;
void(*delete1)(void*) = 0;
void(*delete2)(void*, void*, const int) = 0;

void Setup(const UINT32 new1, const UINT32 new2, const UINT32 delete1, const UINT32 delete2)
{
	impl::new1 = reinterpret_cast<void*(*)(const size_t)>(new1);
	impl::new2 = reinterpret_cast<void*(*)(void*, const size_t, const bool, const wchar_t*, const int)>(new2);
	impl::delete1 = reinterpret_cast<void(*)(void*)>(delete1);
	impl::delete2 = reinterpret_cast<void(*)(void*, void*, const int)>(delete2);
}

} // namespace impl

void* operator new(const size_t size)
{
	return impl::new1(size);
}

void* operator new(const size_t size, void *ptr)
{
	return ptr;
}

void* operator new(const size_t size, const bool object, const wchar_t *name, const int type)
{
	return impl::new2(0, size, object, name, type);
}

void operator delete(void *ptr)
{
	impl::delete1(ptr);
}

void operator delete(void *ptr, const int type)
{
	impl::delete2(0, ptr, type);
}

} // namespace xstd

