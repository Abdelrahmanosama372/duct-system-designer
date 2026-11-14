
#ifndef _AFXEXT
#define _AFXEXT
#endif

#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>
#include <afxrich.h>
#include <afxpriv.h>
#include <afxdllx.h>

#include "autocad/ui/Designer.h"
#include "autocad/commands/UiCommands.h"
#include "autocad/ui/style/Theme.h"
#include "autocad/ui/model/NetworkSession.h"
#include "autocad/ui/panels/PanelFactory.h"

#include "adui.h"
#include "aduiMessage.h"

#include "rxregsvc.h"
#include "aced.h"
#include "acutads.h"
#include "rxmfcapi.h"
#include "AcExtensionModule.h"

#include "aduiDock.h"
#include "aduiPalette.h"
#include "aduiPaletteSet.h"

// Share AutoCAD's MFC application state, as required by AdUi control bars.
AC_IMPLEMENT_EXTENSION_MODULE(DuctModule);

extern "C" BOOL WINAPI
DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    UNREFERENCED_PARAMETER(reserved);

    if (reason == DLL_PROCESS_ATTACH)
        return DuctModule.AttachInstance(instance) ? TRUE : FALSE;

    if (reason == DLL_PROCESS_DETACH)
        DuctModule.DetachInstance();

    return TRUE;
}


namespace {
void DuctLog(const wchar_t* message){OutputDebugStringW(message);OutputDebugStringW(L"\n");}

}

extern "C" AcRx::AppRetCode
acrxEntryPoint(AcRx::AppMsgCode msg, void* appId)
{
    switch (msg)
    {
    case AcRx::kInitAppMsg:

        DuctLog(L"ARX initialization started: TestARX4 Designer UI with Project Information");

        InitAdUiDLL();
        if (Fonts::LoadForModule(DuctModule.ModuleResourceInstance()))
            DuctLog(L"Inter Regular, Medium and SemiBold loaded privately");
        else
            DuctLog(L"Some bundled Inter fonts are unavailable; using Segoe UI fallback");
        UiStyle::StartGraphics();

        acrxDynamicLinker->unlockApplication(appId);
        acrxDynamicLinker->registerAppMDIAware(appId);

        Autocad::Commands::RegisterUiCommands();

        DuctLog(L"ARX initialization completed");

        break;

    case AcRx::kUnloadAppMsg:

        DuctLog(L"ARX unloading");

        Autocad::Commands::UnregisterUiCommands();

        NetworkSession::Reset();
        DuctUi::Shutdown();
        ResetProjectInformationState();
        UiStyle::StopGraphics();
        Fonts::Unload();

        break;
    }

    return AcRx::kRetOK;
}
