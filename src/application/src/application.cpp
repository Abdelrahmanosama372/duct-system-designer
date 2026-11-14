#include "application/application.hpp"
#include <utility>

namespace dsd::application {

void DsdApplication::register_project_information(ProjectInformation information) {
    project_information_ = std::move(information);
}

const std::optional<ProjectInformation>& DsdApplication::project_information() const noexcept {
    return project_information_;
}

void DsdApplication::clear_project_information() noexcept {
    project_information_.reset();
}

}
