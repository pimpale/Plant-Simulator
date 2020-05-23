#include "vulkan_utils.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan.h>

#include <linmath.h>

#include "constants.h"
#include "errors.h"
#include "utils.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
  UNUSED(messageType);
  UNUSED(pUserData);

  /* set severity */
  ErrSeverity errSeverity = ERR_LEVEL_UNKNOWN;
  switch (messageSeverity) {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    errSeverity = ERR_LEVEL_INFO;
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    errSeverity = ERR_LEVEL_INFO;
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    errSeverity = ERR_LEVEL_WARN;
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    errSeverity = ERR_LEVEL_ERROR;
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
    errSeverity = ERR_LEVEL_UNKNOWN;
    break;
  }
  /* log error */
  LOG_ERROR_ARGS(errSeverity, "vulkan validation layer: %s",
                 pCallbackData->pMessage);
  return (VK_FALSE);
}

/* Get required extensions for a VkInstance */
ErrVal new_RequiredInstanceExtensions(uint32_t *pEnabledExtensionCount,
                                      char ***pppEnabledExtensionNames) {
  /* define our own extensions */
  /* get GLFW extensions to use */
  uint32_t glfwExtensionCount = 0;
  const char **ppGlfwExtensionNames =
      glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  *pEnabledExtensionCount = 1 + glfwExtensionCount;
  *pppEnabledExtensionNames =
      (char **)malloc(sizeof(char *) * (*pEnabledExtensionCount));

  if (!(*pppEnabledExtensionNames)) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL, "failed to get required extensions: %s",
                   strerror(errno));
    PANIC();
  }

  /* Allocate buffers for extensions */
  for (uint32_t i = 0; i < *pEnabledExtensionCount; i++) {
    (*pppEnabledExtensionNames)[i] = (char *)malloc(VK_MAX_EXTENSION_NAME_SIZE);
  }
  /* Copy our extensions in  (we're malloccing everything to make it
   * simple to deallocate at the end without worrying about what needs to
   * be freed or not) */
  strncpy((*pppEnabledExtensionNames)[0], VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
          VK_MAX_EXTENSION_NAME_SIZE);
  for (uint32_t i = 0; i < glfwExtensionCount; i++) {
    strncpy((*pppEnabledExtensionNames)[i + 1], ppGlfwExtensionNames[i],
            VK_MAX_EXTENSION_NAME_SIZE);
  }
  return (ERR_OK);
}

/* Deletes char arrays allocated in new_RequiredInstanceExtensions */
void delete_RequiredInstanceExtensions(uint32_t *pEnabledExtensionCount,
                                       char ***pppEnabledExtensionNames) {
  for (uint32_t i = 0; i < *pEnabledExtensionCount; i++) {
    free((*pppEnabledExtensionNames)[i]);
  }
  free(*pppEnabledExtensionNames);
}

/* Get required layer names for validation layers */
ErrVal new_ValidationLayers(uint32_t *pLayerCount, char ***pppLayerNames) {
  *pLayerCount = 1;
  *pppLayerNames = (char **)malloc(sizeof(char *) * sizeof(*pLayerCount));
  (*pppLayerNames)[0] = "VK_LAYER_KHRONOS_validation";
  return (ERR_OK);
}

/* Delete validation layer names allocated in new_ValidationLayers */
void delete_ValidationLayers(uint32_t *pLayerCount, char ***pppLayerNames) {
  UNUSED(pLayerCount);
  free(*pppLayerNames);
}

/* Get array of required device extensions for running graphics (swapchain) */
ErrVal new_RequiredDeviceExtensions(uint32_t *pEnabledExtensionCount,
                                    char ***pppEnabledExtensionNames) {
  *pEnabledExtensionCount = 1;
  *pppEnabledExtensionNames =
      (char **)malloc(sizeof(char *) * sizeof(*pEnabledExtensionCount));
  **pppEnabledExtensionNames = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
  return (ERR_OK);
}

/* Delete array allocated in new_RequiredDeviceExtensions */
void delete_RequiredDeviceExtensions(uint32_t *pEnabledExtensionCount,
                                     char ***pppEnabledExtensionNames) {
  UNUSED(pEnabledExtensionCount);
  free(*pppEnabledExtensionNames);
}

/* Creates new VkInstance with sensible defaults */
ErrVal new_Instance(VkInstance *pInstance, const uint32_t enabledExtensionCount,
                    const char *const *ppEnabledExtensionNames,
                    const uint32_t enabledLayerCount,
                    const char *const *ppEnabledLayerNames) {
  /* Create app info */
  VkApplicationInfo appInfo = {0};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = APPNAME;
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "None";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  /* Create info */
  VkInstanceCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = enabledExtensionCount;
  createInfo.ppEnabledExtensionNames = ppEnabledExtensionNames;
  createInfo.enabledLayerCount = enabledLayerCount;
  createInfo.ppEnabledLayerNames = ppEnabledLayerNames;
  /* Actually create instance */
  VkResult result = vkCreateInstance(&createInfo, NULL, pInstance);
  if (result != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL, "Failed to create instance, error code: %s",
                   vkstrerror(result));
    PANIC();
  }
  return (ERR_OK);
}

/* Destroys instance created in new_Instance */
void delete_Instance(VkInstance *pInstance) {
  vkDestroyInstance(*pInstance, NULL);
  *pInstance = VK_NULL_HANDLE;
}

/**
 * Requires the debug utils extension
 *
 * Creates a new debug callback that prints validation layer errors to stdout or
 * stderr, depending on their severity
 */
ErrVal new_DebugCallback(VkDebugUtilsMessengerEXT *pCallback,
                         const VkInstance instance) {
  VkDebugUtilsMessengerCreateInfoEXT createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;

  /* Returns a function pointer */
  PFN_vkCreateDebugUtilsMessengerEXT func =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkCreateDebugUtilsMessengerEXT");
  if (!func) {
    LOG_ERROR(ERR_LEVEL_FATAL, "Failed to find extension function");
    PANIC();
  }
  VkResult result = func(instance, &createInfo, NULL, pCallback);
  if (result != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL,
                   "Failed to create debug callback, error code: %s",
                   vkstrerror(result));
    PANIC();
  }
  return (ERR_NOTSUPPORTED);
}

/**
 * Requires the debug utils extension
 * Deletes callback created in new_DebugCallback
 */
void delete_DebugCallback(VkDebugUtilsMessengerEXT *pCallback,
                          const VkInstance instance) {
  PFN_vkDestroyDebugUtilsMessengerEXT func =
      (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != NULL) {
    func(instance, *pCallback, NULL);
  }
}
/**
 * gets the best physical device, checks if it has all necessary capabilities.
 */
ErrVal getPhysicalDevice(VkPhysicalDevice *pDevice, const VkInstance instance) {
  uint32_t deviceCount = 0;
  VkResult res = vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
  if (res != VK_SUCCESS || deviceCount == 0) {
    LOG_ERROR(ERR_LEVEL_WARN, "no Vulkan capable device found");
    return (ERR_NOTSUPPORTED);
  }
  VkPhysicalDevice *arr = malloc(deviceCount * sizeof(VkPhysicalDevice));
  if (!arr) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL, "failed to get physical device: %s",
                   strerror(errno));
    PANIC();
  }
  vkEnumeratePhysicalDevices(instance, &deviceCount, arr);

  VkPhysicalDeviceProperties deviceProperties;
  VkPhysicalDevice selectedDevice = VK_NULL_HANDLE;
  for (uint32_t i = 0; i < deviceCount; i++) {
    /* TODO confirm it has required properties */
    vkGetPhysicalDeviceProperties(arr[i], &deviceProperties);
    uint32_t deviceQueueIndex;
    uint32_t ret =
        getDeviceQueueIndex(&deviceQueueIndex, arr[i],
                            VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
    if (ret == VK_SUCCESS) {
      selectedDevice = arr[i];
      break;
    }
  }
  free(arr);
  if (selectedDevice == VK_NULL_HANDLE) {
    LOG_ERROR(ERR_LEVEL_WARN, "no suitable Vulkan device found");
    return (ERR_NOTSUPPORTED);
  } else {
    *pDevice = selectedDevice;
    return (ERR_OK);
  }
}

