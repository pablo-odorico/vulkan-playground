#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include <set>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <sstream>
#include <optional>

struct QueueFamilyIndices {
	std::optional<uint32_t> graphics;
	std::optional<uint32_t> present;

	bool isComplete() {
		return graphics.has_value() && present.has_value();
	}

	std::set<uint32_t> getUniqueQueueFamilies()
	{
		std::set<uint32_t> indices;
		if (graphics.has_value()) indices.insert(graphics.value());
		if (present.has_value()) indices.insert(present.value());
		return indices;
	}
};

class HelloTriangleApplication {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	const int WIDTH = 800;
	const int HEIGHT = 600;

	GLFWwindow* window = nullptr;
	vk::SurfaceKHR surface;

	vk::Instance instance;
	vk::DebugUtilsMessengerEXT messenger;
	vk::PhysicalDevice physicalDevice;
	vk::Device device;
	vk::Queue graphicsQueue;
	vk::Queue presentQueue;
	QueueFamilyIndices queueFamilyIndices;

	void initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	}

	std::vector<const char*> getValidationLayers()
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

	std::vector<const char*> getExtensions()
	{
		std::vector<const char*> extensions;

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		for (uint32_t i = 0; i < glfwExtensionCount; i++) extensions.push_back(glfwExtensions[i]);

		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		return extensions;
	}

	void createInstance()
	{
		vk::ApplicationInfo appInfo;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		vk::InstanceCreateInfo createInfo;
		createInfo.pApplicationInfo = &appInfo;

		const auto layersToEnable = getValidationLayers();
		if (!layersToEnable.empty())
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(layersToEnable.size());
			createInfo.ppEnabledLayerNames = layersToEnable.data();

			const vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = getDebugCreateInfo();
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}

		const auto extensionsToEnable = getExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionsToEnable.size());
		createInfo.ppEnabledExtensionNames = extensionsToEnable.empty() ? nullptr : extensionsToEnable.data();

		instance = vk::createInstance(createInfo, nullptr);

		messenger = instance.createDebugUtilsMessengerEXT(getDebugCreateInfo(), nullptr, vk::DispatchLoaderDynamic(instance));
	}

	void createSurface()
	{
		if (glfwCreateWindowSurface(instance, window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&surface)) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create window surface.");
		}
	}

	void createDevice()
	{
		const std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
		if (physicalDevices.empty()) throw std::runtime_error("Failed to find any GPUs supporting Vulkan");

		physicalDevice = physicalDevices[0];
		const vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
		std::cerr << "Selected device \"" << properties.deviceName << "\"" << std::endl;

		const std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
		for (uint32_t i = 0; i < queueFamilyProperties.size(); i++)
		{
			if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) queueFamilyIndices.graphics = i;
			if (physicalDevice.getSurfaceSupportKHR(i, surface)) queueFamilyIndices.present = i;

			if (queueFamilyIndices.isComplete()) break;
		}
		if (!queueFamilyIndices.isComplete()) throw std::runtime_error("Failed to find a queue supporting graphics.");

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		const float queuePriority = 1.0f;
		for (uint32_t queueFamilyIndex : queueFamilyIndices.getUniqueQueueFamilies())
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

		device = physicalDevice.createDevice(deviceCreateInfo);

		graphicsQueue = device.getQueue(queueFamilyIndices.graphics.value(), 0);
		presentQueue = device.getQueue(queueFamilyIndices.present.value(), 0);
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		std::cerr << "Validation: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE; // True aborts the Vulkan call that triggered the validation layer
	}

	vk::DebugUtilsMessengerCreateInfoEXT getDebugCreateInfo()
	{
		vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
		debugCreateInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning; 
		debugCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
		debugCreateInfo.pfnUserCallback = debugCallback;
		return debugCreateInfo;
	}

	void initVulkan()
	{
		createInstance();
		createSurface(); // Should be called before createDevice() as it may affect the query results
		createDevice();
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
		}
	}

	void cleanup()
	{
		device.destroy();
		instance.destroySurfaceKHR(surface);
		instance.destroyDebugUtilsMessengerEXT(messenger, nullptr, vk::DispatchLoaderDynamic(instance));
		instance.destroy();

		glfwDestroyWindow(window);

		glfwTerminate();
	}
};

int main() {
	HelloTriangleApplication app;

	try {
		app.run();
	}
	catch (const std::exception & e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}