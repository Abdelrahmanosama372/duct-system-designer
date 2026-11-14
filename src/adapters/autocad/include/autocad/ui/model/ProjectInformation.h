#pragma once
#include "autocad/ui/platform/Mfc.h"
#include "application/application.hpp"

// UI draft and edit mode accompany the application-memory project registration.
namespace ProjectInfo
{
    enum Field : size_t
    {
        ProjectName,ProjectNumber,BuildingName,ProjectAddress,ClientName,
        OrganizationName,Author,LevelName,DrawingNumber,Revision,AdditionalInformation,Count
    };
    inline constexpr std::array<const wchar_t*,Count> Labels={L"Project name:",L"Project number:",
        L"Building name:",L"Project address:",L"Client name:",L"Organization name:",L"Author:",
        L"Level name:",L"Drawing number:",L"Revision:",L"Additional information:"};
    inline constexpr std::array<const wchar_t*,Count> Placeholders={L"Enter project name",L"e.g. PRJ-001",
        L"Enter building name",L"Enter project address",L"Enter client name",L"Enter organization name",
        L"Enter author name",L"e.g. Ground floor",L"Enter drawing number",L"e.g. A",L""};
    extern std::array<CString,Count> draft,saved;
    extern bool editing;
    dsd::application::DsdApplication& Application();
    void RegisterProjectInformation();
    void Reset();
}
