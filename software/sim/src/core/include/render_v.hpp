#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <string>
#include <stdexcept>
#include <vector>
#include <optional>
#include <iostream>

class RenderManager {
public:
    void version();
    RenderManager(uint32_t width, uint32_t height, const std::string& title);
    ~RenderManager();

    void run();

private:
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    // Инициализация
    void initWindow();
    void initVulkan();
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();

    // Вспомогательные методы
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device);
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

    // Члены класса
    GLFWwindow* window_;
    uint32_t width_;
    uint32_t height_;
    std::string title_;

    vk::Instance instance_;
    vk::SurfaceKHR surface_;
    vk::PhysicalDevice physicalDevice_;
    vk::Device device_;
    vk::Queue graphicsQueue_;
    vk::Queue presentQueue_;
    vk::SwapchainKHR swapChain_;
    std::vector<vk::Image> swapChainImages_;
    vk::Format swapChainImageFormat_;
    vk::Extent2D swapChainExtent_;
    std::vector<vk::ImageView> swapChainImageViews_;
    vk::RenderPass renderPass_;
    vk::PipelineLayout pipelineLayout_;
    vk::Pipeline graphicsPipeline_;
    std::vector<vk::Framebuffer> swapChainFramebuffers_;
    vk::CommandPool commandPool_;
    std::vector<vk::CommandBuffer> commandBuffers_;
    std::vector<vk::Semaphore> imageAvailableSemaphores_;
    std::vector<vk::Semaphore> renderFinishedSemaphores_;
    std::vector<vk::Fence> inFlightFences_;
    size_t currentFrame_ = 0;
};