#pragma once
#include "../Vulkan/VulkanApp.h"

namespace ProjectJ{
    using RHIType = VulkanRHI;
    using RHIConfig = VulkanConfig;
    class RHI {
    public:
        static RHIType& Get(){
            return *gInstance;
        }
        static void Create(const RHIConfig& config){
            gInstance = new RHIType(config);
            gInstance->Init();
        }
        static void Destroy(){
            gInstance->Cleanup();
            delete gInstance;
        }
    private:
        static RHIType* gInstance;
    };
}