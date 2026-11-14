#include "autocad/ui/panels/DrawingGenerationPanel.h"
#ifndef DUCT_UI_NATIVE_TESTS
#include "autocad/ui/PalettePage.h"
#endif

BEGIN_MESSAGE_MAP(DrawingGenerationPage,ScrollPage)
    ON_WM_CREATE()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(DrawingGenerationContent,TabbedContent)
    ON_WM_CREATE()
END_MESSAGE_MAP()

#ifndef DUCT_UI_NATIVE_TESTS
std::unique_ptr<CAdUiPalette> MakeDrawingGenerationPanel()
{
    auto page=std::make_unique<ContentPalette<DrawingGenerationContent,6700,true>>();
    return page;
}
#endif
