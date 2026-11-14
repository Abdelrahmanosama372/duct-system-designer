#pragma once
#include "autocad/ui/style/Theme.h"

class UiComboBox : public CComboBox
{
    bool m_hover=false;
    CBrush m_input{UiStyle::Colors(m_hWnd).input};
    void Paint(CDC& dc)
    {
        CRect rect;GetClientRect(&rect);
        dc.FillSolidRect(rect,UiStyle::ControlSurface(m_hWnd));
        HWND focus=::GetFocus();const bool focused=focus==m_hWnd || ::IsChild(m_hWnd,focus);
        COLORREF border=!IsWindowEnabled()?UiStyle::Colors(m_hWnd).border:(focused?UiStyle::Colors(m_hWnd).accent:(m_hover?UiStyle::Colors(m_hWnd).muted:UiStyle::Colors(m_hWnd).border));
        UiStyle::Rounded(dc,rect,UiStyle::Colors(m_hWnd).input,border,UiStyle::Px(m_hWnd,UiStyle::ControlRadius));
        const int arrowWidth=UiStyle::Px(m_hWnd,26);
        const int x=rect.right-arrowWidth/2, y=rect.CenterPoint().y;
        CPen pen(PS_SOLID,max(1,UiStyle::Px(m_hWnd,1)),IsWindowEnabled()?UiStyle::Colors(m_hWnd).muted:UiStyle::Colors(m_hWnd).disabled);CPen* oldPen=dc.SelectObject(&pen);
        dc.MoveTo(x-UiStyle::Px(m_hWnd,4),y-2);dc.LineTo(x,y+2);dc.LineTo(x+UiStyle::Px(m_hWnd,4),y-2);
        dc.SelectObject(oldPen);
        if((GetStyle()&0x0003)==CBS_DROPDOWNLIST)
        {
            CString text;GetWindowText(text);rect.DeflateRect(UiStyle::Px(m_hWnd,12),UiStyle::Px(m_hWnd,4));rect.right-=arrowWidth;
            CFont* old=dc.SelectObject(GetFont());dc.SetBkMode(TRANSPARENT);dc.SetTextColor(IsWindowEnabled()?UiStyle::Colors(m_hWnd).bright:UiStyle::Colors(m_hWnd).disabled);
            dc.DrawText(text,rect,DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);dc.SelectObject(old);
        }
    }
protected:
    afx_msg void OnPaint() {CPaintDC dc(this);Paint(dc);}
    afx_msg LRESULT OnPrintClient(WPARAM wp,LPARAM)
    {Paint(*CDC::FromHandle(reinterpret_cast<HDC>(wp)));return 0;}
    afx_msg BOOL OnEraseBkgnd(CDC*) {return TRUE;}
    afx_msg HBRUSH OnCtlColor(CDC* dc,CWnd*,UINT)
    {dc->SetBkColor(UiStyle::Colors(m_hWnd).input);dc->SetTextColor(IsWindowEnabled()?UiStyle::Colors(m_hWnd).bright:UiStyle::Colors(m_hWnd).disabled);return m_input;}
    afx_msg void OnSetFocus(CWnd* old) {CComboBox::OnSetFocus(old);Invalidate(FALSE);}
    afx_msg void OnKillFocus(CWnd* next) {CComboBox::OnKillFocus(next);Invalidate(FALSE);}
    afx_msg void OnMouseMove(UINT flags,CPoint point)
    {
        if(!m_hover){m_hover=true;TRACKMOUSEEVENT event={sizeof(event),TME_LEAVE,m_hWnd,0};TrackMouseEvent(&event);Invalidate(FALSE);}
        CComboBox::OnMouseMove(flags,point);
    }
    afx_msg LRESULT OnMouseLeave(WPARAM,LPARAM){m_hover=false;Invalidate(FALSE);return 0;}
    afx_msg void OnEnable(BOOL enabled){CComboBox::OnEnable(enabled);Invalidate(FALSE);}
public:
    void ConfigureSpacing()
    {
        SetItemHeight(-1,UiStyle::Px(m_hWnd,28));
        SetItemHeight(0,UiStyle::Px(m_hWnd,30));
        COMBOBOXINFO info={sizeof(info)};
        if(GetComboBoxInfo(&info) && info.hwndItem)
            ::SendMessage(info.hwndItem,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN,MAKELPARAM(UiStyle::Px(m_hWnd,10),UiStyle::Px(m_hWnd,10)));
    }
    void DrawItem(LPDRAWITEMSTRUCT item) override
    {
        CDC* dc=CDC::FromHandle(item->hDC);CRect rect=item->rcItem;
        const bool selected=(item->itemState&ODS_SELECTED)!=0;
        dc->FillSolidRect(rect,selected?UiStyle::Colors(m_hWnd).accent:UiStyle::Colors(m_hWnd).input);
        if(item->itemID==static_cast<UINT>(-1))return;
        CString text;GetLBText(static_cast<int>(item->itemID),text);
        rect.DeflateRect(UiStyle::Px(m_hWnd,12),UiStyle::Px(m_hWnd,4));
        auto old=dc->SelectObject(GetFont());dc->SetBkMode(TRANSPARENT);
        dc->SetTextColor(!IsWindowEnabled()?UiStyle::Colors(m_hWnd).disabled:(selected?UiStyle::Colors(m_hWnd).input:UiStyle::Colors(m_hWnd).bright));
        dc->DrawText(text,rect,DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);dc->SelectObject(old);
    }
    void MeasureItem(LPMEASUREITEMSTRUCT item) override{item->itemHeight=UiStyle::Px(m_hWnd,30);}
    DECLARE_MESSAGE_MAP()
};
