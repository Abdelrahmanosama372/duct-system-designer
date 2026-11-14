#pragma once
#include "application/application.hpp"
#include <gtest/gtest.h>

namespace dsd::application::test {
inline SourcePath path(std::string id, std::vector<geometry::Point> vertices,
                       std::wstring layer = L"Ducts") {
    return {std::move(id), std::move(layer), std::move(vertices)};
}
inline geometry::Block block(std::string id, std::wstring name, geometry::Point insertion) {
    return {std::move(id),
            std::move(name),
            insertion,
            {{insertion.x - 20, insertion.y - 10}, {insertion.x + 20, insertion.y + 10}}};
}
inline DuctNetwork draft(std::vector<SourcePath> lines) {
    DuctNetwork result;
    result.id = "NET-001";
    result.root = lines.front();
    result.lines = std::move(lines);
    return result;
}
inline void expect_xy(const geometry::Point& point, double x, double y) {
    EXPECT_NEAR(point.x, x, 1e-7);
    EXPECT_NEAR(point.y, y, 1e-7);
    EXPECT_EQ(point.z, 0);
}
} // namespace dsd::application::test
