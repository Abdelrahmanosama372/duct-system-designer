#pragma once
#include "application/duct_network.hpp"

namespace dsd::application {
class VisualizationTextFactory {
  public:
    [[nodiscard]] std::vector<VisualizationText>
    create(const DuctNetwork& network, const VisualizationTextFields& fields) const;
};
} // namespace dsd::application
