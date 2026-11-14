#include "autocad/ui/model/ApplicationSession.h"

dsd::application::DsdApplication& DuctUi::Application() {
    static dsd::application::DsdApplication application;
    return application;
}