/**
 * Deletes VkDevice created in new_Device
 */
void delete_Device(VkDevice *pDevice) { vkDestroyDevice(*pDevice, NULL); }

/**
 * Sets deviceQueueIndex to queue family index corresponding to the bit passed
 * in for the device
 */
ErrVal getDeviceQueueIndex(uint32_t *deviceQueueIndex,
                           const VkPhysicalDevice device,
                           const VkQueueFlags bit) {
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);
  if (queueFamilyCount == 0) {
    LOG_ERROR(ERR_LEVEL_WARN, "no device queues found");
    return (ERR_NOTSUPPORTED);
  }
  VkQueueFamilyProperties *pFamilyProperties =
      (VkQueueFamilyProperties *)malloc(queueFamilyCount *
                                        sizeof(VkQueueFamilyProperties));
  if (!pFamilyProperties) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL, "Failed to get device queue index: %s",
                   strerror(errno));
    PANIC();
  }
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           pFamilyProperties);
  for (uint32_t i = 0; i < queueFamilyCount; i++) {
    if (pFamilyProperties[i].queueCount > 0 &&
        (pFamilyProperties[0].queueFlags & bit)) {
      free(pFamilyProperties);
      *deviceQueueIndex = i;
      return (ERR_OK);
    }
  }
  free(pFamilyProperties);
  LOG_ERROR(ERR_LEVEL_ERROR, "no suitable device queue found");
  return (ERR_NOTSUPPORTED);
}

ErrVal getPresentQueueIndex(uint32_t *pPresentQueueIndex,
                            const VkPhysicalDevice physicalDevice,
                            const VkSurfaceKHR surface) {
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           NULL);
  if (queueFamilyCount == 0) {
    LOG_ERROR(ERR_LEVEL_WARN, "no queues found");
    return (ERR_NOTSUPPORTED);
  }
  VkQueueFamilyProperties *arr = (VkQueueFamilyProperties *)malloc(
      queueFamilyCount * sizeof(VkQueueFamilyProperties));
  if (!arr) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL, "Failed to get present queue index: %s",
                   strerror(errno));
    PANIC();
  }
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           arr);
  for (uint32_t i = 0; i < queueFamilyCount; i++) {
    VkBool32 surfaceSupport;
    VkResult res = vkGetPhysicalDeviceSurfaceSupportKHR(
        physicalDevice, i, surface, &surfaceSupport);
    if (res == VK_SUCCESS && surfaceSupport) {
      *pPresentQueueIndex = i;
      free(arr);
      return (ERR_OK);
    }
  }
  free(arr);
  return (ERR_NOTSUPPORTED);
}

ErrVal new_Device(VkDevice *pDevice, const VkPhysicalDevice physicalDevice,
                  const uint32_t deviceQueueIndex,
                  const uint32_t enabledExtensionCount,
                  const char *const *ppEnabledExtensionNames,
                  const uint32_t enabledLayerCount,
                  const char *const *ppEnabledLayerNames) {
  VkPhysicalDeviceFeatures deviceFeatures = {0};
  VkDeviceQueueCreateInfo queueCreateInfo = {0};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = deviceQueueIndex;
  queueCreateInfo.queueCount = 1;
  float queuePriority = 1.0f;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  VkDeviceCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = &queueCreateInfo;
  createInfo.queueCreateInfoCount = 1;
  createInfo.pEnabledFeatures = &deviceFeatures;
  createInfo.enabledExtensionCount = enabledExtensionCount;
  createInfo.ppEnabledExtensionNames = ppEnabledExtensionNames;
  createInfo.enabledLayerCount = enabledLayerCount;
  createInfo.ppEnabledLayerNames = ppEnabledLayerNames;

  VkResult res = vkCreateDevice(physicalDevice, &createInfo, NULL, pDevice);
  if (res != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_ERROR, "Failed to create device, error code: %s",
                   vkstrerror(res));
    PANIC();
  }
  return (ERR_OK);
}

ErrVal getQueue(VkQueue *pQueue, const VkDevice device,
                const uint32_t deviceQueueIndex) {
  vkGetDeviceQueue(device, deviceQueueIndex, 0, pQueue);
  return (ERR_OK);
}

ErrVal new_SwapChain(VkSwapchainKHR *pSwapChain, uint32_t *pSwapChainImageCount,
                     const VkSwapchainKHR oldSwapChain,
                     const VkSurfaceFormatKHR surfaceFormat,
                     const VkPhysicalDevice physicalDevice,
                     const VkDevice device, const VkSurfaceKHR surface,
                     const VkExtent2D extent, const uint32_t graphicsIndex,
                     const uint32_t presentIndex) {
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                            &capabilities);

  *pSwapChainImageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 &&
      *pSwapChainImageCount > capabilities.maxImageCount) {
    *pSwapChainImageCount = capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;
  createInfo.minImageCount = *pSwapChainImageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  uint32_t queueFamilyIndices[] = {graphicsIndex, presentIndex};
  if (graphicsIndex != presentIndex) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;  /* Optional */
    createInfo.pQueueFamilyIndices = NULL; /* Optional */
  }

  createInfo.preTransform = capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  /* guaranteed to be available */
  createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = oldSwapChain;
  VkResult res = vkCreateSwapchainKHR(device, &createInfo, NULL, pSwapChain);
  if (res != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_ERROR,
                   "Failed to create swap chain, error code: %s",
                   vkstrerror(res));
    PANIC();
  }
  return (ERR_OK);
}

void delete_SwapChain(VkSwapchainKHR *pSwapChain, const VkDevice device) {
  vkDestroySwapchainKHR(device, *pSwapChain, NULL);
  *pSwapChain = VK_NULL_HANDLE;
}

ErrVal getPreferredSurfaceFormat(VkSurfaceFormatKHR *pSurfaceFormat,
                                 const VkPhysicalDevice physicalDevice,
                                 const VkSurfaceKHR surface) {
  uint32_t formatCount = 0;
  VkSurfaceFormatKHR *pSurfaceFormats;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                       NULL);
  if (formatCount == 0) {
    return (ERR_NOTSUPPORTED);
  }

  pSurfaceFormats =
      (VkSurfaceFormatKHR *)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
  if (!pSurfaceFormats) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL, "could not get preferred format: %s",
                   strerror(errno));
    PANIC();
  }
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                       pSurfaceFormats);

  VkSurfaceFormatKHR preferredFormat = {0};
  if (formatCount == 1 && pSurfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
    /* If it has no preference, use our own */
    preferredFormat = pSurfaceFormats[0];
  } else if (formatCount != 0) {
    /* we default to the first one in the list */
    preferredFormat = pSurfaceFormats[0];
    /* However,  we check to make sure that what we want is in there
     */
    for (uint32_t i = 0; i < formatCount; i++) {
      VkSurfaceFormatKHR availableFormat = pSurfaceFormats[i];
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
          availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        preferredFormat = availableFormat;
      }
    }
  } else {
    LOG_ERROR(ERR_LEVEL_ERROR, "no formats available");
    free(pSurfaceFormats);
    return (ERR_NOTSUPPORTED);
  }

  free(pSurfaceFormats);

  *pSurfaceFormat = preferredFormat;
  return (ERR_OK);
}

