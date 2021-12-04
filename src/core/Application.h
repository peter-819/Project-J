#pragma once

namespace ProjectJ{
    struct AppInfo{
        
    };

    class Application{
    public:
        Application(const AppInfo& appInfo);
        void Run();
    };
}