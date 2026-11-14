#pragma once
#include "autocad/ui/controls/Controls.h"

constexpr UINT WM_UI_VIEW_SCROLLED=WM_APP+72;
class ScrollPage : public CWnd
{
protected:
    CFont m_font, m_bold, m_title, m_cardTitle, m_value, m_buttonFont, m_sectionFont;
    CBrush m_background, m_input, m_card, m_section;
    std::vector<CRect> m_sections;
    UiScrollRail m_scrollRail;
    std::vector<std::unique_ptr<CWnd>> m_controls;
    int m_dpi = 96, m_scroll = 0, m_contentHeight = 0, m_leadingSpace=0;
    struct Placement{HWND window;CRect rect;};
    std::vector<Placement> m_placements;
    bool m_layoutReady = false, m_inLayout = false, m_presentationReady = false;

    int Px(int value) const { return MulDiv(value, m_dpi, 96); }
    int Units(int value) const { return MulDiv(value, 96, m_dpi); }
    int PageWidth() const
    {
        CRect r; GetClientRect(&r);
        const int gutter=Px(LayoutY(m_contentHeight))>r.Height()?Px(14):0;
        return max(400, Units(r.Width()-gutter));
    }
    virtual void LayoutContent() = 0;
    void ReferenceTypography()
    {
        const auto font=[&](CFont& target,int size,int weight)
        {
            target.DeleteObject();target.CreateFont(-Px(size),0,0,0,weight,FALSE,FALSE,0,DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,Fonts::Face(weight));
        };
        font(m_font,13,FW_NORMAL);font(m_sectionFont,13,FW_SEMIBOLD);font(m_title,22,FW_SEMIBOLD);
        font(m_buttonFont,13,FW_SEMIBOLD);font(m_cardTitle,14,FW_SEMIBOLD);font(m_value,14,FW_MEDIUM);
    }


