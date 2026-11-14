#include "autocad/commands/UiCommands.h"
#include "autocad/ui/Designer.h"
#include "autocad/ui/platform/ObjectArx.h"

namespace
{
void ShowDesignerPalette(){DuctUi::Show();}
void HideDesignerPalette(){DuctUi::Hide();}
void ShowProjectInformationPalette(){DuctUi::Show(DuctUi::Panel::ProjectInformation);}
void ShowDrawingGenerationPalette(){DuctUi::Show(DuctUi::Panel::DrawingGeneration);}
void ShowReportsPalette(){DuctUi::Show(DuctUi::Panel::Reports);}
void ShowPropertiesPalette(){DuctUi::Show(DuctUi::Panel::Properties);}
void ShowNetworkPalette(){DuctUi::Show(DuctUi::Panel::Network);}
void HideNetworkPalette(){DuctUi::SelectProjectInformation();}
}

void Autocad::Commands::RegisterUiCommands()
{
        acedRegCmds->addCommand(
            _T("DUCT_SYSTEM"),
            _T("DUCT_PANEL"),
            _T("DUCT_PANEL"),
            ACRX_CMD_MODAL,
            ShowDesignerPalette
        );

        acedRegCmds->addCommand(
            _T("DUCT_SYSTEM"),
            _T("DUCT_PANEL_CLOSE"),
            _T("DUCT_PANEL_CLOSE"),
            ACRX_CMD_MODAL,
            HideDesignerPalette
        );

        acedRegCmds->addCommand(
            _T("DUCT_SYSTEM"), _T("NETWORK_PANEL"), _T("NETWORK_PANEL"),
            ACRX_CMD_MODAL, ShowNetworkPalette);

        acedRegCmds->addCommand(
            _T("DUCT_SYSTEM"), _T("NETWORK_PANEL_CLOSE"), _T("NETWORK_PANEL_CLOSE"),
            ACRX_CMD_MODAL, HideNetworkPalette);

        acedRegCmds->addCommand(
            _T("DUCT_SYSTEM"), _T("PROPERTIES_PANEL"), _T("PROPERTIES_PANEL"),
            ACRX_CMD_MODAL, ShowPropertiesPalette);

        acedRegCmds->addCommand(
            _T("DUCT_SYSTEM"), _T("REPORTS_PANEL"), _T("REPORTS_PANEL"),
            ACRX_CMD_MODAL, ShowReportsPalette);
        acedRegCmds->addCommand(
            _T("DUCT_SYSTEM"), _T("BOQ_PANEL"), _T("BOQ_PANEL"),
            ACRX_CMD_MODAL, ShowReportsPalette);

        acedRegCmds->addCommand(
            _T("DUCT_SYSTEM"), _T("PROJECT_INFO_PANEL"), _T("PROJECT_INFO_PANEL"),
            ACRX_CMD_MODAL, ShowProjectInformationPalette);

        acedRegCmds->addCommand(
            _T("DUCT_SYSTEM"), _T("DRAWING_GENERATION_PANEL"), _T("DRAWING_GENERATION_PANEL"),
            ACRX_CMD_MODAL, ShowDrawingGenerationPalette);

}

void Autocad::Commands::UnregisterUiCommands()
{
    acedRegCmds->removeGroup(_T("DUCT_SYSTEM"));
}
