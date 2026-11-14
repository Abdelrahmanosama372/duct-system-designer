#pragma once
#include "autocad/ui/style/Theme.h"

class UiButton : public CButton
{
    bool m_hover=false, m_primary=false, m_flat=false;
    bool m_selectionComplete=false;
    UiStyle::ButtonRole m_role=UiStyle::ButtonRole::Secondary;
    CFont m_actionFont;
    COLORREF m_surface=UiStyle::Colors(m_hWnd).background;
    void Paint(CDC& dc)
    {
        CRect rect;GetClientRect(&rect);
        const COLORREF surface=UiStyle::ControlSurface(m_hWnd,m_surface);
        const bool checkbox=(GetStyle()&BS_TYPEMASK)==BS_AUTOCHECKBOX;
        const bool down=(GetState()&BST_PUSHED)!=0;
        const bool focus=(GetState()&BST_FOCUS)!=0;
        CString text;GetWindowText(text);
        CFont* font=GetFont();
        if((m_primary || m_role==UiStyle::ButtonRole::Generation) && font)
        {
            if(!m_actionFont.GetSafeHandle())
            {
                LOGFONT value={};font->GetLogFont(&value);value.lfWeight=FW_SEMIBOLD;
                lstrcpyn(value.lfFaceName,Fonts::Face(FW_SEMIBOLD),LF_FACESIZE);m_actionFont.CreateFontIndirect(&value);
            }
            if(m_actionFont.GetSafeHandle())font=&m_actionFont;
        }
        CFont* oldFont=dc.SelectObject(font);
        dc.SetBkMode(TRANSPARENT);
        if(checkbox)
        {
            dc.FillSolidRect(rect,surface);
            const int side=UiStyle::Px(m_hWnd,15);
            CRect box(rect.left+1,rect.CenterPoint().y-side/2,rect.left+side+1,rect.CenterPoint().y+(side+1)/2);
            UiStyle::Rounded(dc,box,UiStyle::Colors(m_hWnd).input,m_hover||focus?UiStyle::Colors(m_hWnd).accent:UiStyle::Colors(m_hWnd).border,UiStyle::Px(m_hWnd,3));
            if(GetCheck()==BST_CHECKED)
            {
                CPen pen(PS_SOLID,max(1,UiStyle::Px(m_hWnd,2)),UiStyle::Colors(m_hWnd).accent);
                CPen* old=dc.SelectObject(&pen);
                dc.MoveTo(box.left+UiStyle::Px(m_hWnd,3),box.CenterPoint().y);
                dc.LineTo(box.left+UiStyle::Px(m_hWnd,6),box.bottom-UiStyle::Px(m_hWnd,4));
                dc.LineTo(box.right-UiStyle::Px(m_hWnd,3),box.top+UiStyle::Px(m_hWnd,4));
                dc.SelectObject(old);
            }
            rect.left=box.right+UiStyle::Px(m_hWnd,10);
            dc.SetTextColor(IsWindowEnabled()?UiStyle::Colors(m_hWnd).bright:UiStyle::Colors(m_hWnd).disabled);
            dc.DrawText(text,rect,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
        }
        else
        {
            const bool enabled=IsWindowEnabled()!=FALSE;
            const bool generation=m_role==UiStyle::ButtonRole::Generation;
            const bool removal=m_role==UiStyle::ButtonRole::Removal;
            const bool primary=m_primary || generation;
            const COLORREF accent=generation?UiStyle::Colors(m_hWnd).green:(removal?UiStyle::Colors(m_hWnd).danger:UiStyle::Colors(m_hWnd).accent);
            const COLORREF base=m_flat?surface:UiStyle::Colors(m_hWnd).panel;
            COLORREF fill=primary?accent:base;
            if(enabled && down)fill=UiStyle::Blend(fill,UiStyle::Colors(m_hWnd).input,20);
            else if(enabled && m_hover)fill=UiStyle::Blend(fill,primary?UiStyle::Colors(m_hWnd).bright:accent,primary?10:12);
            if(!enabled)fill=UiStyle::Colors(m_hWnd).background;
            COLORREF border=m_flat?fill:(!enabled?UiStyle::Colors(m_hWnd).border:(primary||removal||focus?accent:(m_hover?UiStyle::Colors(m_hWnd).muted:UiStyle::Colors(m_hWnd).border)));
            if(enabled && m_selectionComplete)border=UiStyle::Colors(m_hWnd).green;
            dc.FillSolidRect(rect,surface);
            UiStyle::Rounded(dc,rect,fill,border,UiStyle::Px(m_hWnd,UiStyle::ControlRadius));
            // Filled actions use dark text for contrast; removal keeps a restrained red outline.
            dc.SetTextColor(!enabled?UiStyle::Colors(m_hWnd).disabled:
                (primary && UiStyle::Colors(m_hWnd).darkPrimaryText?UiStyle::Colors(m_hWnd).input:UiStyle::Colors(m_hWnd).bright));
            if(focus && enabled && !m_flat)
                dc.FillSolidRect(rect.left+UiStyle::Px(m_hWnd,12),rect.bottom-UiStyle::Px(m_hWnd,3),
                    max(1,rect.Width()-UiStyle::Px(m_hWnd,24)),1,primary?UiStyle::Colors(m_hWnd).bright:accent);
            rect.DeflateRect(UiStyle::Px(m_hWnd,m_flat?2:12),0);
            if(m_flat && !text.IsEmpty() && (text[0]==0x25BC || text[0]==0x25B6))
            {
                const bool expanded=text[0]==0x25BC;
                text.Delete(0,1);text.TrimLeft();
                const int x=rect.left+UiStyle::Px(m_hWnd,7),y=rect.CenterPoint().y;
                const int extent=UiStyle::Px(m_hWnd,4);
                CPen chevron(PS_SOLID,max(1,UiStyle::Px(m_hWnd,2)),focus?UiStyle::Colors(m_hWnd).accent:UiStyle::Colors(m_hWnd).text);
                CPen* oldPen=dc.SelectObject(&chevron);
                if(expanded){dc.MoveTo(x-extent,y-extent/2);dc.LineTo(x,y+extent/2);dc.LineTo(x+extent,y-extent/2);}
                else{dc.MoveTo(x-extent/2,y-extent);dc.LineTo(x+extent/2,y);dc.LineTo(x-extent/2,y+extent);}
                dc.SelectObject(oldPen);rect.left+=UiStyle::Px(m_hWnd,24);
            }
            dc.DrawText(text,rect,((GetStyle()&BS_LEFT)?DT_LEFT:DT_CENTER)|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
        }
        dc.SelectObject(oldFont);
    }
protected:
    afx_msg void OnPaint() { CPaintDC dc(this);Paint(dc); }
    afx_msg LRESULT OnPrintClient(WPARAM wp,LPARAM)
    { CDC* dc=CDC::FromHandle(reinterpret_cast<HDC>(wp));Paint(*dc);return 0; }
    afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
    afx_msg void OnMouseMove(UINT flags,CPoint point)
    {
        if(!m_hover){m_hover=true;TRACKMOUSEEVENT event={sizeof(event),TME_LEAVE,m_hWnd,0};TrackMouseEvent(&event);Invalidate(FALSE);}
        CButton::OnMouseMove(flags,point);
    }
    afx_msg LRESULT OnMouseLeave(WPARAM,LPARAM) { m_hover=false;Invalidate(FALSE);return 0; }
    afx_msg void OnSetFocus(CWnd* old) { CButton::OnSetFocus(old);Invalidate(FALSE); }
    afx_msg void OnKillFocus(CWnd* next) { CButton::OnKillFocus(next);Invalidate(FALSE); }
public:
    afx_msg void OnEnable(BOOL enabled){CButton::OnEnable(enabled);Invalidate(FALSE);}
    void SetFlat(bool value) {m_flat=value;Invalidate(FALSE);}
    void SetSelectionComplete(bool value){m_selectionComplete=value;Invalidate(FALSE);}
    bool SelectionComplete() const{return m_selectionComplete;}
    void SetPrimary(bool value) {m_primary=value;m_role=value?UiStyle::ButtonRole::Primary:UiStyle::ButtonRole::Secondary;Invalidate(FALSE);}
    void SetRole(UiStyle::ButtonRole role){m_role=role;m_primary=role==UiStyle::ButtonRole::Primary;Invalidate(FALSE);}
    UiStyle::ButtonRole Role() const{return m_role;}
    void SetSurface(COLORREF color) {m_surface=color;}
    DECLARE_MESSAGE_MAP()
};
