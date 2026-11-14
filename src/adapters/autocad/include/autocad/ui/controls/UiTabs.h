#pragma once
#include "autocad/ui/style/Theme.h"

class UiTabs : public CTabCtrl
{
    int m_hover=-1;
    COLORREF m_surface=UiStyle::Colors(m_hWnd).background;
    void Paint(CDC& dc)
    {
        CRect client;GetClientRect(&client);dc.FillSolidRect(client,m_surface);
        CFont* old=dc.SelectObject(GetFont());dc.SetBkMode(TRANSPARENT);
        for(int i=0;i<GetItemCount();++i)
        {
            CRect item;GetItemRect(i,&item);const bool active=GetCurSel()==i;
            item.right-=UiStyle::Px(m_hWnd,4);
            item.top=max(0,item.top-UiStyle::Px(m_hWnd,2));
            item.bottom=min(client.bottom,item.top+UiStyle::Px(m_hWnd,32));
            COLORREF fill=active?UiStyle::Colors(m_hWnd).accent:UiStyle::Colors(m_hWnd).panel;
            if(m_hover==i && !active)fill=UiStyle::Blend(fill,UiStyle::Colors(m_hWnd).accent,15);
            UiStyle::Rounded(dc,item,fill,fill,UiStyle::Px(m_hWnd,6));
            wchar_t text[128]={};TCITEM value={};value.mask=TCIF_TEXT;value.pszText=text;value.cchTextMax=128;GetItem(i,&value);
            dc.SetTextColor(!IsWindowEnabled()?UiStyle::Colors(m_hWnd).disabled:
                (active && UiStyle::Colors(m_hWnd).darkPrimaryText?UiStyle::Colors(m_hWnd).input:UiStyle::Colors(m_hWnd).bright));
            dc.DrawText(text,item,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
            if(active && ::GetFocus()==m_hWnd)
                dc.FillSolidRect(item.left+UiStyle::Px(m_hWnd,10),item.bottom-UiStyle::Px(m_hWnd,4),
                    max(1,item.Width()-UiStyle::Px(m_hWnd,20)),1,UiStyle::Colors(m_hWnd).bright);
        }
        dc.SelectObject(old);
    }
    afx_msg void OnPaint(){CPaintDC dc(this);Paint(dc);}
    afx_msg BOOL OnEraseBkgnd(CDC*){return TRUE;}
    afx_msg LRESULT OnPrintClient(WPARAM wp,LPARAM){Paint(*CDC::FromHandle(reinterpret_cast<HDC>(wp)));return 0;}
    afx_msg void OnMouseMove(UINT flags,CPoint point)
    {
        TCHITTESTINFO hit={};hit.pt=point;const int hover=HitTest(&hit);
        if(hover!=m_hover){m_hover=hover;Invalidate(FALSE);}
        TRACKMOUSEEVENT event={sizeof(event),TME_LEAVE,m_hWnd,0};TrackMouseEvent(&event);CTabCtrl::OnMouseMove(flags,point);
    }
    afx_msg LRESULT OnMouseLeave(WPARAM,LPARAM){m_hover=-1;Invalidate(FALSE);return 0;}
    afx_msg void OnSetFocus(CWnd* old){CTabCtrl::OnSetFocus(old);Invalidate(FALSE);}
    afx_msg void OnKillFocus(CWnd* next){CTabCtrl::OnKillFocus(next);Invalidate(FALSE);}
public:
    void SetSurface(COLORREF color){m_surface=color;Invalidate(FALSE);}
    void SetFont(CFont* font,BOOL redraw=TRUE)
    {
        CTabCtrl::SetFont(font,redraw);
        SetPadding(CSize(UiStyle::Px(m_hWnd,12),UiStyle::Px(m_hWnd,7)));
        SetItemSize(CSize(0,UiStyle::Px(m_hWnd,32)));
    }
    DECLARE_MESSAGE_MAP()
};
