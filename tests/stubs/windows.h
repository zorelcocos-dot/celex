#pragma once

// Minimal type-only Windows shim for platform-independent config tests.
// Production builds always use the Windows SDK header.

#include <cstddef>
#include <cstdint>

using HANDLE = void*;
using PVOID = void*;
using SIZE_T = std::size_t;
using PSIZE_T = SIZE_T*;
using DWORD = unsigned long;
using BOOL = int;