ErrVal new_SwapChainImages(VkImage **ppSwapChainImages, uint32_t *pImageCount,
                           const VkDevice device,
                           const VkSwapchainKHR swapChain) {
  vkGetSwapchainImagesKHR(device, swapChain, pImageCount, NULL);

  if (pImageCount == 0) {
    LOG_ERROR(ERR_LEVEL_WARN, "cannot create zero images");
    return (ERR_UNSAFE);
  }

  *ppSwapChainImages = (VkImage *)malloc((*pImageCount) * sizeof(VkImage));
  if (!*ppSwapChainImages) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL, "failed to get swap chain images: %s",
                   strerror(errno));
    PANIC();
  }
  VkResult res = vkGetSwapchainImagesKHR(device, swapChain, pImageCount,
                                         *ppSwapChainImages);
  if (res != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_WARN, "failed to get swap chain images, error: %s",
                   vkstrerror(res));
    return (ERR_UNKNOWN);
  } else {
    return (ERR_OK);
  }
}

void delete_SwapChainImages(VkImage **ppImages) {
  free(*ppImages);
  *ppImages = NULL;
}

ErrVal new_ImageView(VkImageView *pImageView, const VkDevice device,
                     const VkImage image, const VkFormat format,
                     const uint32_t aspectMask) {
  VkImageViewCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  createInfo.image = image;
  createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  createInfo.format = format;
  createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  createInfo.subresourceRange.aspectMask = aspectMask;
  createInfo.subresourceRange.baseMipLevel = 0;
  createInfo.subresourceRange.levelCount = 1;
  createInfo.subresourceRange.baseArrayLayer = 0;
  createInfo.subresourceRange.layerCount = 1;
  VkResult ret = vkCreateImageView(device, &createInfo, NULL, pImageView);
  if (ret != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL,
                   "could not create image view, error code: %s",
                   vkstrerror(ret));
    PANIC();
  }
  return (ERR_OK);
}

void delete_ImageView(VkImageView *pImageView, VkDevice device) {
  vkDestroyImageView(device, *pImageView, NULL);
  *pImageView = VK_NULL_HANDLE;
}

ErrVal new_SwapChainImageViews(VkImageView **ppImageViews,
                               const VkDevice device, const VkFormat format,
                               const uint32_t imageCount,
                               const VkImage *pSwapChainImages) {
  if (imageCount == 0) {
    LOG_ERROR(ERR_LEVEL_WARN, "cannot create zero image views");
    return (ERR_BADARGS);
  }
  VkImageView *pImageViews =
      (VkImageView *)malloc(imageCount * sizeof(VkImageView));
  if (!pImageViews) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL,
                   "could not create swap chain image views: %s",
                   strerror(errno));
    PANIC();
  }

  for (uint32_t i = 0; i < imageCount; i++) {
    ErrVal ret = new_ImageView(&(pImageViews[i]), device, pSwapChainImages[i],
                               format, VK_IMAGE_ASPECT_COLOR_BIT);
    if (ret != ERR_OK) {
      LOG_ERROR(ERR_LEVEL_ERROR, "could not create swap chain image views");
      delete_SwapChainImageViews(ppImageViews, i, device);
      return (ret);
    }
  }

  *ppImageViews = pImageViews;
  return (ERR_OK);
}

void delete_SwapChainImageViews(VkImageView **ppImageViews, uint32_t imageCount,
                                const VkDevice device) {
  for (uint32_t i = 0; i < imageCount; i++) {
    delete_ImageView(&((*ppImageViews)[i]), device);
  }
  free(*ppImageViews);
  *ppImageViews = NULL;
}

ErrVal new_ShaderModule(VkShaderModule *pShaderModule, const VkDevice device,
                        const uint32_t codeSize, const uint32_t *pCode) {
  VkShaderModuleCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = codeSize;
  createInfo.pCode = pCode;
  VkResult res = vkCreateShaderModule(device, &createInfo, NULL, pShaderModule);
  if (res != VK_SUCCESS) {
    LOG_ERROR(ERR_LEVEL_FATAL, "failed to create shader module");
    return (ERR_UNKNOWN);
  }
  return (ERR_OK);
}

ErrVal new_ShaderModuleFromFile(VkShaderModule *pShaderModule,
                                const VkDevice device, char *filename) {
  uint32_t *shaderFileContents;
  uint32_t shaderFileLength;
  readShaderFile(filename, &shaderFileLength, &shaderFileContents);
  ErrVal retVal = new_ShaderModule(pShaderModule, device, shaderFileLength,
                                   shaderFileContents);
  if (retVal != ERR_OK) {
    LOG_ERROR_ARGS(ERR_LEVEL_ERROR,
                   "failed to create shader module from file %s", filename);
  }
  free(shaderFileContents);
  return (retVal);
}

void delete_ShaderModule(VkShaderModule *pShaderModule, const VkDevice device) {
  vkDestroyShaderModule(device, *pShaderModule, NULL);
  *pShaderModule = VK_NULL_HANDLE;
}

ErrVal new_VertexDisplayRenderPass(VkRenderPass *pRenderPass,
                                   const VkDevice device,
                                   const VkFormat swapChainImageFormat) {
  VkAttachmentDescription colorAttachment = {0};
  colorAttachment.format = swapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentDescription depthAttachment = {0};
  getDepthFormat(&depthAttachment.format);
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription pAttachments[2];
  pAttachments[0] = colorAttachment;
  pAttachments[1] = depthAttachment;

  VkAttachmentReference colorAttachmentRef = {0};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef = {0};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {0};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;

  VkRenderPassCreateInfo renderPassInfo = {0};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 2;
  renderPassInfo.pAttachments = pAttachments;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  VkSubpassDependency dependency = {0};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  VkResult res = vkCreateRenderPass(device, &renderPassInfo, NULL, pRenderPass);
  if (res != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL, "Could not create render pass, error: %s",
                   vkstrerror(res));
    PANIC();
  }
  return (ERR_OK);
}

void delete_RenderPass(VkRenderPass *pRenderPass, const VkDevice device) {
  vkDestroyRenderPass(device, *pRenderPass, NULL);
  *pRenderPass = VK_NULL_HANDLE;
}

ErrVal new_VertexDisplayPipelineLayout(VkPipelineLayout *pPipelineLayout,
                                       const VkDevice device) {
  VkPushConstantRange pushConstantRange = {0};
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(mat4x4);
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
  VkResult res = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL,
                                        pPipelineLayout);
  if (res != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL,
                   "failed to create pipeline layout with error: %s",
                   vkstrerror(res));
    PANIC();
  }
  return (ERR_OK);
}

void delete_PipelineLayout(VkPipelineLayout *pPipelineLayout,
                           const VkDevice device) {
  vkDestroyPipelineLayout(device, *pPipelineLayout, NULL);
  *pPipelineLayout = VK_NULL_HANDLE;
}

