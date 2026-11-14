#include "autocad/ui/Designer.h"
#include "autocad/ui/panels/PanelFactory.h"
#include "autocad/ui/style/PaletteStyle.h"
#include <array>
namespace {
void DuctLog(const wchar_t* message){OutputDebugStringW(message);OutputDebugStringW(L"\n");}
static CAdUiPaletteSet* gPaletteSet=nullptr;
using Factory=std::unique_ptr<CAdUiPalette>(*)();
constexpr std::array<Factory,5> factories={MakeProjectInformationPanel,MakeNetworkPanel,MakePropertiesPanel,MakeDrawingGenerationPanel,MakeReportsPanel};
constexpr std::array<const wchar_t*,5> titles={L"Project Information",L"Network",L"Properties",L"Drawing Generation",L"BOQ & Reports"};
std::array<std::unique_ptr<CAdUiPalette>,5> pages;
bool CreatePanel(DuctUi::Panel id)
{
    const auto index=static_cast<size_t>(id);
    pages[index]=factories[index]();
    if(!pages[index]->Create(WS_CHILD|WS_VISIBLE,titles[index],gPaletteSet,0)) {pages[index].reset();return false;}
    if(gPaletteSet->AddPalette(pages[index].get())<0) {pages[index]->DestroyWindow();pages[index].reset();return false;}
    return true;
}
void DestroyPanel(DuctUi::Panel id)
{
    const auto index=static_cast<size_t>(id);
    if(id==DuctUi::Panel::Network)DisconnectNetworkPanel();
    auto& page=pages[index];if(!page)return;
    if(gPaletteSet && gPaletteSet->GetTabIndex(page.get())>=0)gPaletteSet->RemovePalette(page.get());
    if(::IsWindow(page->GetSafeHwnd()))page->DestroyWindow();page.reset();
}


// ============================================================
// Create palette set
// ============================================================

static bool CreateDesignerPalette()
{
    if (gPaletteSet != nullptr)
        return true;

    DuctLog(L"Creating palette set");

    CFrameWnd* pFrame = acedGetAcadFrame();

    if (!pFrame)
    {
        DuctLog(L"ERROR: AutoCAD frame is null");
        return false;
    }

    gPaletteSet = new CAdUiPaletteSet();

    if (!gPaletteSet)
        return false;

    CRect rect(0, 0, 540, 780);

    // Create hidden; FloatControlBar supplies the movable, resizable outer frame.
    if (!gPaletteSet->Create(
        _T("DUCT SYSTEM DESIGNER"),
        WS_CHILD |
        WS_CLIPCHILDREN |
        WS_CLIPSIBLINGS |
        CBRS_TOP |
        CBRS_SIZE_DYNAMIC,
        rect,
        pFrame,
        PSS_PROPERTIES_MENU |
        PSS_CLOSE_BUTTON |
        PSS_SNAP))
    {
        DuctLog(L"ERROR: Palette set creation failed");

        delete gPaletteSet;
        gPaletteSet = nullptr;

        return false;
    }

    if(!PaletteStyle::Apply(*gPaletteSet))
        DuctLog(L"Modern Dark palette frame styling unavailable");

    // Peer tabs appear in the designer workflow order.
    if (!CreatePanel(DuctUi::Panel::ProjectInformation))
        goto failure;
    if (!CreatePanel(DuctUi::Panel::Network))
        goto failure;
    if (!CreatePanel(DuctUi::Panel::Properties))
        goto failure;
    if (!CreatePanel(DuctUi::Panel::DrawingGeneration))
        goto failure;
    if (!CreatePanel(DuctUi::Panel::Reports))
        goto failure;

    // --------------------------------------------------------
    // Docking and resizing
    // --------------------------------------------------------

    DuctLog(L"Enabling palette docking");

    gPaletteSet->EnableDocking(CBRS_ALIGN_ANY);

    gPaletteSet->SetBarStyle(
        gPaletteSet->GetBarStyle() |
        CBRS_SIZE_DYNAMIC
    );

    gPaletteSet->SetActivePalette(0);

    {
        CRuntimeClass* floatingClass =
            AdUiGetRegisteredPaletteSetFloatingFrameClass();

        if (!floatingClass)
        {
            DuctLog(L"ERROR: Palette floating frame class is unavailable");
            goto failure;
        }

        // Place the initial floating window inside the current AutoCAD frame.
        // Do this only on creation so reopening preserves the user's position.
        CPoint origin(100, 100);
        pFrame->ClientToScreen(&origin);
        CRect floatingRect(origin, CSize(540, 780));
        gPaletteSet->InitFloatingPosition(&floatingRect);

        // AutoCAD palettes require their registered floating frame class.
        // Restore the previous class for other AutoCAD control bars.
        CRuntimeClass* previousClass = AdUiSetFloatingFrameClass(floatingClass);
        DuctLog(L"Before FloatControlBar");
        pFrame->FloatControlBar(gPaletteSet, origin, CBRS_ALIGN_TOP);
        DuctLog(L"After FloatControlBar");
        AdUiSetFloatingFrameClass(previousClass);
    }

    DuctLog(L"Palette set initialized");

    return true;

failure:

    DestroyPanel(DuctUi::Panel::ProjectInformation);
    DestroyPanel(DuctUi::Panel::Reports);
    DestroyPanel(DuctUi::Panel::DrawingGeneration);
    DestroyPanel(DuctUi::Panel::Properties);
    DestroyPanel(DuctUi::Panel::Network);

    DuctLog(L"ERROR: Palette initialization failed");

    if (gPaletteSet)
    {
        gPaletteSet->DestroyWindow();

        delete gPaletteSet;
        gPaletteSet = nullptr;
    }
    PaletteStyle::Release();

    return false;
}


// ============================================================
// Show palette
// ============================================================

static void ShowDesignerPalette()
{
    // Extension DLL: retain the host MFC state and switch only resources.
    CAcModuleResourceOverride resources;

    DuctLog(L"ShowDesignerPalette called");

    if (!CreateDesignerPalette())
    {
        acutPrintf(
            _T("\n[Duct System] Failed to create palette.")
        );

        return;
    }

    // Let MFC update both the control bar and its floating frame.
    CFrameWnd* pFrame = acedGetAcadFrame();
    if (!pFrame)
        return;

    DuctLog(L"Before ShowControlBar");
    pFrame->ShowControlBar(gPaletteSet, TRUE, FALSE);
    DuctLog(L"After ShowControlBar");

    DuctLog(L"Before bringing floating frame to top");
    if (CFrameWnd* floatingFrame = gPaletteSet->GetParentFrame())
        floatingFrame->BringWindowToTop();
    DuctLog(L"After bringing floating frame to top");

    DuctLog(L"Palette shown");

    // All designer pages share this movable and dockable palette set.
}


// ============================================================
// Hide palette
// ============================================================

static void HideDesignerPalette()
{
    // Extension DLL: retain the host MFC state and switch only resources.
    CAcModuleResourceOverride resources;

    CFrameWnd* pFrame = acedGetAcadFrame();
    if (gPaletteSet && pFrame)
        pFrame->ShowControlBar(gPaletteSet, FALSE, FALSE);
}


static void HideNetworkPalette(){if(gPaletteSet)gPaletteSet->SetActivePalette(0);}

// ============================================================
// Cleanup
// ============================================================

static void DestroyDesignerPalette()
{
    DuctLog(L"Destroying palette");
    DestroyPanel(DuctUi::Panel::ProjectInformation);
    DestroyPanel(DuctUi::Panel::Reports);
    DestroyPanel(DuctUi::Panel::DrawingGeneration);
    DestroyPanel(DuctUi::Panel::Properties);
    DestroyPanel(DuctUi::Panel::Network);

    if (gPaletteSet)
    {
        if (::IsWindow(gPaletteSet->GetSafeHwnd()))
            gPaletteSet->DestroyWindow();

        delete gPaletteSet;
        gPaletteSet = nullptr;
    }
    PaletteStyle::Release();

    DuctLog(L"Palette cleanup completed");
}



} // namespace
void DuctUi::Show(){ShowDesignerPalette();}
void DuctUi::Hide(){HideDesignerPalette();}
void DuctUi::SelectProjectInformation(){HideNetworkPalette();}
void DuctUi::Shutdown(){DestroyDesignerPalette();}
void DuctUi::Show(Panel panel)
{
    ShowDesignerPalette();
    const auto index=static_cast<size_t>(panel);
    if(gPaletteSet && index<pages.size() && pages[index])gPaletteSet->SetActivePalette(pages[index].get());
}
