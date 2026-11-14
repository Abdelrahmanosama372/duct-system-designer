#include "autocad/ui/layout/TabbedContent.h"

BEGIN_MESSAGE_MAP(TabbedContent,CWnd)
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
    ON_WM_MOUSEWHEEL()
    ON_WM_VSCROLL()
    ON_MESSAGE(WM_UI_VIEW_SCROLLED,OnPageScrolled)
    ON_MESSAGE(WM_PRINT,OnPrint)
END_MESSAGE_MAP()