ErrVal new_VertexDisplayPipeline(VkPipeline *pGraphicsPipeline,
                                 const VkDevice device,
                                 const VkShaderModule vertShaderModule,
                                 const VkShaderModule fragShaderModule,
                                 const VkExtent2D extent,
                                 const VkRenderPass renderPass,
                                 const VkPipelineLayout pipelineLayout) {
  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {0};
  vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {0};
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[2] = {vertShaderStageInfo,
                                                     fragShaderStageInfo};

  VkVertexInputBindingDescription bindingDescription = {0};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(Vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription attributeDescriptions[2];

  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[0].offset = offsetof(Vertex, position);

  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[1].offset = offsetof(Vertex, color);

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = 2;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {0};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)extent.width;
  viewport.height = (float)extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {0};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent = extent;

  VkPipelineDepthStencilStateCreateInfo depthStencil = {0};
  depthStencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewportState = {0};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer = {0};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_NONE; /* VK_CULL_MODE_BACK_BIT; */
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling = {0};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending = {0};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  VkGraphicsPipelineCreateInfo pipelineInfo = {0};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL,
                                pGraphicsPipeline) != VK_SUCCESS) {
    LOG_ERROR(ERR_LEVEL_FATAL, "failed to create graphics pipeline!");
    PANIC();
  }
  return (ERR_OK);
}

void delete_Pipeline(VkPipeline *pPipeline, const VkDevice device) {
  vkDestroyPipeline(device, *pPipeline, NULL);
}

ErrVal new_Framebuffer(VkFramebuffer *pFramebuffer, const VkDevice device,
                       const VkRenderPass renderPass,
                       const VkImageView imageView,
                       const VkImageView depthImageView,
                       const VkExtent2D swapChainExtent) {
  VkFramebufferCreateInfo framebufferInfo = {0};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass = renderPass;
  framebufferInfo.attachmentCount = 2;
  framebufferInfo.pAttachments = (VkImageView[]){imageView, depthImageView};
  framebufferInfo.width = swapChainExtent.width;
  framebufferInfo.height = swapChainExtent.height;
  framebufferInfo.layers = 1;
  VkResult res =
      vkCreateFramebuffer(device, &framebufferInfo, NULL, pFramebuffer);
  if (res == VK_SUCCESS) {
    return (ERR_OK);
  } else {
    LOG_ERROR_ARGS(ERR_LEVEL_WARN, "failed to create framebuffers: %s",
                   vkstrerror(res));
    return (ERR_UNKNOWN);
  }
}

void delete_Framebuffer(VkFramebuffer *pFramebuffer, VkDevice device) {
  vkDestroyFramebuffer(device, *pFramebuffer, NULL);
  *pFramebuffer = VK_NULL_HANDLE;
}

ErrVal new_SwapChainFramebuffers(VkFramebuffer **ppFramebuffers,
                                 const VkDevice device,
                                 const VkRenderPass renderPass,
                                 const VkExtent2D swapChainExtent,
                                 const uint32_t imageCount,
                                 const VkImageView depthImageView,
                                 const VkImageView *pSwapChainImageViews) {
  *ppFramebuffers = (VkFramebuffer *)malloc(imageCount * sizeof(VkFramebuffer));
  if (!(*ppFramebuffers)) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL, "could not create framebuffers: %s",
                   strerror(errno));
    PANIC();
  }

  for (uint32_t i = 0; i < imageCount; i++) {
    ErrVal retVal = new_Framebuffer(&((*ppFramebuffers)[i]), device, renderPass,
                                    pSwapChainImageViews[i], depthImageView,
                                    swapChainExtent);
    if (retVal != ERR_OK) {
      LOG_ERROR(ERR_LEVEL_ERROR, "could not create framebuffers");
      delete_SwapChainFramebuffers(ppFramebuffers, i, device);
      return (retVal);
    }
  }
  return (ERR_OK);
}

void delete_SwapChainFramebuffers(VkFramebuffer **ppFramebuffers,
                                  const uint32_t imageCount,
                                  const VkDevice device) {
  for (uint32_t i = 0; i < imageCount; i++) {
    delete_Framebuffer(&((*ppFramebuffers)[i]), device);
  }
  free(*ppFramebuffers);
  *ppFramebuffers = NULL;
}

ErrVal new_CommandPool(VkCommandPool *pCommandPool, const VkDevice device,
                       const uint32_t queueFamilyIndex) {
  VkCommandPoolCreateInfo poolInfo = {0};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = queueFamilyIndex;
  poolInfo.flags = 0;
  VkResult ret = vkCreateCommandPool(device, &poolInfo, NULL, pCommandPool);
  if (ret != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_ERROR, "failed to create command pool %s",
                   vkstrerror(ret));
    return (ERR_UNKNOWN);
  }
  return (ERR_OK);
}

void delete_CommandPool(VkCommandPool *pCommandPool, const VkDevice device) {
  vkDestroyCommandPool(device, *pCommandPool, NULL);
}

ErrVal new_VertexDisplayCommandBuffers(
    VkCommandBuffer **ppCommandBuffers, const VkBuffer vertexBuffer,
    const uint32_t vertexCount, const VkDevice device,
    const VkRenderPass renderPass,
    const VkPipelineLayout vertexDisplayPipelineLayout,
    const VkPipeline vertexDisplayPipeline, const VkCommandPool commandPool,
    const VkExtent2D swapChainExtent, const uint32_t swapChainFramebufferCount,
    const VkFramebuffer *pSwapChainFramebuffers, const mat4x4 cameraTransform) {
  VkCommandBufferAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = swapChainFramebufferCount;

  VkCommandBuffer *pCommandBuffers = (VkCommandBuffer *)malloc(
      swapChainFramebufferCount * sizeof(VkCommandBuffer));
  if (!pCommandBuffers) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL,
                   "Failed to create graphics command buffers: %s",
                   strerror(errno));
    PANIC();
  }

  VkResult allocateCommandBuffersRetVal =
      vkAllocateCommandBuffers(device, &allocInfo, pCommandBuffers);
  if (allocateCommandBuffersRetVal != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL,
                   "Failed to create graphics command buffers, error code: %s",
                   vkstrerror(allocateCommandBuffersRetVal));
    PANIC();
  }

  for (size_t i = 0; i < swapChainFramebufferCount; i++) {
    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    if (vkBeginCommandBuffer(pCommandBuffers[i], &beginInfo) != VK_SUCCESS) {
      LOG_ERROR(ERR_LEVEL_FATAL,
                "Failed to record into graphics command buffer");
      PANIC();
    }

    VkRenderPassBeginInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = pSwapChainFramebuffers[i];
    renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    VkClearValue pClearColors[2];
    pClearColors[0].color.float32[0] = 0.0f;
    pClearColors[0].color.float32[1] = 0.0f;
    pClearColors[0].color.float32[2] = 0.0f;
    pClearColors[0].color.float32[3] = 0.0f;

    pClearColors[1].depthStencil.depth = 1.0f;
    pClearColors[1].depthStencil.stencil = 0;

    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = pClearColors;

    vkCmdBeginRenderPass(pCommandBuffers[i], &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(pCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                      vertexDisplayPipeline);

    vkCmdPushConstants(pCommandBuffers[i], vertexDisplayPipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4x4),
                       cameraTransform);
    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(pCommandBuffers[i], 0, 1, vertexBuffers, offsets);

    vkCmdDraw(pCommandBuffers[i], vertexCount, 1, 0, 0);
    vkCmdEndRenderPass(pCommandBuffers[i]);

    VkResult endCommandBufferRetVal = vkEndCommandBuffer(pCommandBuffers[i]);
    if (endCommandBufferRetVal != VK_SUCCESS) {
      LOG_ERROR_ARGS(ERR_LEVEL_FATAL,
                     "Failed to record command buffer, error code: %s",
                     vkstrerror(endCommandBufferRetVal));
      PANIC();
    }
  }

  *ppCommandBuffers = pCommandBuffers;
  return (ERR_OK);
}

