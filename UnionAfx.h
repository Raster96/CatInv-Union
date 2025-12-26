#ifndef __UNION_AFX_H__
#define __UNION_AFX_H__

#define UNION_NO_WARNINGS
#include ".build_settings.inl"

extern "C" {
  #include <Windows.h>
  #include <stdlib.h>
  #include <stdio.h>
  #include <math.h>
  #include <Psapi.h>
  #include <time.h>
}

#pragma comment (lib, "Psapi.lib")
#pragma comment (lib, "Shw32.lib")
#pragma comment (lib, "Vdfs32g.lib")

#define SHWAPI __declspec (dllimport)
#define ASTAPI __declspec (dllimport)
#define VDFAPI __declspec (dllimport)

#include "Memory\Shw32.h"
#include "Common\Common.h"
#ifndef DONT_USE_UNION_NAMES_AS_DEFAULT
using namespace Common;
#endif

#include "Core\Core.h"
#ifndef DONT_USE_UNION_NAMES_AS_DEFAULT
using namespace UnionCore;
#endif

#include "Vdfs\Vdfs.h"
#ifndef DONT_USE_UNION_NAMES_AS_DEFAULT
using namespace Vdfs32;
#endif

#include "SystemPack\Union.Patch\CPatchFile.h"
#ifndef DONT_USE_UNION_NAMES_AS_DEFAULT
using namespace SystemPack;
#endif

#include "ZenGin\zGothicAPI.h"

#endif // __UNION_AFX_H__