    template<class T>
    T& Control(LPCTSTR text, DWORD style, UINT id, bool bold = false)
    {
        using ControlType=std::conditional_t<std::is_same_v<T,CButton>,UiButton,
            std::conditional_t<std::is_same_v<T,CComboBox>,UiComboBox,
            std::conditional_t<std::is_same_v<T,CEdit>,UiEdit,T>>>;
        std::unique_ptr<T> object;
        if constexpr(std::is_same_v<T,CStatic>)
        {
            if((style&SS_TYPEMASK)==SS_ETCHEDHORZ)object=std::make_unique<UiSeparator>();
            else if(bold)object=std::make_unique<UiHeading>();
            else object=std::make_unique<CStatic>();
        }
        else object=std::make_unique<ControlType>();
        if constexpr(std::is_same_v<T,CEdit>)style &= ~WS_BORDER;
        if constexpr(std::is_same_v<T,CComboBox>)style |= CBS_OWNERDRAWFIXED|CBS_HASSTRINGS;
        BOOL created;
        if constexpr (std::is_same_v<T, CStatic>)
            created = object->Create(text, WS_CHILD | WS_VISIBLE | style, CRect(0,0,1,1), this, id);
        else if constexpr (std::is_same_v<T, CButton>)
            created = object->Create(text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | style, CRect(0,0,1,1), this, id);
        else
            created = object->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | style, CRect(0,0,1,1), this, id);
        if (!created)
            AfxThrowResourceException();
        SetWindowTheme(object->m_hWnd,L"",L"");
        if constexpr(std::is_same_v<T,CButton>)object->SetFont(&m_buttonFont);
        else object->SetFont(bold ? &m_bold : &m_font);
        if constexpr(std::is_same_v<T,CComboBox>)static_cast<UiComboBox*>(object.get())->ConfigureSpacing();
        T& result = *object;
        m_controls.push_back(std::move(object));
        return result;
    }
    CStatic& Label(LPCTSTR text, UINT id, bool bold = false)
    {
        auto& label=Control<CStatic>(text, SS_LEFT | SS_NOPREFIX, id, bold);
        if(bold)label.SetFont(&m_sectionFont);
        return label;
    }
    // Double only the gaps between existing section containers. Their internal
    // spacing stays unchanged, and controls after them receive the same offset.
    virtual int ExtraSectionGap(int gap) const{return gap>0 && gap<=64?gap:0;}
    int LayoutY(int y) const
    {
        int extra=0;
        for(size_t i=1;i<m_sections.size();++i)
            if(y>=m_sections[i].top)
            {
                const int gap=m_sections[i].top-m_sections[i-1].bottom;
                // Larger intervals contain action controls, rather than blank spacing.
                extra+=ExtraSectionGap(gap);
            }
        return y+extra+m_leadingSpace;
    }
    void Sections(std::initializer_list<std::pair<int,int>> ranges)
    {
        const int right=PageWidth()-12;
        std::vector<CRect> sections;sections.reserve(ranges.size());
        for(const auto& range:ranges)sections.emplace_back(12,range.first,right,range.second);
        m_sections.swap(sections);
    }
    void Place(CWnd& control, int x, int y, int width, int height)
    {
        const CPoint center(x+width/2,y+min(height,34)/2);
        bool section=false;for(const auto& rect:m_sections)if(rect.PtInRect(center)){section=true;break;}
        if(section)::SetProp(control.m_hWnd,L"FormSectionSurface",reinterpret_cast<HANDLE>(1));
        else ::RemoveProp(control.m_hWnd,L"FormSectionSurface");
        // Never copy the old control pixels to a new scroll position.
        const CRect rect(Px(x),Px(LayoutY(y))-m_scroll,Px(x+max(1,width)),Px(LayoutY(y))-m_scroll+Px(height));
        if(m_inLayout)m_placements.push_back({control.m_hWnd,rect});
        else control.SetWindowPos(nullptr,rect.left,rect.top,rect.Width(),rect.Height(),
            SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOREDRAW);
    }
    void Place(UINT id, int x, int y, int width, int height)
    {
        if (CWnd* control = GetDlgItem(id)) Place(*control, x,y,width,height);
    }
    void Relayout()
    {
        if (!m_layoutReady || m_inLayout) return;
        m_inLayout = true;
        // Establish section bounds before calculating the first scroll range.
        if(!m_presentationReady){LayoutContent();m_presentationReady=true;}
        CRect r; GetClientRect(&r);
        const int content = Px(LayoutY(m_contentHeight));
        m_scroll = std::clamp(m_scroll, 0, max(0, content - r.Height()));
        const bool overflow=content>r.Height();
        m_scrollRail.SetWindowPos(&CWnd::wndTop,r.right-Px(14),0,Px(14),r.Height(),SWP_NOACTIVATE|SWP_NOREDRAW);
        m_scrollRail.SetMetrics(content,r.Height(),m_scroll);
        if(((m_scrollRail.GetStyle()&WS_VISIBLE)!=0)!=overflow)m_scrollRail.ShowWindow(overflow?SW_SHOW:SW_HIDE);
        m_placements.clear();LayoutContent();
        // Commit the entire form geometry together, after section bounds are final.
        HDWP batch=BeginDeferWindowPos(static_cast<int>(m_placements.size()));
        for(const auto& item:m_placements)
        {
            if(!batch)break;
            batch=DeferWindowPos(batch,item.window,nullptr,item.rect.left,item.rect.top,item.rect.Width(),item.rect.Height(),
                SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOREDRAW);
        }
        if(batch)EndDeferWindowPos(batch);
        else for(const auto& item:m_placements)
            ::SetWindowPos(item.window,nullptr,item.rect.left,item.rect.top,item.rect.Width(),item.rect.Height(),
                SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOREDRAW);
        m_placements.clear();m_inLayout=false;
        if(m_leadingSpace>0 && GetParent())GetParent()->SendMessage(WM_UI_VIEW_SCROLLED,m_scroll);
        // Moving child HWNDs with repaint disabled requires repainting every child,
        // including edit/combo non-client borders and the newly exposed background.
        RedrawWindow(nullptr, nullptr,
            RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
    }
    virtual void OffsetDecorations(int){}
    void ScrollTo(int position)
    {
        if(!m_layoutReady || m_inLayout)return;
        if(!m_presentationReady){m_scroll=position;Relayout();return;}
        CRect client;GetClientRect(&client);const int content=Px(LayoutY(m_contentHeight));
        const int next=std::clamp(position,0,max(0,content-client.Height()));
        if(next==m_scroll)return;
        const int delta=m_scroll-next;m_scroll=next;
        // Scrolling translates the existing layout. In particular, never submit
        // a combo's expanded list height as a new collapsed control height here.
        std::vector<Placement> moves;
        for(HWND child=::GetWindow(m_hWnd,GW_CHILD);child;child=::GetWindow(child,GW_HWNDNEXT))
        {
            if(child==m_scrollRail.m_hWnd)continue;
            CRect rect;::GetWindowRect(child,&rect);ScreenToClient(&rect);rect.OffsetRect(0,delta);
            moves.push_back({child,rect});
        }
        OffsetDecorations(delta);
        HDWP batch=BeginDeferWindowPos(static_cast<int>(moves.size()));
        constexpr UINT flags=SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOCOPYBITS|SWP_NOREDRAW;
        for(const auto& item:moves)
        {
            if(!batch)break;
            batch=DeferWindowPos(batch,item.window,nullptr,item.rect.left,item.rect.top,0,0,flags);
        }
        const bool committed=batch && EndDeferWindowPos(batch);
        if(!committed)for(const auto& item:moves)::SetWindowPos(item.window,nullptr,item.rect.left,item.rect.top,0,0,flags);
        m_scrollRail.SetMetrics(content,client.Height(),m_scroll);
        if(m_leadingSpace>0 && GetParent())GetParent()->SendMessage(WM_UI_VIEW_SCROLLED,m_scroll);
        // Let the window manager compose one frame after geometry is committed,
        // rather than forcing nested synchronous paints for every wheel message.
        RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_ALLCHILDREN);
    }
    void EnsureVisible(CWnd* child)
    {
        if (!child) return;
        CRect childRect, client;
        child->GetWindowRect(&childRect); ScreenToClient(&childRect); GetClientRect(&client);
        if (childRect.top < 0) ScrollTo(m_scroll + childRect.top - Px(8));
        else if (childRect.bottom > client.bottom)
            ScrollTo(m_scroll + childRect.bottom - client.bottom + Px(8));
    }
    afx_msg int OnCreate(LPCREATESTRUCT cs)
    {
        if (CWnd::OnCreate(cs) == -1) return -1;
        ModifyStyleEx(0, WS_EX_CONTROLPARENT|WS_EX_COMPOSITED);
        ModifyStyle(0, WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
        m_dpi = static_cast<int>(GetDpiForWindow(m_hWnd));
        if (m_dpi == 0) m_dpi = 96;
        m_font.CreateFont(-Px(14),0,0,0,FW_NORMAL,FALSE,FALSE,0,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,Fonts::Face(FW_NORMAL));
        m_bold.CreateFont(-Px(14),0,0,0,FW_SEMIBOLD,FALSE,FALSE,0,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,Fonts::Face(FW_SEMIBOLD));
        m_sectionFont.CreateFont(-Px(16),0,0,0,FW_SEMIBOLD,FALSE,FALSE,0,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,Fonts::Face(FW_SEMIBOLD));
        m_title.CreateFont(-Px(22),0,0,0,FW_SEMIBOLD,FALSE,FALSE,0,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,Fonts::Face(FW_SEMIBOLD));
        m_cardTitle.CreateFont(-Px(16),0,0,0,FW_MEDIUM,FALSE,FALSE,0,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,Fonts::Face(FW_MEDIUM));
        m_value.CreateFont(-Px(16),0,0,0,FW_MEDIUM,FALSE,FALSE,0,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,Fonts::Face(FW_MEDIUM));
        m_buttonFont.CreateFont(-Px(14),0,0,0,FW_MEDIUM,FALSE,FALSE,0,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,Fonts::Face(FW_MEDIUM));
        m_background.CreateSolidBrush(UiStyle::Colors(m_hWnd).background);
        m_section.CreateSolidBrush(UiStyle::Colors(m_hWnd).section);
        m_input.CreateSolidBrush(UiStyle::Colors(m_hWnd).input);
        m_card.CreateSolidBrush(UiStyle::Colors(m_hWnd).card);
        if(!m_scrollRail.Create(this))return -1;
        return 0;
    }
    afx_msg void OnSize(UINT type, int cx, int cy)
    {
        CWnd::OnSize(type,cx,cy);
        Relayout();
    }
    afx_msg void OnVScroll(UINT code, UINT position, CScrollBar* bar)
    {
        if(bar){CWnd::OnVScroll(code,position,bar);return;}
        CRect rect;GetClientRect(&rect);
        int next=m_scroll;
        switch(code)
        {
        case SB_LINEUP: next-=Px(28);break;
        case SB_LINEDOWN: next+=Px(28);break;
        case SB_PAGEUP: next-=rect.Height();break;
        case SB_PAGEDOWN: next+=rect.Height();break;
        case SB_THUMBTRACK: case SB_THUMBPOSITION: next=static_cast<int>(position);break;
        case SB_TOP: next=0;break;
        case SB_BOTTOM: next=Px(LayoutY(m_contentHeight));break;
        default:return;
        }
        ScrollTo(next);
    }
    afx_msg LRESULT OnRailScroll(WPARAM position,LPARAM)
    {
        ScrollTo(static_cast<int>(position));return 0;
    }
    afx_msg BOOL OnMouseWheel(UINT, short delta, CPoint)
    {
        ScrollTo(m_scroll - MulDiv(delta, Px(70), WHEEL_DELTA));
        return TRUE;
    }
    afx_msg BOOL OnEraseBkgnd(CDC* dc)
    {
        CRect r; GetClientRect(&r); dc->FillRect(&r,&m_background);
        for(const auto& section:m_sections)
        {
            CRect box(Px(section.left),Px(LayoutY(section.top))-m_scroll,Px(section.right),Px(LayoutY(section.bottom))-m_scroll);
            UiStyle::Rounded(*dc,box,UiStyle::Colors(m_hWnd).section,UiStyle::Colors(m_hWnd).sectionBorder,Px(6));
        }
        return TRUE;
    }
    afx_msg HBRUSH OnCtlColor(CDC* dc, CWnd* window, UINT kind)
    {
        const bool input=kind==CTLCOLOR_EDIT || kind==CTLCOLOR_LISTBOX ||
            (window && window->IsKindOf(RUNTIME_CLASS(CEdit)));
        const bool card=window && ::GetProp(window->m_hWnd,L"NetworkCardSurface");
        COLORREF text=UiStyle::Colors(m_hWnd).text;
        if(window && (window->GetFont()==&m_bold || window->GetFont()==&m_title))text=UiStyle::Colors(m_hWnd).accent;
        if(window)
        {
            const UINT_PTR color=reinterpret_cast<UINT_PTR>(::GetProp(window->m_hWnd,L"NetworkTextColor"));
            if(color)text=static_cast<COLORREF>(color-1);
        }
        dc->SetTextColor(window && !window->IsWindowEnabled()?UiStyle::Colors(m_hWnd).disabled:(input?UiStyle::Colors(m_hWnd).bright:text));
        const bool section=window && ::GetProp(window->m_hWnd,L"FormSectionSurface");
        dc->SetBkColor(input?UiStyle::Colors(m_hWnd).input:(card?UiStyle::Colors(m_hWnd).card:(section?UiStyle::Colors(m_hWnd).section:UiStyle::Colors(m_hWnd).background)));
        if(input)return static_cast<HBRUSH>(m_input.GetSafeHandle());
        return static_cast<HBRUSH>((card?m_card:(section?m_section:m_background)).GetSafeHandle());
    }
public:
    int ScrollPosition() const{return m_scroll;}
    void SetLeadingSpace(int units)
    {if(m_leadingSpace!=units){m_leadingSpace=units;
        if(units>0)ModifyStyleEx(WS_EX_COMPOSITED,0);else ModifyStyleEx(0,WS_EX_COMPOSITED);Relayout();}}
    BOOL CreatePage(CWnd* parent, UINT id)
    {
        LPCTSTR className = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr,IDC_ARROW),
            reinterpret_cast<HBRUSH>(COLOR_3DFACE+1), nullptr);
        return CreateEx(WS_EX_CONTROLPARENT, className, L"Network content",
            WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            CRect(0,0,1,1),parent,id);
    }
    BOOL GetMinimumSize(CSize& size)
    {
        size = CSize(Px(420),Px(260)); return TRUE;
    }
    bool CanFrameworkTakeFocus()
    {
        HWND focus = ::GetFocus();
        return !(focus == m_hWnd || ::IsChild(m_hWnd,focus));
    }
    BOOL PreTranslateMessage(MSG* message) override
    {
        if(message->message==WM_KEYDOWN)
        {
            CWnd* focus=GetFocus();
            const bool inView=focus && (focus==this || IsChild(focus));
            const bool input=inView && (focus->IsKindOf(RUNTIME_CLASS(CEdit)) ||
                focus->IsKindOf(RUNTIME_CLASS(CComboBox)) ||
                (focus->GetParent() && focus->GetParent()->IsKindOf(RUNTIME_CLASS(CComboBox))));
            if(inView && !input)
            {
                if(message->wParam==VK_NEXT){SendMessage(WM_VSCROLL,SB_PAGEDOWN);return TRUE;}
                if(message->wParam==VK_PRIOR){SendMessage(WM_VSCROLL,SB_PAGEUP);return TRUE;}
                if(message->wParam==VK_HOME){ScrollTo(0);return TRUE;}
                if(message->wParam==VK_END){ScrollTo(Px(LayoutY(m_contentHeight)));return TRUE;}
            }
        }
        if (message->message == WM_KEYDOWN && message->wParam == VK_TAB)
        {
            CWnd* current = GetFocus();
            if (current && (current == this || IsChild(current)))
            {
                CWnd* next = GetNextDlgTabItem(current, (::GetKeyState(VK_SHIFT) & 0x8000) != 0);
                if (next) { next->SetFocus(); EnsureVisible(next); return TRUE; }
            }
        }
        return CWnd::PreTranslateMessage(message);
    }
    DECLARE_MESSAGE_MAP()
};
