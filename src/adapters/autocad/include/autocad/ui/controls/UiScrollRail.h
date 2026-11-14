#pragma once
#include "autocad/ui/style/Theme.h"

inline constexpr UINT WM_UI_SCROLL_TO=WM_APP+71;
class UiScrollRail : public CWnd
{
    int m_content=0,m_view=0,m_position=0;
    bool m_dragging=false,m_hover=false;
    int m_grabOffset=0;
    CRect Track() const
    {
        CRect rect;GetClientRect(&rect);rect.DeflateRect(0,UiStyle::Px(m_hWnd,8));
        if(rect.bottom<rect.top)rect.bottom=rect.top;
        return rect;
    }
    CRect Thumb() const
    {
        CRect track=Track();
        if(m_content<=m_view || track.Height()<=0)return CRect(0,0,0,0);
        const int height=min(track.Height(),max(UiStyle::Px(m_hWnd,32),MulDiv(track.Height(),m_view,max(1,m_content))));
        const int travel=track.Height()-height;
        const int y=track.top+MulDiv(travel,m_position,max(1,m_content-m_view));
        const int width=UiStyle::Px(m_hWnd,m_hover||m_dragging?8:6);
        const int x=track.CenterPoint().x-width/2;
        return CRect(x,y,x+width,y+height);
    }
    void Request(int position)
    {
        const int next=max(0,min(position,max(0,m_content-m_view)));
        GetParent()->SendMessage(WM_UI_SCROLL_TO,static_cast<WPARAM>(next));
    }
    void Paint(CDC& dc)
    {
        CRect rect;GetClientRect(&rect);dc.FillSolidRect(rect,UiStyle::Colors(m_hWnd).background);
        CRect thumb=Thumb();if(thumb.IsRectEmpty())return;
        const COLORREF color=m_dragging||m_hover?UiStyle::Colors(m_hWnd).muted:UiStyle::Colors(m_hWnd).border;
        UiStyle::Rounded(dc,thumb,color,color,max(1,thumb.Width()/2));
    }
protected:
    afx_msg void OnPaint(){CPaintDC dc(this);Paint(dc);}
    afx_msg BOOL OnEraseBkgnd(CDC*){return TRUE;}
    afx_msg LRESULT OnPrint(WPARAM wp,LPARAM){Paint(*CDC::FromHandle(reinterpret_cast<HDC>(wp)));return 0;}
    afx_msg void OnLButtonDown(UINT,CPoint point)
    {
        CRect thumb=Thumb();
        if(thumb.PtInRect(point))
        {m_dragging=true;m_grabOffset=point.y-thumb.top;SetCapture();Invalidate(FALSE);}
        else Request(m_position+(point.y<thumb.top?-m_view:m_view));
    }
    afx_msg void OnLButtonUp(UINT,CPoint)
    {
        if(m_dragging){m_dragging=false;if(GetCapture()==this)ReleaseCapture();Invalidate(FALSE);}
    }
    afx_msg void OnMouseMove(UINT,CPoint point)
    {
        if(m_dragging)
        {
            CRect track=Track(),thumb=Thumb();
            const int travel=track.Height()-thumb.Height();
            if(travel>0)Request(MulDiv(max(0,min(point.y-m_grabOffset-track.top,travel)),m_content-m_view,travel));
            return;
        }
        const bool hover=Thumb().PtInRect(point);
        if(hover!=m_hover){m_hover=hover;Invalidate(FALSE);}
        TRACKMOUSEEVENT event={sizeof(event),TME_LEAVE,m_hWnd,0};TrackMouseEvent(&event);
    }
    afx_msg LRESULT OnMouseLeave(WPARAM,LPARAM){m_hover=false;Invalidate(FALSE);return 0;}
    afx_msg void OnCaptureChanged(CWnd*){m_dragging=false;Invalidate(FALSE);}
    afx_msg BOOL OnMouseWheel(UINT flags,short delta,CPoint point)
    {
        GetParent()->SendMessage(WM_MOUSEWHEEL,MAKEWPARAM(flags,static_cast<WORD>(delta)),MAKELPARAM(point.x,point.y));return TRUE;
    }
public:
    BOOL Create(CWnd* parent)
    {
        LPCTSTR name=AfxRegisterWndClass(0,::LoadCursor(nullptr,IDC_ARROW),nullptr,nullptr);
        return CreateEx(0,name,L"Network scrollbar",WS_CHILD|WS_CLIPSIBLINGS,CRect(0,0,1,1),parent,2900);
    }
    void SetMetrics(int content,int view,int position)
    {
        m_content=content;m_view=view;m_position=position;Invalidate(FALSE);
    }
    DECLARE_MESSAGE_MAP()
};