void delete_CommandBuffers(VkCommandBuffer **ppCommandBuffers,
                           const uint32_t commandBufferCount,
                           const VkCommandPool commandPool,
                           const VkDevice device) {
  vkFreeCommandBuffers(device, commandPool, commandBufferCount,
                       *ppCommandBuffers);
  free(*ppCommandBuffers);
  *ppCommandBuffers = NULL;
}

ErrVal new_Semaphore(VkSemaphore *pSemaphore, const VkDevice device) {
  VkSemaphoreCreateInfo semaphoreInfo = {0};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VkResult ret = vkCreateSemaphore(device, &semaphoreInfo, NULL, pSemaphore);
  if (ret != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_ERROR, "failed to create semaphore: %s",
                   vkstrerror(ret));
    return (ERR_UNKNOWN);
  }
  return (ERR_OK);
}

void delete_Semaphore(VkSemaphore *pSemaphore, const VkDevice device) {
  vkDestroySemaphore(device, *pSemaphore, NULL);
  *pSemaphore = NULL;
}

ErrVal new_Semaphores(VkSemaphore **ppSemaphores, const uint32_t semaphoreCount,
                      const VkDevice device) {
  if (semaphoreCount == 0) {
    LOG_ERROR(
        ERR_LEVEL_WARN,
        "failed to create semaphores: could not allocate 0 bytes of memory");
    return (ERR_BADARGS);
  }
  *ppSemaphores = (VkSemaphore *)malloc(semaphoreCount * sizeof(VkSemaphore));
  if (*ppSemaphores == NULL) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL, "Failed to create semaphores: %s",
                   strerror(errno));
    PANIC();
  }

  for (uint32_t i = 0; i < semaphoreCount; i++) {
    ErrVal retVal = new_Semaphore(&(*ppSemaphores)[i], device);
    if (retVal != ERR_OK) {
      delete_Semaphores(ppSemaphores, i, device);
      return (retVal);
    }
  }
  return (ERR_OK);
}

void delete_Semaphores(VkSemaphore **ppSemaphores,
                       const uint32_t semaphoreCount, const VkDevice device) {
  for (uint32_t i = 0; i < semaphoreCount; i++) {
    delete_Semaphore(&((*ppSemaphores)[i]), device);
  }
  free(*ppSemaphores);
  *ppSemaphores = NULL;
}

ErrVal new_Fence(VkFence *pFence, const VkDevice device) {
  VkFenceCreateInfo fenceInfo = {0};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  VkResult ret = vkCreateFence(device, &fenceInfo, NULL, pFence);
  if (ret != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_ERROR, "failed to create fence: %s",
                   vkstrerror(ret));
    return (ERR_UNKNOWN);
  }
  return (ERR_OK);
}

void delete_Fence(VkFence *pFence, const VkDevice device) {
  vkDestroyFence(device, *pFence, NULL);
  *pFence = VK_NULL_HANDLE;
}

ErrVal new_Fences(VkFence **ppFences, const uint32_t fenceCount,
                  const VkDevice device) {
  if (fenceCount == 0) {
    LOG_ERROR(ERR_LEVEL_WARN, "cannot allocate 0 bytes of memory");
    return (ERR_UNSAFE);
  }
  *ppFences = (VkFence *)malloc(fenceCount * sizeof(VkDevice));
  if (!*ppFences) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL, "failed to create memory fence; %s",
                   strerror(errno));
    PANIC();
  }

  for (uint32_t i = 0; i < fenceCount; i++) {
    ErrVal retVal = new_Fence(&(*ppFences)[i], device);
    if (retVal != ERR_OK) {
      /* Clean up memory */
      delete_Fences(ppFences, i, device);
      return (retVal);
    }
  }
  return (ERR_OK);
}

void delete_Fences(VkFence **ppFences, const uint32_t fenceCount,
                   const VkDevice device) {
  for (uint32_t i = 0; i < fenceCount; i++) {
    delete_Fence(&(*ppFences)[i], device);
  }
  free(*ppFences);
  *ppFences = NULL;
}

/* Draws a frame to the surface provided, and sets things up for the next frame
 */
uint32_t drawFrame(uint32_t *pCurrentFrame, const uint32_t maxFramesInFlight,
                   const VkDevice device, const VkSwapchainKHR swapChain,
                   const VkCommandBuffer *pCommandBuffers,
                   const VkFence *pInFlightFences,
                   const VkSemaphore *pImageAvailableSemaphores,
                   const VkSemaphore *pRenderFinishedSemaphores,
                   const VkQueue graphicsQueue, const VkQueue presentQueue) {
  /* Waits for the the current frame to finish processing */
  VkResult fenceWait = vkWaitForFences(
      device, 1, &pInFlightFences[*pCurrentFrame], VK_TRUE, UINT64_MAX);
  if (fenceWait != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL,
                   "failed to wait for fence while drawing frame: %s",
                   vkstrerror(fenceWait));
    PANIC();
  }
  VkResult fenceResetResult =
      vkResetFences(device, 1, &pInFlightFences[*pCurrentFrame]);
  if (fenceResetResult != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL,
                   "failed to reset fence while drawing frame: %s",
                   vkstrerror(fenceResetResult));
  }
  /* Gets the next image from the swap Chain */
  uint32_t imageIndex;
  VkResult nextImageResult = vkAcquireNextImageKHR(
      device, swapChain, UINT64_MAX, pImageAvailableSemaphores[*pCurrentFrame],
      VK_NULL_HANDLE, &imageIndex);
  /* If the window has been resized, the result will be an out of date error,
   * meaning that the swap chain must be resized */
  if (nextImageResult == VK_ERROR_OUT_OF_DATE_KHR) {
    return (ERR_OUTOFDATE);
  } else if (nextImageResult != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL, "failed to get next frame: %s",
                   vkstrerror(nextImageResult));
    PANIC();
  }

  VkSubmitInfo submitInfo = {0};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  /* Sets up for next frame */
  VkSemaphore waitSemaphores[] = {pImageAvailableSemaphores[*pCurrentFrame]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  /*TODO please push constants here, rather than re record each time*/
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &pCommandBuffers[imageIndex];

  VkSemaphore signalSemaphores[] = {pRenderFinishedSemaphores[*pCurrentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  VkResult queueSubmitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo,
                                             pInFlightFences[*pCurrentFrame]);
  if (queueSubmitResult != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL, "failed to submit queue: %s",
                   vkstrerror(queueSubmitResult));
    PANIC();
  }

  /* Present frame to screen */
  VkPresentInfoKHR presentInfo = {0};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;

  presentInfo.pImageIndices = &imageIndex;

  vkQueuePresentKHR(presentQueue, &presentInfo);
  /* Increments the current frame by one */ /* TODO come up with more graceful
                                               solution */
  *pCurrentFrame = (*pCurrentFrame + 1) % maxFramesInFlight;
  return (ERR_OK);
}

/* Deletes a VkSurfaceKHR */
void delete_Surface(VkSurfaceKHR *pSurface, const VkInstance instance) {
  vkDestroySurfaceKHR(instance, *pSurface, NULL);
  *pSurface = VK_NULL_HANDLE;
}

