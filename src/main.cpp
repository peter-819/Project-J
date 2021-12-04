#include <Jpch.h>
#include "core/Application.h"

int main(char* argv,char** args) {
    ProjectJ::AppInfo appInfo;
    ProjectJ::Application app(appInfo);
    app.Run();
    return 0;
}