#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#include "backend.h"

// Headless: no surface, no swapchain. The session renders into a texture it
// owns and hands the pixels back through mln_texture_read_premultiplied_rgba8.

static VkInstance g_instance = VK_NULL_HANDLE;
static VkPhysicalDevice g_physical_device = VK_NULL_HANDLE;
static VkDevice g_device = VK_NULL_HANDLE;
static VkQueue g_queue = VK_NULL_HANDLE;
static uint32_t g_queue_family = 0;

const char* backend_name(void) {
    return "Vulkan";
}

static int pick_physical_device(void) {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(g_instance, &count, NULL);
    if (count == 0) {
        printf("FAIL no Vulkan physical device (is an ICD installed?)\n");
        return 1;
    }
    VkPhysicalDevice* devices = calloc(count, sizeof *devices);
    vkEnumeratePhysicalDevices(g_instance, &count, devices);

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t families = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &families, NULL);
        VkQueueFamilyProperties* props = calloc(families, sizeof *props);
        vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &families, props);
        for (uint32_t f = 0; f < families; ++f) {
            if (props[f].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                g_physical_device = devices[i];
                g_queue_family = f;
                free(props);
                free(devices);
                return 0;
            }
        }
        free(props);
    }
    free(devices);
    printf("FAIL no graphics queue family\n");
    return 1;
}

int backend_setup(void) {
    const VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "mln-ffi-smoke",
        .apiVersion = VK_API_VERSION_1_1,
    };
    const VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app,
    };
    if (vkCreateInstance(&instance_info, NULL, &g_instance) != VK_SUCCESS) {
        printf("FAIL vkCreateInstance\n");
        return 1;
    }
    if (pick_physical_device() != 0) {
        return 1;
    }

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(g_physical_device, &props);
    printf("Vulkan device: %s (queue family %u)\n", props.deviceName,
           g_queue_family);

    const float priority = 1.0f;
    const VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = g_queue_family,
        .queueCount = 1,
        .pQueuePriorities = &priority,
    };
    const VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
    };
    if (vkCreateDevice(g_physical_device, &device_info, NULL, &g_device) !=
        VK_SUCCESS) {
        printf("FAIL vkCreateDevice\n");
        return 1;
    }
    vkGetDeviceQueue(g_device, g_queue_family, 0, &g_queue);
    return 0;
}

mln_status backend_attach(mln_map map, unsigned width, unsigned height,
                          mln_render_session* out_session) {
    mln_vulkan_owned_texture_descriptor descriptor =
        mln_vulkan_owned_texture_descriptor_default();
    descriptor.extent.width = width;
    descriptor.extent.height = height;
    descriptor.extent.scale_factor = 1.0;
    descriptor.context.instance = g_instance;
    descriptor.context.physical_device = g_physical_device;
    descriptor.context.device = g_device;
    descriptor.context.graphics_queue = g_queue;
    descriptor.context.graphics_queue_family_index = g_queue_family;
    descriptor.context.get_instance_proc_addr = (void*)vkGetInstanceProcAddr;
    descriptor.context.get_device_proc_addr = (void*)vkGetDeviceProcAddr;
    return mln_vulkan_owned_texture_attach(map, &descriptor, out_session);
}

void backend_report_host_context(const char* when) {
    // Vulkan has no thread-current context to lose, so unlike EGL there is
    // nothing here that a careless backend could clobber. Printed for symmetry.
    printf("host VkDevice %s: %p\n", when, (void*)g_device);
}

void backend_release_current(void) {
    // Vulkan has no thread-current context to release.
}

void backend_begin_render(void) {
    // Nothing to make current.
}

void backend_end_render(void) {
    // Nothing to restore.
}
