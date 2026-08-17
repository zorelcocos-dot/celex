#pragma once

#include <windows.h>
#include <tlhelp32.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>

extern "C" int32_t Luck_ReadVirtualMemory(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer,
    SIZE_T NumberOfBytesToRead,
    PSIZE_T NumberOfBytesRead);

extern "C" int32_t Luck_WriteVirtualMemory(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer,
    SIZE_T NumberOfBytesToWrite,
    PSIZE_T NumberOfBytesWritten);

class MemoryManager final
{
private:
    mutable std::shared_mutex handleMutex;
    HANDLE processHandle = nullptr;
    int32_t processId = 0;
    uintptr_t baseAddress = 0;
    std::atomic_uint64_t failedReads = 0;
    std::atomic_uint64_t failedWrites = 0;

public:
    MemoryManager() = default;
    ~MemoryManager();

    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    int32_t getProcessId(const std::string& processName);
    uintptr_t getModuleAddress(const std::string& moduleName);

    bool attachToProcess(const std::string& processName);
    void detach();
    bool isProcessAlive() const;

    bool readRaw(uintptr_t address, void* buffer, uintptr_t size);
    std::string readString(uintptr_t address);

    template <typename T>
    T read(uintptr_t address);

    template <typename T>
    bool write(uintptr_t address, const T& value);

    int32_t getProcessId() const;
    void setProcessId(int32_t newProcessId);

    uintptr_t getBaseAddress() const;
    void setBaseAddress(uintptr_t newBaseAddress);

    uint64_t getFailedReadCount() const;
    uint64_t getFailedWriteCount() const;
};

template <typename T>
T MemoryManager::read(uintptr_t address)
{
    T buffer{};
    std::shared_lock lock(handleMutex);
    if (!processHandle || address == 0)
    {
        failedReads.fetch_add(1, std::memory_order_relaxed);
        return buffer;
    }

    SIZE_T bytesRead = 0;
    const int32_t status = Luck_ReadVirtualMemory(
        processHandle,
        reinterpret_cast<void*>(address),
        &buffer,
        sizeof(T),
        &bytesRead);
    if (status < 0 || bytesRead != sizeof(T))
    {
        failedReads.fetch_add(1, std::memory_order_relaxed);
        return T{};
    }
    return buffer;
}

template <typename T>
bool MemoryManager::write(uintptr_t address, const T& value)
{
    std::shared_lock lock(handleMutex);
    if (!processHandle || address == 0)
    {
        failedWrites.fetch_add(1, std::memory_order_relaxed);
        return false;
    }

    SIZE_T bytesWritten = 0;
    const int32_t status = Luck_WriteVirtualMemory(
        processHandle,
        reinterpret_cast<void*>(address),
        const_cast<T*>(&value),
        sizeof(T),
        &bytesWritten);
    if (status < 0 || bytesWritten != sizeof(T))
    {
        failedWrites.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    return true;
}

inline std::unique_ptr<MemoryManager> Memory = std::make_unique<MemoryManager>();
