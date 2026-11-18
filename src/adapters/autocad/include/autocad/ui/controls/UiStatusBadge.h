#pragma once
#include "autocad/ui/style/Theme.h"

// A noninteractive status pill; the native static control keeps it out of the tab order.
class UiStatusBadge : public CStatic
{
public:
    void DrawItem(LPDRAWITEMSTRUCT item) override
    {
        CDC* dc=CDC::FromHandle(item->hDC);
        CRect rect(item->rcItem);
        const auto& colors=UiStyle::Colors(m_hWnd);
        CString text;GetWindowText(text);
        const COLORREF ink=(text==L"Registered" || text==L"Analyzed")?colors.success:colors.amber;
        dc->FillSolidRect(rect,colors.card);
        CRect pill=rect;pill.DeflateRect(0,UiStyle::Px(m_hWnd,5));
        UiStyle::Rounded(*dc,pill,UiStyle::Blend(colors.card,ink,16),UiStyle::Blend(colors.card,ink,16),UiStyle::Px(m_hWnd,8));
        auto* oldFont=dc->SelectObject(GetFont());
        dc->SetBkMode(TRANSPARENT);dc->SetTextColor(ink);
        dc->DrawText(text,pill,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
        dc->SelectObject(oldFont);
    }
};
