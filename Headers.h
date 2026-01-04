#pragma once

#define PLUGIN_NAME "CatInv"

#include "CatInvConst.h"
#include "CatInvCore.h"
#include "CatInvOptions.h"

namespace GOTHIC_ENGINE {
    std::wstring AToW(const std::string& st, UINT cpg = CP_ACP, DWORD flags = 0);
    std::string WToA(const std::wstring& st, UINT cpg = CP_ACP, DWORD flags = 0);
    zSTRING WCharToZString(wchar_t wc);
}