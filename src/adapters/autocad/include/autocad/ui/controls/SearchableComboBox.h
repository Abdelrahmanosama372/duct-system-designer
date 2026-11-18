#pragma once
#include "autocad/ui/controls/UiComboBox.h"

// Editable search text is separate from the list of valid selectable values.
class SearchableComboBox : public UiComboBox {
    std::vector<CString> m_items;
    bool m_updating = false;
public:
    void SetItems(const std::vector<std::wstring>& items, LPCTSTR preferred) {
        CString previous; GetWindowText(previous);
        m_updating = true;
        m_items.clear(); ResetContent();
        for (const auto& item : items) m_items.emplace_back(item.c_str());
        std::sort(m_items.begin(), m_items.end(), [](const auto& a, const auto& b) {
            return a.CompareNoCase(b) < 0;
        });
        CString selected;
        for (const auto& item : m_items) {
            AddString(item);
            if (item.CompareNoCase(previous) == 0) selected = item;
        }
        if (selected.IsEmpty())
            for (const auto& item : m_items)
                if (item.CompareNoCase(preferred) == 0) selected = item;
        if (selected.IsEmpty() && !m_items.empty()) selected = m_items.front();
        SetWindowText(selected);
        EnableWindow(!m_items.empty());
        SetMinVisibleItems(6);
        m_updating = false;
    }
    void FilterEdit() {
        if (m_updating) return;
        CString text; GetWindowText(text);
        CString query = text; query.MakeLower();
        const DWORD caret = GetEditSel();
        m_updating = true;
        ResetContent();
        for (const auto& item : m_items) {
            CString candidate = item; candidate.MakeLower();
            if (candidate.Find(query) >= 0) AddString(item);
        }
        ShowDropDown(GetCount() > 0);
        SetWindowText(text);
        SetEditSel(LOWORD(caret), HIWORD(caret));
        m_updating = false;
    }
    bool SelectedValue(std::wstring& value) const {
        CString text; GetWindowText(text);
        for (const auto& item : m_items)
            if (item.CompareNoCase(text) == 0) {
                value = item.GetString(); return true;
            }
        return false;
    }
};
