#pragma once
#include "autocad/ui/style/Theme.h"

class UiHeading : public CStatic
{
    bool m_pageTitle=false,m_eyebrow=false,m_plainTitle=false;
    int m_number=0;
    CFont m_badgeFont;
    void Paint(CDC& dc)
    {
        CRect rect;GetClientRect(&rect);dc.FillSolidRect(rect,UiStyle::ControlSurface(m_hWnd));
        CString text;GetWindowText(text);auto old=dc.SelectObject(GetFont());dc.SetBkMode(TRANSPARENT);
        dc.SetTextColor(UiStyle::Colors(m_hWnd).bright);
        if(m_eyebrow)
        {
            dc.SetTextColor(UiStyle::Colors(m_hWnd).accent);int previous=dc.SetTextCharacterExtra(1);
            dc.DrawText(text,rect,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);dc.SetTextCharacterExtra(previous);
        }
        else if(m_number>0)
        {
            const int side=UiStyle::Px(m_hWnd,22);CRect badge(rect.left,rect.CenterPoint().y-side/2,rect.left+side,rect.CenterPoint().y+(side+1)/2);
            UiStyle::Rounded(dc,badge,UiStyle::Colors(m_hWnd).badge,UiStyle::Colors(m_hWnd).badge,UiStyle::Px(m_hWnd,4));
            CString number;number.Format(L"%02d",m_number);dc.SetTextColor(UiStyle::Colors(m_hWnd).accent);
            if(!m_badgeFont.GetSafeHandle())
            {
                LOGFONT font={};GetFont()->GetLogFont(&font);font.lfHeight=-UiStyle::Px(m_hWnd,10);
                m_badgeFont.CreateFontIndirect(&font);
            }
            CFont* titleFont=dc.SelectObject(&m_badgeFont);
            dc.DrawText(number,badge,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);dc.SelectObject(titleFont);
            rect.left+=side+UiStyle::Px(m_hWnd,10);dc.SetTextColor(UiStyle::Colors(m_hWnd).bright);
            dc.DrawText(text,rect,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
        }
        else if(m_pageTitle && !m_plainTitle)
        {
            CRect label=rect;label.bottom-=UiStyle::Px(m_hWnd,12);
            dc.DrawText(text,label,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
            dc.FillSolidRect(rect.left,rect.bottom-1,rect.Width(),1,UiStyle::Colors(m_hWnd).border);
        }
        else
        {
            dc.DrawText(text,rect,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
        }
        dc.SelectObject(old);
    }
    afx_msg void OnPaint(){CPaintDC dc(this);Paint(dc);}
    afx_msg LRESULT OnPrint(WPARAM wp,LPARAM){Paint(*CDC::FromHandle(reinterpret_cast<HDC>(wp)));return 0;}
public:
    void SetPageTitle(){m_pageTitle=true;}
    void SetPlainTitle(){m_plainTitle=true;m_pageTitle=true;}
    void SetNumber(int number){m_number=number;}
    void SetEyebrow(){m_eyebrow=true;}
    DECLARE_MESSAGE_MAP()
};
