#pragma once

#ifndef RBX_ENABLE_VMPROTECT
#define RBX_ENABLE_VMPROTECT 0
#endif

#if RBX_ENABLE_VMPROTECT
#if !defined(_WIN32)
#error VMProtect marker calls require a Windows target
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <VMProtect/VMProtectSDK.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace RBX::Security::ProtectionMarkers
{
inline void beginMutation(const char* name = nullptr) noexcept
{
#if RBX_ENABLE_VMPROTECT
    VMProtectBeginMutation(name);
#else
    (void)name;
#endif
}

inline void beginVirtualization(const char* name = nullptr) noexcept
{
#if RBX_ENABLE_VMPROTECT
    VMProtectBeginVirtualization(name);
#else
    (void)name;
#endif
}

inline void end() noexcept
{
#if RBX_ENABLE_VMPROTECT
    VMProtectEnd();
#endif
}

inline bool isDebuggerPresent(bool checkKernelDebuggers = false) noexcept
{
#if RBX_ENABLE_VMPROTECT
    return VMProtectIsDebuggerPresent(checkKernelDebuggers ? TRUE : FALSE) != FALSE;
#elif defined(_WIN32)
    (void)checkKernelDebuggers;
    return ::IsDebuggerPresent() != FALSE;
#else
    (void)checkKernelDebuggers;
    return false;
#endif
}
}