/* Gets the extent of the given GLFW window */
ErrVal getWindowExtent(VkExtent2D *pExtent, GLFWwindow *pWindow) {
  int width;
  int height;
  glfwGetFramebufferSize(pWindow, &width, &height);
  pExtent->width = (uint32_t)width;
  pExtent->height = (uint32_t)height;
  return (ERR_OK);
}

/* Creates a new window using the GLFW library. */
ErrVal new_GLFWwindow(GLFWwindow **ppGLFWwindow) {
  /* Not resizable */
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  *ppGLFWwindow =
      glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, APPNAME, NULL, NULL);
  if (*ppGLFWwindow == NULL) {
    LOG_ERROR(ERR_LEVEL_ERROR, "failed to create GLFW window");
    return (ERR_UNKNOWN);
  }
  return (ERR_OK);
}

/* Creates a new window surface using the glfw libraries. This must be deleted
 * with the delete_Surface function*/
ErrVal new_SurfaceFromGLFW(VkSurfaceKHR *pSurface, GLFWwindow *pWindow,
                           const VkInstance instance) {
  VkResult res = glfwCreateWindowSurface(instance, pWindow, NULL, pSurface);
  if (res != VK_SUCCESS) {
    LOG_ERROR(ERR_LEVEL_FATAL, "failed to create surface, quitting");
    PANIC();
  }
  return (ERR_OK);
}

/* returns any errors encountered. Finds the index of the correct type of memory
 * to allocate for a given physical device using the bits and flags that are
 * requested. */
ErrVal getMemoryTypeIndex(uint32_t *memoryTypeIndex,
                          const uint32_t memoryTypeBits,
                          const VkMemoryPropertyFlags memoryPropertyFlags,
                          const VkPhysicalDevice physicalDevice) {

  /* Retrieve memory properties */
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
  /* Check each memory type to see if it conforms to our requirements */
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((memoryTypeBits &
         (1 << i)) && /* TODO figure out what's going on over here */
        (memProperties.memoryTypes[i].propertyFlags & memoryPropertyFlags) ==
            memoryPropertyFlags) {
      *memoryTypeIndex = i;
      return (ERR_OK);
    }
  }
  LOG_ERROR(ERR_LEVEL_ERROR, "failed to find suitable memory type");
  return (ERR_MEMORY);
}

ErrVal new_VertexBuffer(VkBuffer *pBuffer, VkDeviceMemory *pBufferMemory,
                        const Vertex *pVertices, const uint32_t vertexCount,
                        const VkDevice device,
                        const VkPhysicalDevice physicalDevice,
                        const VkCommandPool commandPool, const VkQueue queue) {
  /* Construct staging buffers */
  VkDeviceSize bufferSize = sizeof(Vertex) * vertexCount;
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  ErrVal stagingBufferCreateResult = new_Buffer_DeviceMemory(
      &stagingBuffer, &stagingBufferMemory, bufferSize, physicalDevice, device,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  if (stagingBufferCreateResult != ERR_OK) {
    LOG_ERROR(
        ERR_LEVEL_ERROR,
        "failed to create vertex buffer: failed to create staging buffer");
    return (stagingBufferCreateResult);
  }

  /* Copy data to staging buffer, making sure to clean up leaks */
  ErrVal copyResult =
      copyToDeviceMemory(&stagingBufferMemory, bufferSize, pVertices, device);
  if (copyResult != ERR_OK) {
    LOG_ERROR(ERR_LEVEL_ERROR,
              "failed to create vertex buffer: could not map memory");
    delete_Buffer(&stagingBuffer, device);
    delete_DeviceMemory(&stagingBufferMemory, device);
    return (copyResult);
  }

  /* Create vertex buffer and allocate memory for it */
  ErrVal vertexBufferCreateResult = new_Buffer_DeviceMemory(
      pBuffer, pBufferMemory, bufferSize, physicalDevice, device,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  /* Handle errors */
  if (vertexBufferCreateResult != ERR_OK) {
    /* Delete the temporary staging buffers */
    LOG_ERROR(ERR_LEVEL_ERROR, "failed to create vertex buffer");
    delete_Buffer(&stagingBuffer, device);
    delete_DeviceMemory(&stagingBufferMemory, device);
    return (vertexBufferCreateResult);
  }

  /* Copy the data over from the staging buffer to the vertex buffer */
  copyBuffer(*pBuffer, stagingBuffer, bufferSize, commandPool, queue, device);

  /* Delete the temporary staging buffers */
  delete_Buffer(&stagingBuffer, device);
  delete_DeviceMemory(&stagingBufferMemory, device);

  return (ERR_OK);
}

ErrVal new_Buffer_DeviceMemory(VkBuffer *pBuffer, VkDeviceMemory *pBufferMemory,
                               const VkDeviceSize size,
                               const VkPhysicalDevice physicalDevice,
                               const VkDevice device,
                               const VkBufferUsageFlags usage,
                               const VkMemoryPropertyFlags properties) {
  VkBufferCreateInfo bufferInfo = {0};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  /* Create buffer */
  VkResult bufferCreateResult =
      vkCreateBuffer(device, &bufferInfo, NULL, pBuffer);
  if (bufferCreateResult != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_ERROR, "failed to create buffer: %s",
                   vkstrerror(bufferCreateResult));
    return (ERR_UNKNOWN);
  }
  /* Allocate memory for buffer */
  VkMemoryRequirements memoryRequirements;
  vkGetBufferMemoryRequirements(device, *pBuffer, &memoryRequirements);

  VkMemoryAllocateInfo allocateInfo = {0};
  allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.allocationSize = memoryRequirements.size;
  /* Get the type of memory required, handle errors */
  ErrVal getMemoryTypeRetVal = getMemoryTypeIndex(
      &allocateInfo.memoryTypeIndex, memoryRequirements.memoryTypeBits,
      properties, physicalDevice);
  if (getMemoryTypeRetVal != ERR_OK) {
    LOG_ERROR(ERR_LEVEL_ERROR, "failed to get type of memory to allocate");
    return (ERR_MEMORY);
  }

  /* Actually allocate memory */
  VkResult memoryAllocateResult =
      vkAllocateMemory(device, &allocateInfo, NULL, pBufferMemory);
  if (memoryAllocateResult != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_ERROR, "failed to allocate memory for buffer: %s",
                   vkstrerror(memoryAllocateResult));
    return (ERR_ALLOCFAIL);
  }
  vkBindBufferMemory(device, *pBuffer, *pBufferMemory, 0);
  return (ERR_OK);
}

ErrVal copyBuffer(VkBuffer destinationBuffer, const VkBuffer sourceBuffer,
                  const VkDeviceSize size, const VkCommandPool commandPool,
                  const VkQueue queue, const VkDevice device) {
  VkCommandBuffer copyCommandBuffer;
  ErrVal beginResult = new_begin_OneTimeSubmitCommandBuffer(
      &copyCommandBuffer, device, commandPool);

  if (beginResult != ERR_OK) {
    LOG_ERROR(ERR_LEVEL_ERROR, "failed to begin command buffer");
    return (beginResult);
  }

  VkBufferCopy copyRegion = {0};
  copyRegion.size = size;
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  vkCmdCopyBuffer(copyCommandBuffer, sourceBuffer, destinationBuffer, 1,
                  &copyRegion);

  ErrVal endResult = delete_end_OneTimeSubmitCommandBuffer(
      &copyCommandBuffer, device, queue, commandPool);

  if (endResult != ERR_OK) {
    LOG_ERROR(ERR_LEVEL_ERROR, "failed to end command buffer");
    return (endResult);
  }

  return (ERR_OK);
}

