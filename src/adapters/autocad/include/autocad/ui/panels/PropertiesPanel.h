#pragma once
#include "autocad/ui/layout/TabbedContent.h"
#include "autocad/ui/model/PropertiesSession.h"

enum PropertyControl : UINT {
    PC_TITLE = 3100,
    PC_SUBTITLE,
    PC_SEGMENT_LABEL,
    PC_SEGMENT,
    PC_PICK,
    PC_NETWORK_LABEL,
    PC_NETWORK,
    PC_FLOW_LABEL,
    PC_FLOW,
    PC_WIDTH_LABEL,
    PC_WIDTH,
    PC_HEIGHT_LABEL,
    PC_HEIGHT,
    PC_APPLY,
    PC_STATUS,
    PC_SELECTION_SECTION,
    PC_DATA_SECTION
};
class SegmentPropertiesPage : public ScrollPage {
    std::optional<PropertiesSession::Selection> m_selection;

  protected:
    int ExtraSectionGap(int gap) const override { return gap > 0 && gap <= 64 ? 12 - gap : 0; }
    void LayoutContent() override;
    afx_msg int OnCreate(LPCREATESTRUCT cs);
    BOOL OnCommand(WPARAM wp, LPARAM lp) override;

  public:
    void LoadSelection();
    DECLARE_MESSAGE_MAP()
};

enum EquipmentPropertyControl : UINT {
    EP_TITLE = 3400,
    EP_SELECTED_LABEL,
    EP_SELECTED,
    EP_PICK,
    EP_NETWORK_LABEL,
    EP_NETWORK,
    EP_FIRST_LABEL,
    EP_FIRST,
    EP_SECOND_LABEL,
    EP_SECOND,
    EP_THIRD_LABEL,
    EP_THIRD,
    EP_FOURTH_LABEL,
    EP_FOURTH,
    EP_APPLY,
    EP_STATUS,
    EP_SUBTITLE,
    EP_SELECTION_SECTION,
    EP_DATA_SECTION
};
class EquipmentPropertiesPage : public ScrollPage {
    bool m_damper;
    std::optional<PropertiesSession::Selection> m_selection;

  protected:
    int ExtraSectionGap(int gap) const override { return gap > 0 && gap <= 64 ? 12 - gap : 0; }
    void LayoutContent() override;
    afx_msg int OnCreate(LPCREATESTRUCT cs);
    BOOL OnCommand(WPARAM wp, LPARAM lp) override;

  public:
    explicit EquipmentPropertiesPage(bool damper) : m_damper(damper) {}
    void LoadSelection();
    DECLARE_MESSAGE_MAP()
};

class PropertiesContent : public TabbedContent {
    SegmentPropertiesPage m_segments;
    EquipmentPropertiesPage m_terminals{false}, m_dampers{true};

  protected:
    afx_msg int OnCreate(LPCREATESTRUCT cs);
    afx_msg void OnDestroy();
    afx_msg LRESULT OnPropertiesUpdated(WPARAM, LPARAM);

  public:
    BOOL CreateView(CWnd* parent, UINT id) {
        return TabbedContent::CreateView(parent, id, L"Properties");
    }
    DECLARE_MESSAGE_MAP()
};
