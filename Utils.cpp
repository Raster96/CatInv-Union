#include "UnionAfx.h"
#include <string>
#include <algorithm>

namespace GOTHIC_ENGINE {
    
    // ANSI to UNICODE conversion, via a given codepage
    std::wstring AToW(const std::string& st, UINT cpg, DWORD flags) {
        if (st.empty()) return L"";
        
        int cwch = MultiByteToWideChar(cpg, flags, st.c_str(), -1, nullptr, 0);
        if (cwch == 0) return L"";
        
        std::wstring result(cwch - 1, L'\0');
        MultiByteToWideChar(cpg, flags, st.c_str(), -1, &result[0], cwch);
        
        return result;
    }
    
    // UNICODE to ANSI conversion, via a given codepage
    std::string WToA(const std::wstring& st, UINT cpg, DWORD flags) {
        if (st.empty()) return "";
        
        int cwch = WideCharToMultiByte(cpg, flags, st.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (cwch == 0) return "";
        
        std::string result(cwch - 1, '\0');
        WideCharToMultiByte(cpg, flags, st.c_str(), -1, &result[0], cwch, nullptr, nullptr);
        
        return result;
    }
    
    // Helper function to convert wchar_t to zSTRING
    zSTRING WCharToZString(wchar_t wc) {
        std::wstring ws(1, wc);
        std::string ansi = WToA(ws, CP_ACP);
        return zSTRING(ansi.c_str());
    }
    
    // Compare two strings using locale-aware comparison (for proper sorting)
    int CompareStringsLocaleAware(const char* str1, const char* str2) {
        if (!str1 || !str2) return 0;
        
        std::wstring wstr1 = AToW(str1);
        std::wstring wstr2 = AToW(str2);
        
        // Use CompareStringW with LOCALE_USER_DEFAULT for proper alphabetical sorting
        // NORM_IGNORECASE makes it case-insensitive
        // Returns: 1 (str1 < str2), 2 (str1 == str2), 3 (str1 > str2)
        int result = CompareStringW(
            LOCALE_USER_DEFAULT,
            NORM_IGNORECASE,
            wstr1.c_str(), -1,
            wstr2.c_str(), -1
        );
        
        // Convert to standard comparison result: <0, 0, >0
        return result - 2;
    }
}