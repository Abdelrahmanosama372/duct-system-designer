#pragma once

#include <string>

namespace dsd::application {

// Every field is optional; an empty string represents a blank field.
struct ProjectInformation {
    std::wstring project_name;
    std::wstring project_number;
    std::wstring building_name;
    std::wstring project_address;
    std::wstring client_name;
    std::wstring organization_name;
    std::wstring author;
    std::wstring level_name;
    std::wstring drawing_number;
    std::wstring revision;
    std::wstring additional_information;

    bool operator==(const ProjectInformation&) const = default;
};

} // namespace dsd::application
