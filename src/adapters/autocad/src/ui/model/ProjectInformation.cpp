#include "autocad/ui/model/ProjectInformation.h"

namespace {
dsd::application::DsdApplication application;

std::wstring Text(const CString& value) {
    return {value.GetString(), static_cast<std::size_t>(value.GetLength())};
}
}

namespace ProjectInfo
{
    std::array<CString,Count> draft{},saved{};
    bool editing = true;

    dsd::application::DsdApplication& Application() { return application; }

    void RegisterProjectInformation() {
        application.register_project_information({
            .project_name = Text(draft[ProjectName]),
            .project_number = Text(draft[ProjectNumber]),
            .building_name = Text(draft[BuildingName]),
            .project_address = Text(draft[ProjectAddress]),
            .client_name = Text(draft[ClientName]),
            .organization_name = Text(draft[OrganizationName]),
            .author = Text(draft[Author]),
            .level_name = Text(draft[LevelName]),
            .drawing_number = Text(draft[DrawingNumber]),
            .revision = Text(draft[Revision]),
            .additional_information = Text(draft[AdditionalInformation]),
        });
        saved = draft;
        editing = false;
    }

    void Reset() {
        for (auto& value : draft) value.Empty();
        for (auto& value : saved) value.Empty();
        editing = true;
        application.clear_project_information();
    }
}
