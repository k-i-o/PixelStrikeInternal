#pragma once
#include "../../IL2CPPResolver/il2cpp_resolver.hpp"
#include "../../IL2CPPResolver/Unity/Structures/Engine.hpp"
#include "../Utils/Offsets.h"

// Put "inline" keyword to every function, regards

namespace GameFunctions {
	inline Unity::CCamera* GetUnityCamera()
	{
		Unity::CCamera* (UNITY_CALLING_CONVENTION GetCameraTemplate)();
		return reinterpret_cast<decltype(GetCameraTemplate)>(Offsets::GetCameraMainOffset)(); 
	}

	//inline void (UNITY_CALLING_CONVENTION RecoilTemplate)(float);
	//inline void RecoilHook(float recoil)
	//{
	//	if (Variables::EnableRecoil)
	//		recoil = Variables::RecoilEdit;

	//	return RecoilTemplate(recoil);
	//}
}