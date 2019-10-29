#include <cstdlib>
#include <functional>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;

    bool IsComplete()
    {
        return graphics.has_value() && present.has_value();
    }

    std::set<uint32_t> GetUniqueQueueFamilies()
    {
        std::set<uint32_t> indices;
        if (graphics.has_value()) indices.insert(graphics.value());
        if (present.has_value()) indices.insert(present.value());
        return indices;
    }
};

class VulkanApp
{
public:
    void Run()
    {
        CreateWindow();
        InitializeVulkan();
        MainLoop();
        Uninitialize();
    }

private:
    const uint32_t WIDTH = 1280;
    const uint32_t HEIGHT = 720;

    GLFWwindow* m_pWindow = nullptr;

    vk::SurfaceKHR m_Surface;
    VkSwapchainKHR m_SwapChain;
    std::vector<VkImage> m_SwapChainImages;
    std::vector<VkImageView> m_SwapChainImageViews;
    vk::Format m_SwapChainImageFormat;
    vk::Extent2D m_SwapChainExtent;

    vk::Instance m_Instance;
    vk::DebugUtilsMessengerEXT m_DebugMessenger;
    vk::PhysicalDevice m_PhysicalDevice;
    vk::Device m_Device;
    vk::Queue m_GraphicsQueue;
    vk::Queue m_PresentQueue;
    QueueFamilyIndices m_QueueFamilyIndices;

    void CreateWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        m_pWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    std::vector<const char*> GetValidationLayers()
    {
        std::vector<const char*> layers;

        static const std::vector<std::string> layersToEnable{
            "VK_LAYER_KHRONOS_validation"
        };

        const std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

        for (const std::string& layerToEnable : layersToEnable)
        {
            auto it = std::find_if(availableLayers.begin(), availableLayers.end(),
                [&](const vk::LayerProperties& p_Layer) { return std::string(p_Layer.layerName) == layerToEnable; });
            if (it == availableLayers.end())
            {
                std::cerr << "- Layer \"" << layerToEnable << "\" is not available, ignoring." << std::endl;
            }
            layers.push_back(layerToEnable.data());
        }

        return layers;
    }

    std::vector<const char*> GetExtensions()
    {
        std::vector<const char*> extensions;

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        for (uint32_t i = 0; i < glfwExtensionCount; i++) extensions.push_back(glfwExtensions[i]);

        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        return extensions;
    }

    void CreateInstance()
    {
        vk::ApplicationInfo appInfo;
        appInfo.pApplicationName = "Vulkan Playground";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        vk::InstanceCreateInfo createInfo;
        createInfo.pApplicationInfo = &appInfo;

        const auto layersToEnable = GetValidationLayers();
        if (!layersToEnable.empty())
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(layersToEnable.size());
            createInfo.ppEnabledLayerNames = layersToEnable.data();

            const vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = GetDebugCreateInfo();
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }

