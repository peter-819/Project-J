#include <Jpch.h>
#include "VulkanCommand.h"

namespace ProjectJ{
    VulkanQueue::VulkanQueue(){    
        vkGetDeviceQueue(RHI::Get().mDevice,RHI::Get().mQueueFamilyIndices.graphicsFamily.value(),0,&mGraphicQueue);
        vkGetDeviceQueue(RHI::Get().mDevice,RHI::Get().mQueueFamilyIndices.presentFamily.value(),0,&mPresentQueue);
    
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = RHI::Get().mQueueFamilyIndices.graphicsFamily.value();
        poolInfo.flags = 0;
        VK_CHECK(vkCreateCommandPool(RHI::Get().mDevice,&poolInfo,nullptr,&mCommandPool),"failed to create command pool.");
        
        PCreateSyncObjects();
        AllocCommandBuffer(RHI::Get().mSwapChain->GetImageCount(),mFrameCommandBuffers);
    }
    VulkanQueue::~VulkanQueue(){ 
        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
            vkDestroySemaphore(RHI::Get().mDevice,mRenderFinishedSemaphores[i],nullptr);
            vkDestroySemaphore(RHI::Get().mDevice,mImageAvailableSemaphores[i],nullptr);
            vkDestroyFence(RHI::Get().mDevice,mInFlightFences[i],nullptr);
        }
        vkDestroyCommandPool(RHI::Get().mDevice,mCommandPool,nullptr);
        
    }
    VkCommandBuffer VulkanQueue::AllocCommandBuffer(){
        VkCommandBuffer commandBuffer;
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = mCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        VK_CHECK(vkAllocateCommandBuffers(RHI::Get().mDevice,&allocInfo,&commandBuffer),"failed to allocate command buffer.");
        return commandBuffer;
    }
    void VulkanQueue::AllocCommandBuffer(uint32_t count,std::vector<VkCommandBuffer>& commandBuffers){
        commandBuffers.resize(count);
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = mCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();
        VK_CHECK(vkAllocateCommandBuffers(RHI::Get().mDevice,&allocInfo,commandBuffers.data()),"failed to allocate command buffers.");
    }

    void VulkanQueue::PrepareFrameCommands(std::function<void(uint32_t, VkCommandBuffer&)> func){
        for(uint32_t i = 0; i < mFrameCommandBuffers.size(); i++){
            func(i, mFrameCommandBuffers[i]);
        }
    }
    
    void VulkanQueue::ExecuteDirectly(std::function<void(VkCommandBuffer&)> func){
        auto commandBuffer = AllocCommandBuffer();
        func(commandBuffer);
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(mGraphicQueue,1,&submitInfo,VK_NULL_HANDLE);
        vkQueueWaitIdle(mGraphicQueue);
        vkFreeCommandBuffers(RHI::Get().mDevice,mCommandPool,1,&commandBuffer);
    }

    void VulkanQueue::BeginFrame(){ 
        vkWaitForFences(RHI::Get().mDevice,1,&mInFlightFences[mCurrentFrame],VK_TRUE,UINT64_MAX);
        mImageIndex = RHI::Get().mSwapChain->AcquireNextImage(mImageAvailableSemaphores[mCurrentFrame]);
        
    }
    void VulkanQueue::EndFrame(){
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {mImageAvailableSemaphores[mCurrentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &mFrameCommandBuffers[mImageIndex];
        VkSemaphore signalSemaphores[] = {mRenderFinishedSemaphores[mCurrentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        
        vkResetFences(RHI::Get().mDevice,1,&mInFlightFences[mCurrentFrame]);
        VK_CHECK(vkQueueSubmit(mGraphicQueue,1,&submitInfo,mInFlightFences[mCurrentFrame]),"failed to submit draw command buffer.");

        RHI::Get().mSwapChain->Present(mPresentQueue,mRenderFinishedSemaphores[mCurrentFrame]);
        mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }
    void VulkanQueue::PCreateSyncObjects(){
        mImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        mRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        mInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
            VK_CHECK(vkCreateSemaphore(RHI::Get().mDevice,&semaphoreInfo,nullptr,&mImageAvailableSemaphores[i]),"failed to create semaphores.");
            VK_CHECK(vkCreateSemaphore(RHI::Get().mDevice,&semaphoreInfo,nullptr,&mRenderFinishedSemaphores[i]),"failed to create semaphores.");    
            VK_CHECK(vkCreateFence(RHI::Get().mDevice,&fenceInfo,nullptr,&mInFlightFences[i]),"failed to create fence.");
        }
    }

    //------------------------------------ScopedFrame-----------------------------------------//
    ScopedFrame::ScopedFrame(std::shared_ptr<VulkanQueue> queue){
        Queue = queue;
        Queue->BeginFrame();
        ImageIndex = queue->mImageIndex;
    
    }
    ScopedFrame::~ScopedFrame(){
        Queue->EndFrame();
    }
}