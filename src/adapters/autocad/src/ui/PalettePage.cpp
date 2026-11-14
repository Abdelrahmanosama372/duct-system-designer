#include "autocad/ui/PalettePage.h"
void PalettePage::LayoutView()
{
    if(!ready_)return;
    CRect rect;GetClientRect(&rect);ContentWindow().MoveWindow(rect,FALSE);
    RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_ALLCHILDREN);
}
int PalettePage::OnCreate(LPCREATESTRUCT cs)
{
    if(CAdUiPalette::OnCreate(cs)==-1)return -1;
    ModifyStyle(0,WS_CLIPCHILDREN|WS_CLIPSIBLINGS);ModifyStyleEx(0,WS_EX_CONTROLPARENT);
    if(!CreateContent())return -1;
    ready_=true;ContentWindow().ShowWindow(SW_SHOW);LayoutView();return 0;
}
void PalettePage::OnSize(UINT type,int cx,int cy){CAdUiPalette::OnSize(type,cx,cy);LayoutView();}
BOOL PalettePage::OnEraseBkgnd(CDC* dc){CRect rect;GetClientRect(&rect);dc->FillSolidRect(rect,UiStyle::Background);return TRUE;}
BOOL PalettePage::GetMinimumSize(CSize& size)
{
    const int dpi=m_hWnd?GetDpiForWindow(m_hWnd):96;
    size=CSize(MulDiv(460,dpi?dpi:96,96),MulDiv(300,dpi?dpi:96,96));return TRUE;
}
bool PalettePage::CanFrameworkTakeFocus(){HWND focus=::GetFocus();return !(focus==m_hWnd || ::IsChild(m_hWnd,focus));}
BOOL PalettePage::PreTranslateMessage(MSG* message)
{
    if(ContentWindow().PreTranslateMessage(message))return TRUE;
    return CAdUiPalette::PreTranslateMessage(message);
}
void PalettePage::OnSetActive(){CAdUiPalette::OnSetActive();ActivateContent();LayoutView();}
BEGIN_MESSAGE_MAP(PalettePage,CAdUiPalette)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()
