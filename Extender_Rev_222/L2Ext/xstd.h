#pragma once

#include <Windows.h>
#undef max // stupid windows macros
#include <limits>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <utility>

namespace xstd {

	namespace impl {
		extern void* (*new1)(const size_t);
		extern void* (*new2)(void*, const size_t, const bool, const wchar_t*, const int);
		extern void(*delete1)(void*);
		extern void(*delete2)(void*, void*, const int);
		void Setup(const UINT32 new1, const UINT32 new2, const UINT32 delete1, const UINT32 delete2);
	} // namespace impl

	void* operator new(const size_t size);
	void* operator new(const size_t size, void *ptr);
	void* operator new(const size_t size, const bool object, const wchar_t *name, const int type);
	void operator delete(void *ptr);
	void operator delete(void *ptr, const int type);

	template<class Type>
	class allocator {
	public:
		typedef Type value_type;
		typedef Type* pointer;
		typedef const Type* const_pointer;
		typedef Type& reference;
		typedef const Type& const_reference;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

		template<class OtherType>
		struct rebind {
			typedef allocator<OtherType> other;
		};

		pointer address(reference r) {
			return &r;
		}

		const_pointer address(const_reference cr) {
			return &cr;
		}

		allocator() throw() {}
		allocator(const allocator&) throw() {}
		~allocator() throw() {}

		size_type max_size() const throw() {
			return std::numeric_limits<std::size_t>::max() / sizeof(Type);
		}

		pointer allocate(size_type count, const void* = 0) {
			if (count > max_size()) throw std::bad_alloc();
			return reinterpret_cast<pointer>(xstd::operator new(count * sizeof(Type), false, L"xstd", 1));
		}

		void construct(pointer ptr, const_reference cr) {
			new(ptr) Type(cr);
		}

		void destroy(pointer ptr) {
			ptr->~Type();
		}

		void deallocate(pointer ptr, size_type) {
			xstd::operator delete(ptr, 1);
		}
	};

	template<class Type1, class Type2>
	bool operator==(const allocator<Type1>&, const allocator<Type2>&) throw() {
		return true;
	}

	template<class Type1, class Type2>
	bool operator!=(const allocator<Type1>&, const allocator<Type2>&) throw() {
		return false;
	}

	template<class CharT, class Traits = std::char_traits<CharT>, class Allocator = allocator<CharT> >
	class basic_string : public std::basic_string<CharT, Traits, Allocator> {
	public:
		// Constructor predeterminado
		basic_string() : std::basic_string<CharT, Traits, Allocator>() {}

		// Constructor que toma un puntero a const CharT
		basic_string(const CharT *data, const Allocator &alloc = Allocator()) : std::basic_string<CharT, Traits, Allocator>(data, alloc) {}

		bool operator<(const basic_string<CharT, Traits, Allocator> &other) const {
			return wcscmp(this->c_str(), other.c_str()) < 0;
		}
	};

	typedef basic_string<char> string;
	typedef basic_string<wchar_t> wstring;

	template<class Type, class Allocator = allocator<Type> >
	class vector : public std::vector<Type, Allocator> {};

	template<class Type, class Compare = std::less<Type>, class Allocator = allocator<Type> >
	class set : public std::set<Type, Compare, Allocator> {};

	template<class KeyType, class ValueType, class Compare = std::less<KeyType>, class Allocator = allocator<std::pair<KeyType, ValueType> > >
	class map : public std::map<KeyType, ValueType, Compare, Allocator> {};

} // namespace xstd