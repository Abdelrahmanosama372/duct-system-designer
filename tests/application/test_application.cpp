#include "application/application.hpp"
#include <gtest/gtest.h>

namespace dsd::application::test {
namespace {

TEST(ApplicationProjectInformationTest, InitiallyHasNoRegisteredProject) {
    const DsdApplication application;
    EXPECT_FALSE(application.project_information());
}

TEST(ApplicationProjectInformationTest, AcceptsAllFieldsBlank) {
    DsdApplication application;
    application.register_project_information({});

    ASSERT_TRUE(application.project_information());
    EXPECT_EQ(*application.project_information(), ProjectInformation{});
}

TEST(ApplicationProjectInformationTest, RegistersEveryFieldIncludingUnicodeAndMultilineNotes) {
    DsdApplication application;
    const ProjectInformation information{
        .project_name = L"\u0645\u0634\u0631\u0648\u0639",
        .project_number = L"PRJ-001",
        .building_name = L"Main building",
        .project_address = L"Project address",
        .client_name = L"Client",
        .organization_name = L"Organization",
        .author = L"Engineer",
        .level_name = L"Ground floor",
        .drawing_number = L"M-101",
        .revision = L"A",
        .additional_information = L"Design notes\r\nReview pending.",
    };

    application.register_project_information(information);

    ASSERT_TRUE(application.project_information());
    EXPECT_EQ(*application.project_information(), information);
}

TEST(ApplicationProjectInformationTest, RegistrationOwnsItsSnapshotOfCallerData) {
    DsdApplication application;
    ProjectInformation information;
    information.project_name = L"Original";
    application.register_project_information(information);

    information.project_name = L"Unsaved modification";

    ASSERT_TRUE(application.project_information());
    EXPECT_EQ(application.project_information()->project_name, L"Original");
}

TEST(ApplicationProjectInformationTest, ReregistrationReplacesPreviousValuesIncludingClearedFields) {
    DsdApplication application;
    ProjectInformation first;
    first.project_name = L"Old name";
    first.client_name = L"Previous client";
    application.register_project_information(first);
    ProjectInformation replacement;
    replacement.project_name = L"Updated name";

    application.register_project_information(replacement);

    ASSERT_TRUE(application.project_information());
    EXPECT_EQ(*application.project_information(), replacement);
}

TEST(ApplicationProjectInformationTest, RegisteringInOneApplicationDoesNotAffectAnother) {
    DsdApplication first;
    const DsdApplication second;

    first.register_project_information({});

    EXPECT_FALSE(second.project_information());
}

TEST(ApplicationProjectInformationTest, ClearRemovesRegisteredProjectInformation) {
    DsdApplication application;
    application.register_project_information({});

    application.clear_project_information();

    EXPECT_FALSE(application.project_information());
}

} // namespace
} // namespace dsd::application::test
