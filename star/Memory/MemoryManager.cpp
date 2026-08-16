#include "MemoryManager.h"
#include <vector>

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
	auto pid = getProcessId(processName);
	if (pid == 0) {
		return false;
	}
	HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, false, pid);

	if (process == INVALID_HANDLE_VALUE || !process) {
		return false;
	}

	processHandle = process;
	processId = pid;

	baseAddress = getModuleAddress(processName);
	if (baseAddress == 0) {
		// Base address not found - still consider attached but log
		CloseHandle(process);
		processHandle = nullptr;
		processId = 0;
		return false;
	}

	return true;
}


void MemoryManager::readRaw(uintptr_t address, void* buffer, uintptr_t size) {
	if (!processHandle || !buffer || address == 0 || size == 0)
		return;
	Luck_ReadVirtualMemory(processHandle, reinterpret_cast<void*>(address), buffer, size, nullptr);
}

std::string MemoryManager::readString(uintptr_t address) {
	if (address == 0 || !processHandle)
		return "";
	std::string result;
	result.reserve(32);

	int32_t StrLength = read<int32_t>(address + 0x18);
	// Validate length to avoid absurd allocations / loops
	if (StrLength < 0 || StrLength > 2048)
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

int32_t MemoryManager::getProcessId() {
	return processId;
}

void MemoryManager::setProcessId(int32_t newProcessId) {
	processId = newProcessId;
}

uintptr_t MemoryManager::getBaseAddress() {
	return baseAddress;
}

void MemoryManager::setBaseAddress(uintptr_t newBaseAddress) {
	baseAddress = newBaseAddress;
}