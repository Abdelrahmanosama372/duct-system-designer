#pragma once

#include "application/project_information.hpp"
#include <optional>

namespace dsd::application {

class DsdApplication {
public:
    void register_project_information(ProjectInformation information);
    [[nodiscard]] const std::optional<ProjectInformation>& project_information() const noexcept;
    void clear_project_information() noexcept;

private:
    std::optional<ProjectInformation> project_information_;
};

} // namespace dsd::application