void delete_Buffer(VkBuffer *pBuffer, const VkDevice device) {
  vkDestroyBuffer(device, *pBuffer, NULL);
  *pBuffer = VK_NULL_HANDLE;
}

void delete_DeviceMemory(VkDeviceMemory *pDeviceMemory, const VkDevice device) {
  vkFreeMemory(device, *pDeviceMemory, NULL);
  *pDeviceMemory = VK_NULL_HANDLE;
}

/*
 * Allocates, creates and begins one command buffer to be used.
 * Must be ended with delete_end_OneTimeSubmitCommandBuffer
 */
ErrVal new_begin_OneTimeSubmitCommandBuffer(VkCommandBuffer *pCommandBuffer,
                                            const VkDevice device,
                                            const VkCommandPool commandPool) {
  VkCommandBufferAllocateInfo allocateInfo = {0};
  allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocateInfo.commandPool = commandPool;
  allocateInfo.commandBufferCount = 1;

  VkResult allocateResult =
      vkAllocateCommandBuffers(device, &allocateInfo, pCommandBuffer);
  if (allocateResult != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_ERROR, "failed to allocate command buffers: %s",
                   vkstrerror(allocateResult));
    return (ERR_MEMORY);
  }

  VkCommandBufferBeginInfo beginInfo = {0};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  VkResult res = vkBeginCommandBuffer(*pCommandBuffer, &beginInfo);
  if (res != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_ERROR, "failed to create command buffer: %s",
                   vkstrerror(res));
    return (ERR_UNKNOWN);
  }

  return (ERR_OK);
}

/*
 * Ends, submits, and deletes one command buffer that was previously created in
 * new_begin_OneTimeSubmitCommandBuffer
 */
ErrVal delete_end_OneTimeSubmitCommandBuffer(VkCommandBuffer *pCommandBuffer,
                                             const VkDevice device,
                                             const VkQueue queue,
                                             const VkCommandPool commandPool) {
  ErrVal retVal = ERR_OK;
  VkResult bufferEndResult = vkEndCommandBuffer(*pCommandBuffer);
  if (bufferEndResult != VK_SUCCESS) {
    /* Clean up resources */
    LOG_ERROR_ARGS(ERR_LEVEL_ERROR,
                   "failed to end one time submit command buffer: %s",
                   vkstrerror(bufferEndResult));
    retVal = ERR_UNKNOWN;
    goto FREEALL;
  }

  VkSubmitInfo submitInfo = {0};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = pCommandBuffer;

  VkResult queueSubmitResult =
      vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  if (queueSubmitResult != VK_SUCCESS) {
    LOG_ERROR_ARGS(
        ERR_LEVEL_ERROR,
        "failed to submit one time submit command buffer to queue: %s",
        vkstrerror(queueSubmitResult));
    retVal = ERR_UNKNOWN;
    goto FREEALL;
  }
/* Deallocate the buffer */
FREEALL:
  vkQueueWaitIdle(queue);
  vkFreeCommandBuffers(device, commandPool, 1, pCommandBuffer);
  *pCommandBuffer = VK_NULL_HANDLE;
  return (retVal);
}

ErrVal copyToDeviceMemory(VkDeviceMemory *pDeviceMemory,
                          const VkDeviceSize deviceSize, const void *source,
                          const VkDevice device) {
  void *data;
  VkResult mapMemoryResult =
      vkMapMemory(device, *pDeviceMemory, 0, deviceSize, 0, &data);

  /* On failure */
  if (mapMemoryResult != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_ERROR,
                   "failed to copy to device memory: failed to map memory: %s",
                   vkstrerror(mapMemoryResult));
    return (ERR_MEMORY);
  }

  /* If it was successful, go on and actually copy it, making sure to unmap once
   * done */
  memcpy(data, source, (size_t)deviceSize);
  vkUnmapMemory(device, *pDeviceMemory);
  return (ERR_OK);
}

ErrVal new_Image(VkImage *pImage, VkDeviceMemory *pImageMemory,
                 const uint32_t width, const uint32_t height,
                 const VkFormat format, const VkImageTiling tiling,
                 const VkImageUsageFlags usage,
                 const VkMemoryPropertyFlags properties,
                 const VkPhysicalDevice physicalDevice, const VkDevice device) {
  VkImageCreateInfo imageInfo = {0};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkResult createImageResult = vkCreateImage(device, &imageInfo, NULL, pImage);
  if (createImageResult != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_ERROR, "failed to create image: %s",
                   vkstrerror(createImageResult));
    return (ERR_UNKNOWN);
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, *pImage, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;

  ErrVal memGetResult = getMemoryTypeIndex(&allocInfo.memoryTypeIndex,
                                           memRequirements.memoryTypeBits,
                                           properties, physicalDevice);

  if (memGetResult != ERR_OK) {
    LOG_ERROR(ERR_LEVEL_ERROR, "failed to create image: allocation failed");
    return (ERR_MEMORY);
  }

  VkResult allocateResult =
      vkAllocateMemory(device, &allocInfo, NULL, pImageMemory);
  if (allocateResult != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_ERROR, "failed to create image: %s",
                   vkstrerror(allocateResult));
    return (ERR_MEMORY);
  }

  VkResult bindResult = vkBindImageMemory(device, *pImage, *pImageMemory, 0);
  if (bindResult != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_ERROR, "failed to create image: %s",
                   vkstrerror(bindResult));
    return (ERR_UNKNOWN);
  }
  return (ERR_OK);
}

void delete_Image(VkImage *pImage, const VkDevice device) {
  vkDestroyImage(device, *pImage, NULL);
}

/* Gets image format of depth */
void getDepthFormat(VkFormat *pFormat) {
  /* TODO we might want to redo this so that there are more compatible images */
  *pFormat = VK_FORMAT_D32_SFLOAT;
}

ErrVal new_DepthImage(VkImage *pImage, VkDeviceMemory *pImageMemory,
                      const VkExtent2D swapChainExtent,
                      const VkPhysicalDevice physicalDevice,
                      const VkDevice device, const VkCommandPool commandPool,
                      const VkQueue queue) {
  VkFormat depthFormat = {0};
  getDepthFormat(&depthFormat);
  ErrVal retVal =
      new_Image(pImage, pImageMemory, swapChainExtent.width,
                swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice, device);
  if (retVal != ERR_OK) {
    LOG_ERROR(ERR_LEVEL_ERROR, "failed to create depth image");
    return (retVal);
  }

  ErrVal transitionVal =
      transitionImageLayout(device, commandPool, *pImage, depthFormat, queue,
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
  if (transitionVal != ERR_OK) {
    LOG_ERROR(ERR_LEVEL_ERROR,
              "failed to create depth image: could not set barriers");
    return (transitionVal);
  }

  return (ERR_OK);
}

ErrVal new_DepthImageView(VkImageView *pImageView, const VkDevice device,
                          const VkImage depthImage) {
  VkFormat depthFormat;
  getDepthFormat(&depthFormat);
  ErrVal retVal = new_ImageView(pImageView, device, depthImage, depthFormat,
                                VK_IMAGE_ASPECT_DEPTH_BIT);
  if (retVal != ERR_OK) {
    LOG_ERROR(ERR_LEVEL_ERROR, "failed to create depth image view");
  }
  return (retVal);
}

/* Transitions the image layout using a barrier mask */
ErrVal transitionImageLayout(const VkDevice device,
                             const VkCommandPool commandPool,
                             const VkImage image, const VkFormat format,
                             const VkQueue queue, const VkImageLayout oldLayout,
                             const VkImageLayout newLayout) {
  VkCommandBuffer commandBuffer;
  ErrVal beginRetVal =
      new_begin_OneTimeSubmitCommandBuffer(&commandBuffer, device, commandPool);
  if (beginRetVal != ERR_OK) {
    LOG_ERROR(
        ERR_LEVEL_ERROR,
        "failed to transition image layout: failed to begin command buffer");
    return (beginRetVal);
  }

  VkImageMemoryBarrier barrier = {0};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;

  if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
        format == VK_FORMAT_D24_UNORM_S8_UINT) {
      barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  } else {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } else {
    LOG_ERROR(ERR_LEVEL_ERROR, "unsupported layout transition");
    return (ERR_BADARGS);
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, NULL,
                       0, NULL, 1, &barrier);

  ErrVal endRetVal = delete_end_OneTimeSubmitCommandBuffer(
      &commandBuffer, device, queue, commandPool);
  if (endRetVal != ERR_OK) {
    LOG_ERROR(
        ERR_LEVEL_ERROR,
        "failed to transition image layout: failed to submit command buffer");
    return (endRetVal);
  }
  return (ERR_OK);
}

