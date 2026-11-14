#pragma once
#include "autocad/ui/platform/Mfc.h"
#include <array>
#include <string>

// Load bundled Inter fonts only into the AutoCAD process, never into Windows.
// Windows' legacy family names differ for the three static OpenType weights.
namespace Fonts
{
    inline constexpr const wchar_t* Files[]={L"Inter-Regular.otf",L"Inter-Medium.otf",L"Inter-SemiBold.otf"};
    inline constexpr const wchar_t* Families[]={L"Inter",L"Inter Medium",L"Inter SemiBold"};
    inline std::array<std::wstring,3> paths;
    inline std::array<bool,3> loaded{};
    inline bool initialized=false;

    inline bool AllLoaded(){return loaded[0] && loaded[1] && loaded[2];}
    inline const wchar_t* Face(int weight)
    {
        const int index=weight>=FW_SEMIBOLD?2:(weight>=FW_MEDIUM?1:0);
        return loaded[index]?Families[index]:L"Segoe UI";
    }
    inline bool LoadDirectory(const std::wstring& directory)
    {
        if(initialized)return AllLoaded();
        initialized=true;
        std::wstring base=directory;
        if(!base.empty() && base.back()!=L'\\' && base.back()!=L'/')base+=L'\\';
        for(size_t i=0;i<loaded.size();++i)
        {
            paths[i]=base+Files[i];
            loaded[i]=AddFontResourceExW(paths[i].c_str(),FR_PRIVATE,nullptr)>0;
        }
        return AllLoaded();
    }
    inline bool LoadForModule(HINSTANCE module)
    {
        std::wstring file(32768,L'\0');
        const DWORD length=GetModuleFileNameW(module,file.data(),static_cast<DWORD>(file.size()));
        if(length==0 || length>=file.size())return false;
        file.resize(length);
        const auto separator=file.find_last_of(L"\\/");
        if(separator==std::wstring::npos)return false;
        return LoadDirectory(file.substr(0,separator+1)+L"fonts");
    }
    inline void Unload()
    {
        // The caller must destroy all UI windows and their HFONTs first.
        for(size_t i=0;i<loaded.size();++i)
        {
            if(loaded[i])RemoveFontResourceExW(paths[i].c_str(),FR_PRIVATE,nullptr);
            loaded[i]=false;paths[i].clear();
        }
        initialized=false;
    }
}
