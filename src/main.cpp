#include <cstdlib>
#include <functional>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;

    bool IsComplete() {
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
    const int WIDTH = 800;
    const int HEIGHT = 600;

    GLFWwindow* m_pWindow = nullptr;
    vk::SurfaceKHR m_Surface;

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

        //extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

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

        m_Device = m_PhysicalDevice.createDevice(deviceCreateInfo);

        m_GraphicsQueue = m_Device.getQueue(m_QueueFamilyIndices.graphics.value(), 0);
        m_PresentQueue = m_Device.getQueue(m_QueueFamilyIndices.present.value(), 0);
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
