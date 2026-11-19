#include "autocad/ui/model/ProjectInformation.h"
#include "autocad/ui/model/ApplicationSession.h"
#include "autocad/ui/model/NetworkActions.h"

namespace {

std::wstring Text(const CString &value) {
    return {value.GetString(), static_cast<std::size_t>(value.GetLength())};
}
} // namespace

namespace ProjectInfo {
std::array<CString, Count> &Draft() { return DuctUi::Session().project_draft; }
std::array<CString, Count> &Saved() { return DuctUi::Session().project_saved; }
bool &Editing() { return DuctUi::Session().project_editing; }
void ReloadFromApplication() {
    const auto &info = Application().project_information();
    if (!info) {
        Draft() = {};
        Saved() = {};
        Editing() = true;
        return;
    }
    Draft() = {info->project_name.c_str(),
               info->project_number.c_str(),
               info->building_name.c_str(),
               info->project_address.c_str(),
               info->client_name.c_str(),
               info->organization_name.c_str(),
               info->author.c_str(),
               info->level_name.c_str(),
               info->drawing_number.c_str(),
               info->revision.c_str(),
               info->additional_information.c_str()};
    Saved() = Draft();
    Editing() = false;
}

dsd::application::DsdApplication &Application() { return DuctUi::Application(); }

void RegisterProjectInformation() {
    NetworkActions::CommitDrawingChange([&] {
        Application().register_project_information({
            .project_name = Text(Draft()[ProjectName]),
            .project_number = Text(Draft()[ProjectNumber]),
            .building_name = Text(Draft()[BuildingName]),
            .project_address = Text(Draft()[ProjectAddress]),
            .client_name = Text(Draft()[ClientName]),
            .organization_name = Text(Draft()[OrganizationName]),
            .author = Text(Draft()[Author]),
            .level_name = Text(Draft()[LevelName]),
            .drawing_number = Text(Draft()[DrawingNumber]),
            .revision = Text(Draft()[Revision]),
            .additional_information = Text(Draft()[AdditionalInformation]),
        });
        Saved() = Draft();
        Editing() = false;
    });
}

void Reset() {
    for (auto &value : Draft())
        value.Empty();
    for (auto &value : Saved())
        value.Empty();
    Editing() = true;
    Application().clear_project_information();
}
} // namespace ProjectInfo
