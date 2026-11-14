#pragma once
#include "autocad/ui/style/Theme.h"

class UiSeparator : public CStatic
{
    void Paint(CDC& dc) {CRect r;GetClientRect(&r);dc.FillSolidRect(r,UiStyle::Colors(m_hWnd).border);}
    afx_msg void OnPaint() {CPaintDC dc(this);Paint(dc);}
    afx_msg LRESULT OnPrintClient(WPARAM wp,LPARAM) {Paint(*CDC::FromHandle(reinterpret_cast<HDC>(wp)));return 0;}
    DECLARE_MESSAGE_MAP()
};
