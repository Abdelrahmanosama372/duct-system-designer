#include "autocad/ui/controls/UiButton.h"

BEGIN_MESSAGE_MAP(UiButton,CButton)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_MOUSEMOVE()
    ON_WM_SETFOCUS()
    ON_WM_KILLFOCUS()
    ON_WM_ENABLE()
    ON_MESSAGE(WM_MOUSELEAVE,OnMouseLeave)
    ON_MESSAGE(WM_PRINTCLIENT,OnPrintClient)
END_MESSAGE_MAP()

namespace {
// Resolve beside this DLL/EXE rather than using AutoCAD's working directory.
void IconModuleAnchor() {}
std::wstring IconAssetPath(const wchar_t* name)
{
    HMODULE module=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCWSTR>(&IconModuleAnchor),&module))return {};
    std::wstring path(32768,L'\0');
    const DWORD length=GetModuleFileNameW(module,path.data(),static_cast<DWORD>(path.size()));
    if(!length || length>=path.size())return {};
    path.resize(length);
    const auto separator=path.find_last_of(L"\\/");
    if(separator==std::wstring::npos)return {};
    return path.substr(0,separator+1)+L"icons\\"+name;
}
}
void UiButton::SetIcon(Icon icon)
{
    if(icon==m_icon && (m_iconImage || (icon!=Icon::Eye && icon!=Icon::EyeOff)))return;
    m_icon=icon;m_iconImage.reset();
    if(icon==Icon::Eye || icon==Icon::EyeOff)
    {
        const auto path=IconAssetPath(icon==Icon::Eye?L"eye.png":L"eye-off.png");
        auto image=std::make_unique<Gdiplus::Bitmap>(path.c_str());
        if(image->GetLastStatus()==Gdiplus::Ok)m_iconImage=std::move(image);
    }
    Invalidate(FALSE);
}
void UiButton::PaintIcon(CDC& dc,const CRect& rect,COLORREF ink)
{
    const int x=rect.CenterPoint().x,y=rect.CenterPoint().y;
    auto px=[this](int value){return UiStyle::Px(m_hWnd,value);};
    if(m_icon==Icon::More)
    {
        CPen pen(PS_SOLID,1,ink);CBrush dots(ink);
        auto* oldPen=dc.SelectObject(&pen);auto* oldBrush=dc.SelectObject(&dots);
        for(int offset=-5;offset<=5;offset+=5)
            dc.Ellipse(x-px(1),y+px(offset)-px(1),x+px(2),y+px(offset)+px(2));
        dc.SelectObject(oldBrush);dc.SelectObject(oldPen);
        return;
    }
    if(!m_iconImage)return;
    Gdiplus::Graphics graphics(dc.GetSafeHdc());
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    // Preserve PNG transparency while applying the native button state color.
    Gdiplus::ColorMatrix tint={{
        {0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,1,0},
        {GetRValue(ink)/255.f,GetGValue(ink)/255.f,GetBValue(ink)/255.f,0,1}}};
    Gdiplus::ImageAttributes attributes;attributes.SetColorMatrix(&tint);
    const int size=px(18);
    graphics.DrawImage(m_iconImage.get(),Gdiplus::Rect(x-size/2,y-size/2,size,size),0,0,
                       m_iconImage->GetWidth(),m_iconImage->GetHeight(),Gdiplus::UnitPixel,&attributes);
}
