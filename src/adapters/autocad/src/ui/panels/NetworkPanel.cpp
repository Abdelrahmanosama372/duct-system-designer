#include "autocad/ui/panels/NetworkPanel.h"
#ifndef DUCT_UI_NATIVE_TESTS
#include "autocad/ui/PalettePage.h"
#endif

BEGIN_MESSAGE_MAP(NetworkRegistrationPage,ScrollPage)
    ON_MESSAGE(WM_DSD_NETWORK_UPDATED,OnNetworkUpdated)
    ON_WM_CREATE()
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(RegisteredNetworksPage,ScrollPage)
    ON_WM_CREATE()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(NetworkContent,CWnd)
    ON_MESSAGE(WM_PRINT,OnPrint)
    ON_MESSAGE(WM_UI_VIEW_SCROLLED,OnPageScrolled)
    ON_WM_MOUSEWHEEL()
    ON_WM_VSCROLL()
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
    ON_NOTIFY(TCN_SELCHANGE,2400,OnViewChanged)
END_MESSAGE_MAP()

static NetworkContent* activeNetworkContent=nullptr;
void RefreshNetworkCards(){if(activeNetworkContent)activeNetworkContent->RefreshCards();}
void EditNetworkDraft(size_t index){if(activeNetworkContent)activeNetworkContent->EditDraft(index);}
#ifndef DUCT_UI_NATIVE_TESTS
std::unique_ptr<CAdUiPalette> MakeNetworkPanel()
{
    auto page=std::make_unique<ContentPalette<NetworkContent,2500,false>>();
    activeNetworkContent=&page->Content();
    return page;
}
#endif
void DisconnectNetworkPanel(){activeNetworkContent=nullptr;}
