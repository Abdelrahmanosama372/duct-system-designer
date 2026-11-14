#include "autocad/ui/panels/PropertiesPanel.h"
#ifndef DUCT_UI_NATIVE_TESTS
#include "autocad/ui/PalettePage.h"
#endif

BEGIN_MESSAGE_MAP(SegmentPropertiesPage,ScrollPage)
    ON_WM_CREATE()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(EquipmentPropertiesPage,ScrollPage)
    ON_WM_CREATE()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(PropertiesContent,TabbedContent)
    ON_WM_CREATE()
END_MESSAGE_MAP()

#ifndef DUCT_UI_NATIVE_TESTS
std::unique_ptr<CAdUiPalette> MakePropertiesPanel()
{
    auto page=std::make_unique<ContentPalette<PropertiesContent,3300,true>>();
    return page;
}
#endif
