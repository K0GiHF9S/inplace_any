#pragma once

#include "type_traits.h"

inline void* operator new(lib::size_t, void* ptr) noexcept { return ptr; }
inline void* operator new[](lib::size_t, void* ptr) noexcept { return ptr; }
inline void operator delete(void*, void* ) noexcept {}
inline void operator delete[](void*, void* ) noexcept {}