ErrVal new_NodeUpdateComputePipelineLayout(
    VkPipelineLayout *pPipelineLayout,
    const VkDescriptorSetLayout nodeComputeBufferLayout,
    const VkDevice device) {
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = NULL;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &nodeComputeBufferLayout;
  VkResult res = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL,
                                        pPipelineLayout);
  if (res != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL,
                   "failed to create pipeline layout with error: %s",
                   vkstrerror(res));
    PANIC();
  }

  return (ERR_OK);
}

ErrVal new_NodeTopologyComputePipelineLayout(VkPipelineLayout *pPipelineLayout,
                                             const VkDevice device) {
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = NULL;
  VkResult res = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL,
                                        pPipelineLayout);
  if (res != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL,
                   "failed to create pipeline layout with error: %s",
                   vkstrerror(res));
    PANIC();
  }

  return (ERR_OK);
}

ErrVal
new_VertexGenerationComputePipelineLayout(VkPipelineLayout *pPipelineLayout,
                                          const VkDevice device) {
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = NULL;
  VkResult res = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL,
                                        pPipelineLayout);
  if (res != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_FATAL,
                   "failed to create pipeline layout with error: %s",
                   vkstrerror(res));
    PANIC();
  }

  return (ERR_OK);
}

ErrVal new_ComputePipeline(VkPipeline *pPipeline,
                           const VkPipelineLayout pipelineLayout,
                           const VkShaderModule shaderModule,
                           const VkDevice device) {

  VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {0};
  shaderStageCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageCreateInfo.module = shaderModule;
  shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  shaderStageCreateInfo.pName = "main";

  VkComputePipelineCreateInfo computePipelineCreateInfo = {0};
  computePipelineCreateInfo.sType =
      VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineCreateInfo.layout = pipelineLayout;
  computePipelineCreateInfo.stage = shaderStageCreateInfo;

  VkResult ret = vkCreateComputePipelines(
      device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, NULL, pPipeline);
  if (ret != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_ERROR, "failed to create compute pipelines %s",
                   vkstrerror(ret));
    return (ERR_UNKNOWN);
  }
  return (ERR_OK);
}

ErrVal new_ComputeStorageDescriptorSetLayout(
    VkDescriptorSetLayout *pDescriptorSetLayout, const VkDevice device) {
  VkDescriptorSetLayoutBinding storageLayoutBinding = {0};
  storageLayoutBinding.binding = 0;
  storageLayoutBinding.descriptorCount = 1;
  storageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  storageLayoutBinding.pImmutableSamplers = NULL;
  storageLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &storageLayoutBinding;
  VkResult retVal = vkCreateDescriptorSetLayout(device, &layoutInfo, NULL,
                                                pDescriptorSetLayout);
  if (retVal != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_ERROR,
                   "failed to create descriptor set layout: %s",
                   vkstrerror(retVal));
    return (ERR_UNKNOWN);
  }
  return (ERR_OK);
}

void delete_DescriptorSetLayout(VkDescriptorSetLayout *pDescriptorSetLayout,
                                const VkDevice device) {
  vkDestroyDescriptorSetLayout(device, *pDescriptorSetLayout, NULL);
  *pDescriptorSetLayout = VK_NULL_HANDLE;
}

ErrVal new_DescriptorPool(VkDescriptorPool *pDescriptorPool,
                          const VkDescriptorType descriptorType,
                          const uint32_t maxAllocFrom, const VkDevice device) {
  VkDescriptorPoolSize descriptorPoolSize;
  descriptorPoolSize.type = descriptorType;
  descriptorPoolSize.descriptorCount = maxAllocFrom;

  VkDescriptorPoolCreateInfo poolInfo = {0};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &descriptorPoolSize;
  poolInfo.maxSets = maxAllocFrom;

  /* Actually create descriptor pool */
  VkResult ret =
      vkCreateDescriptorPool(device, &poolInfo, NULL, pDescriptorPool);

  if (ret != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_ERROR, "failed to create descriptor pool; %s",
                   vkstrerror(ret));
    return (ERR_UNKNOWN);
  } else {
    return (ERR_OK);
  }
}

void delete_DescriptorPool(VkDescriptorPool *pDescriptorPool,
                           const VkDevice device) {
  vkDestroyDescriptorPool(device, *pDescriptorPool, NULL);
  *pDescriptorPool = VK_NULL_HANDLE;
}

ErrVal new_ComputeBufferDescriptorSet(
    VkDescriptorSet *pDescriptorSet, const VkBuffer computeBufferDescriptorSet,
    const VkDeviceSize computeBufferSize,
    const VkDescriptorSetLayout descriptorSetLayout,
    const VkDescriptorPool descriptorPool, const VkDevice device) {

  VkDescriptorSetAllocateInfo allocateInfo = {0};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = descriptorPool;
  allocateInfo.descriptorSetCount = 1;
  allocateInfo.pSetLayouts = &descriptorSetLayout;
  VkResult allocateDescriptorSetRetVal =
      vkAllocateDescriptorSets(device, &allocateInfo, pDescriptorSet);
  if (allocateDescriptorSetRetVal != VK_SUCCESS) {
    LOG_ERROR_ARGS(ERR_LEVEL_ERROR, "failed to allocate descriptor sets: %s",
                   vkstrerror(allocateDescriptorSetRetVal));
    return (ERR_MEMORY);
  }

  VkDescriptorBufferInfo bufferInfo = {0};
  bufferInfo.buffer = computeBufferDescriptorSet;
  bufferInfo.range = computeBufferSize;
  bufferInfo.offset = 0;

  VkWriteDescriptorSet descriptorWrites = {0};
  descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites.dstSet = *pDescriptorSet;
  descriptorWrites.dstBinding = 0;
  descriptorWrites.dstArrayElement = 0;
  descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  descriptorWrites.descriptorCount = 1;
  descriptorWrites.pBufferInfo = &bufferInfo;
  descriptorWrites.pImageInfo = NULL;
  descriptorWrites.pTexelBufferView = NULL;
  vkUpdateDescriptorSets(device, 1, &descriptorWrites, 0, NULL);
  return (ERR_OK);
}

void delete_DescriptorSets(VkDescriptorSet **ppDescriptorSets) {
  free(*ppDescriptorSets);
  *ppDescriptorSets = NULL;
}
