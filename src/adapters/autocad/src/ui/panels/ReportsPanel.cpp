#include "autocad/ui/panels/ReportsPanel.h"
#ifndef DUCT_UI_NATIVE_TESTS
#include "autocad/ui/PalettePage.h"
#endif

BEGIN_MESSAGE_MAP(SystemReportsPage,ScrollPage)
    ON_WM_CREATE()
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(ProjectSummaryPage,ScrollPage)
    ON_WM_CREATE()
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(ReportExportPage,ScrollPage)
    ON_WM_CREATE()
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(ReportsContent,TabbedContent)
    ON_WM_CREATE()
END_MESSAGE_MAP()

#ifndef DUCT_UI_NATIVE_TESTS
std::unique_ptr<CAdUiPalette> MakeReportsPanel()
{
    auto page=std::make_unique<ContentPalette<ReportsContent,4900,true>>();
    return page;
}
#endif
