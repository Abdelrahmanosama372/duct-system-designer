#pragma once
#include "autocad/ui/style/Theme.h"

class UiEdit : public CEdit
{
    CString m_placeholder;
    void Placeholder(CDC& dc)
    {
        if(m_placeholder.IsEmpty() || GetWindowTextLength()!=0)return;
        CRect rect;GetClientRect(&rect);auto old=dc.SelectObject(GetFont());
        dc.SetBkMode(TRANSPARENT);dc.SetTextColor(UiStyle::Colors(m_hWnd).muted);
        dc.DrawText(m_placeholder,rect,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);dc.SelectObject(old);
    }

    void Border(CDC& dc,bool excludeClient)
    {
        CRect window,client;GetWindowRect(&window);GetClientRect(&client);
        ClientToScreen(&client);client.OffsetRect(-window.left,-window.top);
        window.OffsetRect(-window.TopLeft());
        const int saved=dc.SaveDC();if(excludeClient)dc.ExcludeClipRect(client);
        dc.FillSolidRect(window,UiStyle::ControlSurface(m_hWnd));
        UiStyle::Rounded(dc,window,UiStyle::Colors(m_hWnd).input,GetFocus()==this?UiStyle::Colors(m_hWnd).accent:UiStyle::Colors(m_hWnd).border,UiStyle::Px(m_hWnd,6));
        dc.RestoreDC(saved);
    }
protected:
    afx_msg void OnNcCalcSize(BOOL calc,NCCALCSIZE_PARAMS* params)
    {
        CEdit::OnNcCalcSize(calc,params);
        CRect rect=params->rgrc[0];
        if(rect.Height()>UiStyle::Px(m_hWnd,24) && rect.Width()>UiStyle::Px(m_hWnd,24))
        {
            rect.DeflateRect(UiStyle::Px(m_hWnd,11),UiStyle::Px(m_hWnd,6));params->rgrc[0]=rect;
        }
    }
    afx_msg void OnPaint(){Default();CClientDC dc(this);Placeholder(dc);}
    afx_msg void OnNcPaint() {CWindowDC dc(this);Border(dc,true);}
    afx_msg void OnSetFocus(CWnd* old) {CEdit::OnSetFocus(old);RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_FRAME);}
    afx_msg void OnKillFocus(CWnd* next) {CEdit::OnKillFocus(next);RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_FRAME);}
    afx_msg LRESULT OnPrint(WPARAM wp,LPARAM flags)
    {
        CDC* dc=CDC::FromHandle(reinterpret_cast<HDC>(wp));Border(*dc,false);
        CRect window,client;GetWindowRect(&window);GetClientRect(&client);ClientToScreen(&client);
        client.OffsetRect(-window.left,-window.top);
        const int saved=dc->SaveDC();dc->OffsetViewportOrg(client.left,client.top);
        dc->IntersectClipRect(0,0,client.Width(),client.Height());
        LRESULT result=DefWindowProc(WM_PRINTCLIENT,wp,flags|PRF_CLIENT);Placeholder(*dc);dc->RestoreDC(saved);return result;
    }
public:
    void SetPlaceholder(LPCTSTR text)
    {
        // Native cue banners and custom placeholders must never paint together.
        SendMessage(EM_SETCUEBANNER,FALSE,reinterpret_cast<LPARAM>(L""));
        m_placeholder=text;Invalidate(FALSE);
    }
    DECLARE_MESSAGE_MAP()
};
