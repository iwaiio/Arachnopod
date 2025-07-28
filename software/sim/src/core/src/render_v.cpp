#include "render_v.hpp"

const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

void RenderManager::version() {
    std::cout << "LAUNCH!" << std::endl;

    uint32_t version = 0;
    vkEnumerateInstanceVersion(&version);
    std::cout << "Vulkan API version: " 
            << VK_VERSION_MAJOR(version) << "."
            << VK_VERSION_MINOR(version) << "."
            << VK_VERSION_PATCH(version) << std::endl;
};

RenderManager::RenderManager(uint32_t width, uint32_t height, const std::string& title)
    : width_(width), height_(height), title_(title) {
    initWindow();
    initVulkan();
}

RenderManager::~RenderManager() {
    device_.waitIdle();

    for (auto framebuffer : swapChainFramebuffers_) {
        device_.destroyFramebuffer(framebuffer);
    }
    device_.destroyPipeline(graphicsPipeline_);
    device_.destroyPipelineLayout(pipelineLayout_);
    device_.destroyRenderPass(renderPass_);

    for (auto imageView : swapChainImageViews_) {
        device_.destroyImageView(imageView);
    }
    device_.destroySwapchainKHR(swapChain_);
    device_.destroy();

    if (enableValidationLayers) {
        instance_.destroyDebugUtilsMessengerEXT(debugMessenger_, nullptr, vk::DispatchLoaderDynamic(instance_));
    }

    instance_.destroySurfaceKHR(surface_);
    instance_.destroy();

    glfwDestroyWindow(window_);
    glfwTerminate();
}

void RenderManager::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);  // Отключаем OpenGL
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);    // Запрещаем изменение размера
    window_ = glfwCreateWindow(width_, height_, title_.c_str(), nullptr, nullptr);
}

void RenderManager::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
}

// ... (остальные методы, такие как createInstance(), pickPhysicalDevice() и т.д.)

void RenderManager::run() {
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        drawFrame();
    }
    device_.waitIdle();
}

void RenderManager::drawFrame() {
    device_.waitForFences(1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    auto result = device_.acquireNextImageKHR(
        swapChain_, UINT64_MAX, imageAvailableSemaphores_[currentFrame_], nullptr, &imageIndex);

    if (result == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapChain();
        return;
    }

    vk::SubmitInfo submitInfo;
    vk::Semaphore waitSemaphores[] = {imageAvailableSemaphores_[currentFrame_]};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers_[imageIndex];

    vk::Semaphore signalSemaphores[] = {renderFinishedSemaphores_[currentFrame_]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    device_.resetFences(1, &inFlightFences_[currentFrame_]);
    graphicsQueue_.submit(1, &submitInfo, inFlightFences_[currentFrame_]);

    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    vk::SwapchainKHR swapChains[] = {swapChain_};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = presentQueue_.presentKHR(&presentInfo);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        recreateSwapChain();
    }

    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}
