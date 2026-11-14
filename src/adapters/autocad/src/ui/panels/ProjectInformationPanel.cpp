#include "autocad/ui/panels/ProjectInformationPanel.h"
#ifndef DUCT_UI_NATIVE_TESTS
#include "autocad/ui/PalettePage.h"
#endif

BEGIN_MESSAGE_MAP(ProjectInformationPage,ScrollPage)
    ON_WM_CREATE()
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

#ifndef DUCT_UI_NATIVE_TESTS
std::unique_ptr<CAdUiPalette> MakeProjectInformationPanel()
{
    auto page=std::make_unique<ContentPalette<ProjectInformationPage,5600,false>>();
    return page;
}
#endif
void ResetProjectInformationState(){ProjectInfo::Reset();}
