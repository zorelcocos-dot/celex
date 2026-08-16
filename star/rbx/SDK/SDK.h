#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>

#include "../offsets.h"
#include "../../rbx/math/math.h"
#include "../../Memory/MemoryManager.h"

class RobloxInstance
{
public:
	uintptr_t address;

	RobloxInstance(uintptr_t addy)
	{
		address = addy;
	}

	operator bool() const
	{
		return address != 0;
	}

	inline std::string Name() const
	{
		if (address == 0) return "";
		uintptr_t namePtr = Memory->read<uintptr_t>(address + Offsets::Instance::Name);
		if (namePtr == 0) return "";
		return Memory->readString(namePtr);
	}

	inline std::string Class() const
	{
		if (address == 0) return "";
		uintptr_t classDesc = Memory->read<uintptr_t>(address + Offsets::Instance::ClassDescriptor);
		if (classDesc == 0) return "";
		uintptr_t classNamePtr = Memory->read<uintptr_t>(classDesc + Offsets::Instance::ClassName);
		if (classNamePtr == 0) return "";
		return Memory->readString(classNamePtr);
	}

	inline bool IsA(std::string className) const
	{
		if (address == 0) return false;
		return Class() == className;
	}

	inline std::vector<RobloxInstance> GetChildren() const
	{
		std::vector<RobloxInstance> returnVector;
		if (address == 0) return returnVector;
		uintptr_t childrenStart = Memory->read<uintptr_t>(address + Offsets::Instance::ChildrenStart);
		if (childrenStart == 0) return returnVector;
		// Offsets::Instance::ChildrenEnd is offset inside the children container structure
		uintptr_t childrenEnd = Memory->read<uintptr_t>(childrenStart + Offsets::Instance::ChildrenEnd);
		if (childrenEnd == 0 || childrenEnd <= childrenStart) return returnVector;
		uintptr_t childrenArray = Memory->read<uintptr_t>(childrenStart);
		if (childrenArray == 0) return returnVector;
		// Sanity: limit to avoid absurd iteration on corrupted memory
		const size_t maxChildren = 5000;
		size_t count = (childrenEnd - childrenArray) / 0x10;
		if (count > maxChildren) return returnVector;

		for (uintptr_t child = childrenArray; child < childrenEnd; child += 0x10)
		{
			uintptr_t childPtr = Memory->read<uintptr_t>(child);
			if (childPtr == 0) continue;
			returnVector.emplace_back(RobloxInstance(childPtr));
		}

		return returnVector;
	}

	inline RobloxInstance FindFirstChild(std::string name = "") const
	{
		for (auto& child : this->GetChildren())
		{
			if (!child.address) continue;
			if (name == "")
				return child;

			// Avoid expensive Name() read if string comparison likely fails quickly?
			if (child.Name() == name)
				return child;
		}
		return RobloxInstance(0);
	}

	inline RobloxInstance FindFirstChildWhichIsA(std::string className = "") const
	{
		if (className.empty()) return RobloxInstance(0);
		for (auto& child : this->GetChildren())
		{
			if (!child.address) continue;
			if (child.Class() == className)
				return child;
		}
		return RobloxInstance(0);
	}

	inline Vectors::Vector3 Position() const
	{
		if (address == 0) return Vectors::Vector3{0,0,0};
		uintptr_t prim = Memory->read<uintptr_t>(address + Offsets::BasePart::Primitive);
		if (prim == 0) return Vectors::Vector3{0,0,0};
		return Memory->read<Vectors::Vector3>(prim + Offsets::Primitive::Position);
	}

	inline Vectors::Vector3 Size() const
	{
		if (address == 0) return Vectors::Vector3{0,0,0};
		uintptr_t prim = Memory->read<uintptr_t>(address + Offsets::BasePart::Primitive);
		if (prim == 0) return Vectors::Vector3{0,0,0};
		return Memory->read<Vectors::Vector3>(prim + Offsets::Primitive::Size);
	}

	inline sCFrame CFrame() const
	{
		if (address == 0) return sCFrame{};
		if (Class() == "Camera")
		{
			auto rotation = Memory->read<Matrixes::Matrix3x3>(address + Offsets::Camera::Rotation);
			auto position = Memory->read<Vectors::Vector3>(address + Offsets::Camera::Position);

			sCFrame newCFrame
			{
				rotation.r00, rotation.r01, rotation.r02,
				rotation.r10, rotation.r11, rotation.r12,
				rotation.r20, rotation.r21, rotation.r22,
				position.x, position.y, position.z
			};
			return newCFrame;
		}
		else
		{
			uintptr_t primitiveAddr = Memory->read<uintptr_t>(address + Offsets::BasePart::Primitive);
			if (primitiveAddr == 0) return sCFrame{};
			return Memory->read<sCFrame>(primitiveAddr + Offsets::Primitive::Rotation);
		}
	}

	inline RobloxInstance Character() const
	{
		return RobloxInstance(Memory->read<uintptr_t>(address + Offsets::Player::ModelInstance));
	}

	inline float Health() const
	{
		auto character = Character();
		auto humanoid = character.FindFirstChildWhichIsA("Humanoid");

		return Memory->read<float>(humanoid.address + Offsets::Humanoid::Health);
	}

	inline float MaxHealth() const
	{
		auto character = Character();
		auto humanoid = character.FindFirstChildWhichIsA("Humanoid");

		return Memory->read<float>(humanoid.address + Offsets::Humanoid::MaxHealth);
	}

	inline RobloxInstance Team() const
	{
		return RobloxInstance(Memory->read<uintptr_t>(address + Offsets::Player::Team));
	}

	inline int RigType() const
	{
		return Memory->read<int>(address + Offsets::Humanoid::RigType);
	}

	inline void SetWalkspeed(float value)
	{
		value = std::round(value);
		Memory->write(address + Offsets::Humanoid::WalkspeedCheck, value);
		Memory->write(address + Offsets::Humanoid::Walkspeed, value);
	}

	inline void SetJumpPower(float value)
	{
		value = std::round(value);
		Memory->write(address + Offsets::Humanoid::JumpPower, value);
	}

	inline float GetWalkspeed()
	{
		return std::round(Memory->read<float>(address + Offsets::Humanoid::Walkspeed));
	}

	inline float GetJumpPower()
	{
		return std::round(Memory->read<float>(address + Offsets::Humanoid::JumpPower));
	}

	inline float GetFOV()
	{
		auto radiantsFOV = Memory->read<float>(address + Offsets::Camera::FieldOfView);

		auto degreesFOV = radiantsFOV * 180 / 3.1415926535;

		return std::round(degreesFOV);

	}

	inline void SetFOV(float value)
	{
		value = std::round(value);

		auto radiantsValue = value * 3.1415926535 / 180;

		Memory->write<float>(address + Offsets::Camera::FieldOfView, radiantsValue);
	}

};