#include "Signatures.h"

bool Signatures::SearchSignatures(bool NeedDebug)
{
    Unity::il2cppClass* GetUnityCamera = IL2CPP::Class::Find("UnityEngine.Camera");
    Offsets::GetCameraMainOffset = (uintptr_t)IL2CPP::Class::Utils::GetMethodPointer(GetUnityCamera, "get_main");

    //Unity::il2cppClass* NewWeapon = IL2CPP::Class::Find("NewWeapon"); // @TODO: UNCOMMENT ME IF U USING ME!
    //Offsets::SetRecoil = (uintptr_t)IL2CPP::Class::Utils::GetMethodPointer(NewWeapon, "setRecoil"); // @TODO: UNCOMMENT ME IF U USING ME!

    if(NeedDebug)
        Utils::AddressLog(Offsets::GetCameraMainOffset - UnitySDK::UnityGameAssembly, "GetUnityCamera");

    return true;
}
