#include "startup.h"

/* This file contains the vulkan function pointers that will be loaded at startup/runtime. */

//-------------------- vars
extern PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;

//GLOBAL_LEVEL_VULKAN_FUNCTION
extern PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
extern PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
extern PFN_vkCreateInstance vkCreateInstance;
//

//DEBUG_VULKAN_FUNCTION
extern PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
extern PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
extern PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;
//

//INSTANCE_LEVEL_VULKAN_FUNCTIONS
extern PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
extern PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
extern PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
extern PFN_vkCreateDevice vkCreateDevice;
extern PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
extern PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
extern PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
extern PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
extern PFN_vkDestroyInstance vkDestroyInstance;
extern PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties;
//extern PFN_

//EXTENSION FUNCTIONS
extern PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
extern PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
extern PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
extern PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
extern PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
#ifdef VK_USE_PLATFORM_WIN32_KHR
extern PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
#elif defined VK_USE_PLATFORM_XCB_KHR
extern PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR;
#elif defined VK_USE_PLATFORM_XLIB_KHR
extern PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR;
#endif
//

//DEVICE_LEVEL_VULKAN_FUNCTION

extern PFN_vkGetDeviceQueue vkGetDeviceQueue;
extern PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
extern PFN_vkDestroyDevice vkDestroyDevice;
extern PFN_vkCreateBuffer vkCreateBuffer;
extern PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
extern PFN_vkAllocateMemory vkAllocateMemory;
extern PFN_vkBindBufferMemory vkBindBufferMemory;
extern PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
extern PFN_vkCreateImage vkCreateImage;
extern PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
extern PFN_vkBindImageMemory vkBindImageMemory;
extern PFN_vkCreateImageView vkCreateImageView;
extern PFN_vkMapMemory vkMapMemory;
extern PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges;
extern PFN_vkUnmapMemory vkUnmapMemory;
extern PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
extern PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage;
extern PFN_vkCmdCopyImageToBuffer vkCmdCopyImageToBuffer;
extern PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
extern PFN_vkEndCommandBuffer vkEndCommandBuffer;
extern PFN_vkQueueSubmit vkQueueSubmit;
extern PFN_vkDestroyImageView vkDestroyImageView;
extern PFN_vkDestroyImage vkDestroyImage;
extern PFN_vkDestroyBuffer vkDestroyBuffer;
extern PFN_vkFreeMemory vkFreeMemory;
extern PFN_vkCreateCommandPool vkCreateCommandPool;
extern PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
extern PFN_vkCreateSemaphore vkCreateSemaphore;
extern PFN_vkCreateFence vkCreateFence;
extern PFN_vkWaitForFences vkWaitForFences;
extern PFN_vkResetFences vkResetFences;
extern PFN_vkDestroyFence vkDestroyFence;
extern PFN_vkDestroySemaphore vkDestroySemaphore;
extern PFN_vkResetCommandBuffer vkResetCommandBuffer;
extern PFN_vkFreeCommandBuffers vkFreeCommandBuffers;
extern PFN_vkResetCommandPool vkResetCommandPool;
extern PFN_vkDestroyCommandPool vkDestroyCommandPool;
extern PFN_vkCreateBufferView vkCreateBufferView;
extern PFN_vkDestroyBufferView vkDestroyBufferView;
extern PFN_vkQueueWaitIdle vkQueueWaitIdle;
extern PFN_vkCreateSampler vkCreateSampler;
extern PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout;
extern PFN_vkCreateDescriptorPool vkCreateDescriptorPool;
extern PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets;
extern PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets;
extern PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets;
extern PFN_vkFreeDescriptorSets vkFreeDescriptorSets;
extern PFN_vkResetDescriptorPool vkResetDescriptorPool;
extern PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool;
extern PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout;
extern PFN_vkDestroySampler vkDestroySampler;
extern PFN_vkCreateRenderPass vkCreateRenderPass;
extern PFN_vkCreateFramebuffer vkCreateFramebuffer;
extern PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
extern PFN_vkDestroyRenderPass vkDestroyRenderPass;
extern PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
extern PFN_vkCmdNextSubpass vkCmdNextSubpass;
extern PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
extern PFN_vkCreatePipelineCache vkCreatePipelineCache;
extern PFN_vkGetPipelineCacheData vkGetPipelineCacheData;
extern PFN_vkMergePipelineCaches vkMergePipelineCaches;
extern PFN_vkDestroyPipelineCache vkDestroyPipelineCache;
extern PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
extern PFN_vkCreateComputePipelines vkCreateComputePipelines;
extern PFN_vkDestroyPipeline vkDestroyPipeline;
extern PFN_vkDestroyEvent vkDestroyEvent;
extern PFN_vkDestroyQueryPool vkDestroyQueryPool;
extern PFN_vkCreateShaderModule vkCreateShaderModule;
extern PFN_vkDestroyShaderModule vkDestroyShaderModule;
extern PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
extern PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
extern PFN_vkCmdBindPipeline vkCmdBindPipeline;
extern PFN_vkCmdSetViewport vkCmdSetViewport;
extern PFN_vkCmdSetScissor vkCmdSetScissor;
extern PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
extern PFN_vkCmdDraw vkCmdDraw;
extern PFN_vkCmdDrawIndexed vkCmdDrawIndexed;
extern PFN_vkCmdDispatch vkCmdDispatch;
extern PFN_vkCmdCopyImage vkCmdCopyImage;
extern PFN_vkCmdPushConstants vkCmdPushConstants;
extern PFN_vkCmdClearColorImage vkCmdClearColorImage;
extern PFN_vkCmdClearDepthStencilImage vkCmdClearDepthStencilImage;
extern PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer;
extern PFN_vkCmdSetLineWidth vkCmdSetLineWidth;
extern PFN_vkCmdSetDepthBias vkCmdSetDepthBias;
extern PFN_vkCmdSetBlendConstants vkCmdSetBlendConstants;
extern PFN_vkCmdExecuteCommands vkCmdExecuteCommands;
extern PFN_vkCmdClearAttachments vkCmdClearAttachments;
extern PFN_vkInvalidateMappedMemoryRanges vkInvalidateMappedMemoryRanges;
extern PFN_vkCreateQueryPool vkCreateQueryPool;
extern PFN_vkCmdWriteTimestamp vkCmdWriteTimestamp;
extern PFN_vkCmdResetQueryPool vkCmdResetQueryPool;
extern PFN_vkGetQueryPoolResults vkGetQueryPoolResults;
//---

//DEVICE_LEVEL_VULKAN_FUNCTION_FROM_EXTENSION
extern PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
extern PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
extern PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
extern PFN_vkQueuePresentKHR vkQueuePresentKHR;
extern PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
//---