        const auto extensionsToEnable = GetExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionsToEnable.size());
        createInfo.ppEnabledExtensionNames = extensionsToEnable.empty() ? nullptr : extensionsToEnable.data();

        m_Instance = vk::createInstance(createInfo, nullptr);

        m_DebugMessenger = m_Instance.createDebugUtilsMessengerEXT(GetDebugCreateInfo(), nullptr, vk::DispatchLoaderDynamic(m_Instance));
    }

    void CreateSurface()
    {
        if (glfwCreateWindowSurface(m_Instance, m_pWindow, nullptr, reinterpret_cast<VkSurfaceKHR*>(&m_Surface)) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface.");
        }
    }

    void CreateDevice()
    {
        const std::vector<vk::PhysicalDevice> physicalDevices = m_Instance.enumeratePhysicalDevices();
        if (physicalDevices.empty()) throw std::runtime_error("Failed to find any GPUs supporting Vulkan");

        m_PhysicalDevice = physicalDevices[0];
        const vk::PhysicalDeviceProperties properties = m_PhysicalDevice.getProperties();
        std::cerr << "Selected device \"" << properties.deviceName << "\"" << std::endl;

        const std::vector<vk::QueueFamilyProperties> queueFamilyProperties = m_PhysicalDevice.getQueueFamilyProperties();
        for (uint32_t i = 0; i < queueFamilyProperties.size(); i++)
        {
            if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) m_QueueFamilyIndices.graphics = i;
            if (m_PhysicalDevice.getSurfaceSupportKHR(i, m_Surface)) m_QueueFamilyIndices.present = i;

            if (m_QueueFamilyIndices.IsComplete()) break;
        }
        if (!m_QueueFamilyIndices.IsComplete()) throw std::runtime_error("Failed to find a queue supporting graphics.");

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        const float queuePriority = 1.0f;
        for (uint32_t queueFamilyIndex : m_QueueFamilyIndices.GetUniqueQueueFamilies())
        {
            vk::DeviceQueueCreateInfo queueCreateInfo;
            queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        vk::PhysicalDeviceFeatures deviceFeatures;

        vk::DeviceCreateInfo deviceCreateInfo;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
        const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

        m_Device = m_PhysicalDevice.createDevice(deviceCreateInfo);

        m_GraphicsQueue = m_Device.getQueue(m_QueueFamilyIndices.graphics.value(), 0);
        m_PresentQueue = m_Device.getQueue(m_QueueFamilyIndices.present.value(), 0);
    }

    void CreateSwapChain()
    {
        // Surface format
        std::optional<vk::SurfaceFormatKHR> surfaceFormat;
        const std::vector<vk::SurfaceFormatKHR> formats = m_PhysicalDevice.getSurfaceFormatsKHR(m_Surface);
        for (const auto& format : formats) {
            if ((format.format == vk::Format::eB8G8R8A8Unorm) && (format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)) {
                surfaceFormat = format;
            }
        }
        if (!surfaceFormat.has_value() && !formats.empty()) surfaceFormat = formats[0];
        if (!surfaceFormat.has_value()) throw std::runtime_error("Failed to find suitable surface format.");

        // Present mode: Prefer Mailbox present mode, but use FIFO if not available. FIFO has to be supported.
        const std::vector<vk::PresentModeKHR> presentModes = m_PhysicalDevice.getSurfacePresentModesKHR(m_Surface);
        const bool isMailboxSupported = std::find(presentModes.begin(), presentModes.end(), vk::PresentModeKHR::eMailbox) != presentModes.end();
        const vk::PresentModeKHR presentMode = isMailboxSupported ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;

        // ROI of the Window
        const vk::SurfaceCapabilitiesKHR capabilities = m_PhysicalDevice.getSurfaceCapabilitiesKHR(m_Surface);
        const vk::Extent2D extent = (capabilities.currentExtent.width != UINT32_MAX) ?
            capabilities.currentExtent :
            vk::Extent2D(std::clamp(WIDTH, capabilities.minImageExtent.width, capabilities.maxImageExtent.width), std::clamp(HEIGHT, capabilities.minImageExtent.height, capabilities.maxImageExtent.height));
 
        // Chain length
        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0) imageCount = std::min(imageCount, capabilities.maxImageCount);

        vk::SwapchainCreateInfoKHR createInfo;
        createInfo.surface = m_Surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.value().format;
        createInfo.imageColorSpace = surfaceFormat.value().colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1; // Mono, 2 for stereo
        createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
        createInfo.preTransform = capabilities.currentTransform; // No transform
        createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque; // Ignore alpha when compositing window
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        uint32_t queueFamilyIndices[] = { m_QueueFamilyIndices.graphics.value(), m_QueueFamilyIndices.present.value() };
        if (m_QueueFamilyIndices.graphics != m_QueueFamilyIndices.present) {
            createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }

        m_SwapChain = m_Device.createSwapchainKHR(createInfo);
        m_SwapChainImageFormat = createInfo.imageFormat;
        m_SwapChainExtent = createInfo.imageExtent;

        // Get images and image views
        for (const vk::Image& image : m_Device.getSwapchainImagesKHR(m_SwapChain)) m_SwapChainImages.push_back(image);

        for (const vk::Image& image : m_SwapChainImages)
        {
            vk::ImageViewCreateInfo createInfo;
            createInfo.image = image;
            createInfo.viewType = vk::ImageViewType::e2D;
            createInfo.format = m_SwapChainImageFormat;
            createInfo.components.r = vk::ComponentSwizzle::eIdentity;
            createInfo.components.g = vk::ComponentSwizzle::eIdentity;;
            createInfo.components.b = vk::ComponentSwizzle::eIdentity;;
            createInfo.components.a = vk::ComponentSwizzle::eIdentity;;
            createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            m_SwapChainImageViews.push_back(m_Device.createImageView(createInfo));
        }
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
    {
        std::cerr << "Validation: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE; // True aborts the Vulkan call that triggered the validation layer
    }

    vk::DebugUtilsMessengerCreateInfoEXT GetDebugCreateInfo()
    {
        vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
        debugCreateInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
        debugCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
        debugCreateInfo.pfnUserCallback = debugCallback;
        return debugCreateInfo;
    }

    void InitializeVulkan()
    {
        CreateInstance();
        CreateSurface(); // Should be called before createDevice() as it may affect the query results
        CreateDevice();
        CreateSwapChain();
    }

    void MainLoop()
    {
        while (!glfwWindowShouldClose(m_pWindow))
        {
            glfwPollEvents();
        }
    }

    void Uninitialize()
    {
        for (auto& imageView : m_SwapChainImageViews) m_Device.destroyImageView(imageView);
        m_SwapChainImageViews.clear();

        m_SwapChainImages.clear(); // No need to destroy, we didn't create them

        m_Device.destroySwapchainKHR(m_SwapChain);
        m_Device.destroy();
        m_Instance.destroySurfaceKHR(m_Surface);
        m_Instance.destroyDebugUtilsMessengerEXT(m_DebugMessenger, nullptr, vk::DispatchLoaderDynamic(m_Instance));
        m_Instance.destroy();

        glfwDestroyWindow(m_pWindow);

        glfwTerminate();
    }
};

int main()
{
    VulkanApp app;

    try {
        app.Run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
