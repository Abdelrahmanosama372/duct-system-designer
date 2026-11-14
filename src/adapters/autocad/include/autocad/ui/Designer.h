#pragma once
// Call on AutoCAD's UI thread after the ObjectARX module has initialized.
namespace DuctUi
{
    enum class Panel { ProjectInformation, Network, Properties, DrawingGeneration, Reports };
    void Show();                       // Reopen without changing the active tab.
    void Show(Panel panel);
    void Hide();                       // Keep all form values and tab selections.
    void SelectProjectInformation();    // Compatibility behavior of NETWORK_PANEL_CLOSE.
    void Shutdown();                    // Destroy windows before unloading MFC/fonts.
}
