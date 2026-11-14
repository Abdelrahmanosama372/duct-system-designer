#pragma once
#include "autocad/ui/platform/Mfc.h"
#include <uxtheme.h>
#include <gdiplus.h>
#include "autocad/ui/style/Fonts.h"
#pragma comment(lib,"gdiplus.lib")
#pragma comment(lib,"uxtheme.lib")

// Modern Dark styling tokens shared by all native controls. Layout and behavior stay in their pages.
namespace UiStyle
{
    constexpr COLORREF MainBackground=RGB(43,50,61);    // #2B323D graphite canvas
    constexpr COLORREF Background=MainBackground;
    constexpr COLORREF SectionBackground=RGB(54,64,78); // #36404E lighter form panels
    constexpr COLORREF Panel=RGB(61,75,94);             // #3D4B5E navy navigation
    constexpr COLORREF Card=RGB(54,67,84);
    constexpr COLORREF Input=RGB(36,44,55);             // #242C37 inset fields
    constexpr COLORREF Border=RGB(79,94,113);           // #4F5E71 visible soft borders
    constexpr COLORREF Text=RGB(192,203,216);           // #C0CBD8 labels
    constexpr COLORREF Bright=RGB(239,243,248);         // #EFF3F8 primary text
    constexpr COLORREF Muted=Text;
    constexpr COLORREF Disabled=RGB(137,150,167);
    constexpr COLORREF Blue=RGB(57,143,235);
    constexpr COLORREF Green=RGB(46,162,118);
    constexpr COLORREF SuccessText=RGB(129,215,176);
    constexpr COLORREF Danger=RGB(231,107,107);
    constexpr COLORREF Amber=RGB(224,177,92);
    constexpr COLORREF Purple=Blue;
    constexpr int ControlRadius=6;
    inline COLORREF Blend(COLORREF base,COLORREF overlay,int percent)
    {
        return RGB((GetRValue(base)*(100-percent)+GetRValue(overlay)*percent)/100,
            (GetGValue(base)*(100-percent)+GetGValue(overlay)*percent)/100,
            (GetBValue(base)*(100-percent)+GetBValue(overlay)*percent)/100);
    }
    enum class ButtonRole { Secondary, Primary, Generation, Removal };
    // A visual treatment can be scoped to a page without changing its controls or state.
    struct VisualStyle
    {
        COLORREF background,section,panel,card,input,border,text,bright,muted,disabled;
        COLORREF accent,green,success,danger,amber,sectionBorder,badge;
        bool darkPrimaryText=false;
    };
    inline const VisualStyle DefaultStyle={Background,SectionBackground,Panel,Card,Input,Border,Text,Bright,Muted,
        Disabled,Blue,Green,SuccessText,Danger,Amber,Border,Panel,false};
    // Colors sampled from the revised Figma reference image.
    inline const VisualStyle ProjectReferenceStyle={RGB(37,45,57),RGB(53,65,83),RGB(40,51,66),RGB(53,65,83),
        RGB(40,51,66),RGB(86,101,124),RGB(193,204,218),RGB(243,246,252),RGB(165,179,199),RGB(128,143,164),
        RGB(157,195,255),Green,SuccessText,Danger,Amber,RGB(73,89,111),RGB(67,91,124),true};
    inline const VisualStyle& Colors(HWND window)
    {
        for(HWND current=window;current;current=::GetParent(current))
            if(auto* style=reinterpret_cast<const VisualStyle*>(::GetProp(current,L"DuctVisualStyle")))return *style;
        return DefaultStyle;
    }
    inline void SetVisualStyle(CWnd& window,const VisualStyle& style)
    {::SetProp(window.m_hWnd,L"DuctVisualStyle",reinterpret_cast<HANDLE>(const_cast<VisualStyle*>(&style)));}
    inline ULONG_PTR graphicsToken=0;
    inline bool StartGraphics()
    {
        if(graphicsToken)return true;
        Gdiplus::GdiplusStartupInput startup;
        return Gdiplus::GdiplusStartup(&graphicsToken,&startup,nullptr)==Gdiplus::Ok;
    }
    inline void StopGraphics()
    {
        if(graphicsToken){Gdiplus::GdiplusShutdown(graphicsToken);graphicsToken=0;}
    }
    inline Gdiplus::Color Color(COLORREF color)
    {return Gdiplus::Color(255,GetRValue(color),GetGValue(color),GetBValue(color));}
    inline void Rounded(CDC& dc,const CRect& rect,COLORREF fill,COLORREF border,int radius)
    {
        if(rect.Width()<2 || rect.Height()<2)return;
        if(!graphicsToken)
        {
            CBrush brush(fill);CPen pen(PS_SOLID,1,border);
            auto oldBrush=dc.SelectObject(&brush);auto oldPen=dc.SelectObject(&pen);
            dc.RoundRect(rect,CPoint(radius*2,radius*2));dc.SelectObject(oldBrush);dc.SelectObject(oldPen);return;
        }
        Gdiplus::Graphics graphics(dc.GetSafeHdc());graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        const float x=static_cast<float>(rect.left)+0.5f,y=static_cast<float>(rect.top)+0.5f;
        const float w=static_cast<float>(rect.Width())-1,h=static_cast<float>(rect.Height())-1;
        const float diameter=min(static_cast<float>(radius*2),min(w,h));
        Gdiplus::GraphicsPath path;
        path.AddArc(x,y,diameter,diameter,180,90);path.AddArc(x+w-diameter,y,diameter,diameter,270,90);
        path.AddArc(x+w-diameter,y+h-diameter,diameter,diameter,0,90);
        path.AddArc(x,y+h-diameter,diameter,diameter,90,90);path.CloseFigure();
        Gdiplus::SolidBrush brush(Color(fill));Gdiplus::Pen pen(Color(border),1.0f);
        graphics.FillPath(&brush,&path);graphics.DrawPath(&pen,&path);
    }
    inline int Px(HWND window,int value)
    {
        const int dpi=GetDpiForWindow(window);return MulDiv(value,dpi?dpi:96,96);
    }
    inline COLORREF ControlSurface(HWND window,COLORREF fallback=Background)
    {const auto& style=Colors(window);
        return ::GetProp(window,L"FormSectionSurface")?style.section:(fallback==Background?style.background:fallback);}
    inline void Surface(CWnd& control)
    { ::SetProp(control.m_hWnd,L"NetworkCardSurface",reinterpret_cast<HANDLE>(1)); }
    inline void TextColor(CWnd& control,COLORREF color)
    { ::SetProp(control.m_hWnd,L"NetworkTextColor",reinterpret_cast<HANDLE>(static_cast<UINT_PTR>(color)+1)); }
}

