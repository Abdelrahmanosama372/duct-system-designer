#pragma once
#include "autocad/ui/platform/ObjectArx.h"
#include "autocad/ui/style/Theme.h"

// One native palette bridge for every form. It owns its content for the full
// palette lifetime; activating a tab never recreates controls or resets values.
class PalettePage : public CAdUiPalette
{
    bool ready_=false;
    void LayoutView();
protected:
    virtual BOOL CreateContent()=0;
    virtual CWnd& ContentWindow()=0;
    virtual void ActivateContent() {}
    afx_msg int OnCreate(LPCREATESTRUCT cs);
    afx_msg void OnSize(UINT type,int cx,int cy);
    afx_msg BOOL OnEraseBkgnd(CDC* dc);
public:
    BOOL GetMinimumSize(CSize& size) override;
    bool CanFrameworkTakeFocus() override;
    BOOL PreTranslateMessage(MSG* message) override;
    void OnSetActive() override;
    DECLARE_MESSAGE_MAP()
};
template<class ContentType,UINT ControlId,bool RefreshOnActivation>
class ContentPalette final : public PalettePage
{
    ContentType content_;
protected:
    BOOL CreateContent() override
    {
        if constexpr(requires { content_.CreateView(this,ControlId); })
            return content_.CreateView(this,ControlId);
        else return content_.CreatePage(this,ControlId);
    }
    CWnd& ContentWindow() override { return content_; }
    void ActivateContent() override
    { if constexpr(RefreshOnActivation) content_.Refresh(); }
public:
    ContentType& Content(){return content_;}
};
