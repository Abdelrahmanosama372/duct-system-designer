#include "autocad/ui/layout/ScrollPage.h"

BEGIN_MESSAGE_MAP(ScrollPage,CWnd)
    ON_MESSAGE(WM_UI_SCROLL_TO,OnRailScroll)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_VSCROLL()
    ON_WM_MOUSEWHEEL()
    ON_WM_ERASEBKGND()
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

