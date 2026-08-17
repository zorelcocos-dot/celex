#include "MemoryManager.h"
#include <vector>

MemoryManager::~MemoryManager()
{
	detach();
}

int32_t MemoryManager::getProcessId(const std::string& processName) {
	uint32_t processId = 0;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (snapshot == INVALID_HANDLE_VALUE) {
		return processId;
	}

	PROCESSENTRY32 processEntry{};
	processEntry.dwSize = sizeof(PROCESSENTRY32);

	if (Process32First(snapshot, &processEntry)) {
		do {
			if (!_stricmp(processName.c_str(), processEntry.szExeFile)) {
				processId = processEntry.th32ProcessID;
				break;
			}
		} while (Process32Next(snapshot, &processEntry));
	}

	CloseHandle(snapshot);
	return processId;
}

uintptr_t MemoryManager::getModuleAddress(const std::string& moduleName) {
	uintptr_t moduleAddress = 0;
	std::shared_lock lock(handleMutex);

	if (!processHandle) {
		return moduleAddress;
	}

	DWORD processId = GetProcessId(processHandle);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);

	if (snapshot == INVALID_HANDLE_VALUE) {
		return moduleAddress;
	}

	MODULEENTRY32 moduleEntry{};
	moduleEntry.dwSize = sizeof(MODULEENTRY32);

	if (Module32First(snapshot, &moduleEntry)) {
		do {
			if (!_stricmp(moduleName.c_str(), moduleEntry.szModule)) {
				moduleAddress = reinterpret_cast<uintptr_t>(moduleEntry.modBaseAddr);
				break;
			}
		} while (Module32Next(snapshot, &moduleEntry));
	}

	CloseHandle(snapshot);
	return moduleAddress;
}

bool MemoryManager::attachToProcess(const std::string& processName)
{
	detach();

	auto pid = getProcessId(processName);
	if (pid == 0) {
		return false;
	}
	constexpr DWORD requiredAccess = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ |
		PROCESS_VM_WRITE | PROCESS_VM_OPERATION;
	HANDLE process = OpenProcess(requiredAccess, false, pid);

	if (process == INVALID_HANDLE_VALUE || !process) {
		return false;
	}

	{
		std::unique_lock lock(handleMutex);
		processHandle = process;
		processId = pid;
	}

	const uintptr_t moduleAddress = getModuleAddress(processName);
	if (moduleAddress == 0) {
		detach();
		return false;
	}
	setBaseAddress(moduleAddress);

	return true;
}

void MemoryManager::detach()
{
	std::unique_lock lock(handleMutex);
	if (processHandle)
		CloseHandle(processHandle);
	processHandle = nullptr;
	processId = 0;
	baseAddress = 0;
}

bool MemoryManager::isProcessAlive() const
{
	std::shared_lock lock(handleMutex);
	if (!processHandle)
		return false;
	DWORD exitCode = 0;
	return GetExitCodeProcess(processHandle, &exitCode) && exitCode == STILL_ACTIVE;
}

bool MemoryManager::readRaw(uintptr_t address, void* buffer, uintptr_t size) {
	std::shared_lock lock(handleMutex);
	if (!processHandle || !buffer || address == 0 || size == 0)
	{
		failedReads.fetch_add(1, std::memory_order_relaxed);
		return false;
	}
	SIZE_T bytesRead = 0;
	const int32_t status = Luck_ReadVirtualMemory(
		processHandle, reinterpret_cast<void*>(address), buffer, size, &bytesRead);
	if (status < 0 || bytesRead != size)
	{
		failedReads.fetch_add(1, std::memory_order_relaxed);
		return false;
	}
	return true;
}

std::string MemoryManager::readString(uintptr_t address) {
	if (address == 0)
		return "";
	std::string result;
	result.reserve(32);

	// MSVC std::string layout (x64):
	//   +0x00 : union { char _Buf[16]; char* _Ptr; }
	//   +0x10 : size_t _Mysize  (actual string length)
	//   +0x18 : size_t _Myres   (capacity)
	// Reading the length from +0x18 (capacity) instead of +0x10 (size) makes
	// heap strings (>15 chars, e.g. "HumanoidRootPart") include trailing
	// garbage and breaks exact-name lookups. See Offsets::Misc::StringLength.
	int32_t StrLength = read<int32_t>(address + 0x10);
	int32_t StrCapacity = read<int32_t>(address + 0x18);
	// Validate length/capacity to avoid absurd allocations / loops
	if (StrLength < 0 || StrLength > 2048 || StrCapacity < 0 || StrCapacity > 2048)
		return "";
	if (StrLength > StrCapacity)
		return "";

	uintptr_t strAddress = address;
	if (StrLength >= 16) {
		strAddress = read<uintptr_t>(address);
		if (strAddress == 0)
			return "";
		// Limit to StrLength to avoid reading past string
		for (int offset = 0; offset < StrLength; ++offset)
		{
			char character = read<char>(strAddress + offset);
			if (character == 0)
				break;
			result.push_back(character);
			// Safety cap
			if ((int)result.size() >= StrLength)
				break;
		}
		return result;
	}

	// SSO - small string stored inline
	for (int offset = 0; offset < StrLength; ++offset)
	{
		char character = read<char>(strAddress + offset);
		if (character == 0)
			break;
		result.push_back(character);
		if ((int)result.size() > 64)
			break;
	}

	return result;
}

int32_t MemoryManager::getProcessId() const {
	std::shared_lock lock(handleMutex);
	return processId;
}

void MemoryManager::setProcessId(int32_t newProcessId) {
	std::unique_lock lock(handleMutex);
	processId = newProcessId;
}

uintptr_t MemoryManager::getBaseAddress() const {
	std::shared_lock lock(handleMutex);
	return baseAddress;
}

void MemoryManager::setBaseAddress(uintptr_t newBaseAddress) {
	std::unique_lock lock(handleMutex);
	baseAddress = newBaseAddress;
}

uint64_t MemoryManager::getFailedReadCount() const
{
	return failedReads.load(std::memory_order_relaxed);
}

uint64_t MemoryManager::getFailedWriteCount() const
{
	return failedWrites.load(std::memory_order_relaxed);
}