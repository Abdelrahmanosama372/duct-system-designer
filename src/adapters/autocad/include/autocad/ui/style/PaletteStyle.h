#pragma once
#include "autocad/ui/platform/ObjectArx.h"
#include "autocad/ui/style/Theme.h"
#include "aduiTheme.h"
#include "aduiThemeManager.h"

// A private SDK theme affects only this palette set, including its existing vertical tabs.
// Keep the manager reference until all palette HWNDs have been destroyed.
namespace PaletteStyle
{
    inline CAdUiTheme* theme=nullptr;
    inline bool Apply(CAdUiPaletteSet& palette)
    {
        auto* manager=AdUiGetThemeManager();if(!manager)return false;
        theme=manager->GetTheme(L"DuctSystemDesigner.ModernDark");if(!theme)return false;
        theme->SuspendModifiedNotifications();
        const auto color=[&](AdUiThemeElement element,COLORREF value){theme->SetColor(element,value);};
        color(kPaletteBackground,UiStyle::Background);
        color(kPaletteCaptionBackground,UiStyle::MainBackground);color(kPaletteCaptionInactive,UiStyle::MainBackground);
        color(kPaletteCaptionText,UiStyle::Bright);color(kPaletteCaptionInactiveText,UiStyle::Muted);
        color(kPaletteTabBackground,UiStyle::MainBackground);color(kPaletteTabText,UiStyle::Text);
        color(kPaletteTabTextHilite,UiStyle::Bright);color(kPaletteTabSelectHilite,UiStyle::Panel);
        color(kPaletteTabHighlight,UiStyle::Blue);color(kPaletteTabShadow,UiStyle::Border);
        color(kPaletteBorder,UiStyle::Border);color(kPaletteFrameInnerBorder,UiStyle::Border);
        color(kPaletteFrameOuterBorder,UiStyle::Border);color(kPaletteFrameBackground,UiStyle::MainBackground);
        color(kPaletteFrameShadow,UiStyle::MainBackground);
        color(kPaletteToolTip,UiStyle::Panel);color(kPaletteToolTipText,UiStyle::Bright);
        color(kPaletteButtonText,UiStyle::Bright);color(kPaletteButtonTextDisabled,UiStyle::Disabled);
        color(kPaletteButtonBorder,UiStyle::Border);color(kPaletteItemText,UiStyle::Bright);
        color(kPaletteScrollBackground,UiStyle::Background);color(kPaletteScrollThumb,UiStyle::Border);
        color(kPaletteScrollThumbHover,UiStyle::Muted);color(kPaletteScrollArrow,UiStyle::Text);
        color(kPanelBackground,UiStyle::Background);color(kPanelBorder,UiStyle::Border);
        color(kControlBackground,UiStyle::Input);color(kControlBorder,UiStyle::Border);
        color(kControlText,UiStyle::Bright);color(kControlDisabledText,UiStyle::Disabled);
        color(kControlHighlight,UiStyle::Blue);color(kControlActiveOuterBorder,UiStyle::Blue);
        color(kTabControlActiveBackground,UiStyle::Panel);color(kTabControlInactiveBackground,UiStyle::MainBackground);
        const UINT dpi=GetDpiForWindow(palette.m_hWnd);
        for(auto element:{kPaletteFontCaption,kPaletteFontTab,kPaletteFont})
        {
            LOGFONT font={};theme->GetFont(element,&font);
            // Retain SDK rotation/orientation for the right-side vertical tab labels.
            font.lfHeight=-MulDiv(element==kPaletteFontTab?14:13,dpi?dpi:96,96);
            font.lfWeight=FW_MEDIUM;font.lfQuality=CLEARTYPE_QUALITY;
            lstrcpyn(font.lfFaceName,Fonts::Face(FW_MEDIUM),LF_FACESIZE);theme->SetFont(element,&font);
        }
        theme->ResumeModifiedNotifications();palette.SetTheme(theme);palette.UpdateTabs();return true;
    }
    inline void Release()
    {
        if(theme){if(auto* manager=AdUiGetThemeManager())manager->ReleaseTheme(theme);theme=nullptr;}
    }
}
