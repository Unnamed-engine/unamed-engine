#include "VulkanSwapchain.hpp"
#include "VulkanRenderer.hpp"
#include "VkUtilsFactory.hpp"
#include "VkTypes.hpp"

void Hush::VulkanSwapchain::Recreate(uint32_t width, uint32_t height, Hush::VulkanRenderer* renderer)
{
	this->m_renderer = renderer;

	vkb::Swapchain vkbSwapChain = this->BuildVkbSwapchain(width, height, renderer);
	this->m_swapChainExtent = vkbSwapChain.extent;
	this->m_swapchain = vkbSwapChain.swapchain;
	this->m_images = vkbSwapChain.get_images().value();
	this->m_imageViews = vkbSwapChain.get_image_views().value();
	//> Init_Swapchain

	constexpr VkImageUsageFlags drawImageUsages = 
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | 
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | 
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | 
		VK_IMAGE_USAGE_STORAGE_BIT | 
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	
	VkExtent3D drawImageExtent = { width, height, 1 };

	VkImageCreateInfo rimgInfo =
		VkUtilsFactory::CreateImageCreateInfo(renderer->GetDrawImage().imageFormat, drawImageUsages, drawImageExtent);

	// for the draw image, we want to allocate it from gpu local memory
	VmaAllocationCreateInfo rimgAllocInfo = {};
	rimgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	rimgAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// allocate and create the image
	vmaCreateImage(renderer->GetVmaAllocator(), &rimgInfo, &rimgAllocInfo, &renderer->GetDrawImage().image,
		&renderer->GetDrawImage().allocation, nullptr);

	// build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo rViewInfo = VkUtilsFactory::CreateImageViewCreateInfo(
		renderer->GetDrawImage().imageFormat, renderer->GetDrawImage().image, VK_IMAGE_ASPECT_COLOR_BIT);

	HUSH_VK_ASSERT(vkCreateImageView(renderer->GetVulkanDevice(), &rViewInfo, nullptr, &renderer->GetDrawImage().imageView),
		"Failed to create image view");

	renderer->GetDepthImage().imageFormat = VK_FORMAT_D32_SFLOAT;
	renderer->GetDepthImage().imageExtent = drawImageExtent;
	VkImageUsageFlags depthImageUsages{};
	depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VkImageCreateInfo depthImgInfo = VkUtilsFactory::CreateImageCreateInfo(renderer->GetDepthImage().imageFormat, depthImageUsages, drawImageExtent);

	//allocate and create the image
	vmaCreateImage(
		renderer->GetVmaAllocator(),
		&depthImgInfo,
		&rimgAllocInfo,
		&renderer->GetDepthImage().image,
		&renderer->GetDepthImage().allocation,
		nullptr
	);

	//build a image-view for the draw image to use for rendering
	VkImageViewCreateInfo dview_info = VkUtilsFactory::CreateImageViewCreateInfo(
		renderer->GetDepthImage().imageFormat,
		renderer->GetDepthImage().image,
		VK_IMAGE_ASPECT_DEPTH_BIT
	);

	VkResult rc = vkCreateImageView(renderer->GetVulkanDevice(), &dview_info, nullptr, &renderer->GetDepthImage().imageView);
	HUSH_VK_ASSERT(rc, "Failed to create depth image view!");

	// add to deletion queues
	renderer->GetDeletionQueue().PushFunction([=]() {

		vkDestroyImageView(renderer->GetVulkanDevice(), renderer->GetDepthImage().imageView, nullptr);
		vmaDestroyImage(renderer->GetVmaAllocator(), renderer->GetDepthImage().image, renderer->GetDepthImage().allocation);
		vkDestroyImageView(renderer->GetVulkanDevice(), renderer->GetDrawImage().imageView, nullptr);
		vmaDestroyImage(renderer->GetVmaAllocator(), renderer->GetDrawImage().image, renderer->GetDrawImage().allocation);
	});
	//< Init_Swapchain

}

void Hush::VulkanSwapchain::Resize(uint32_t width, uint32_t height)
{
	this->m_renderer->EndUIFrame();
	vkDeviceWaitIdle(this->m_renderer->GetVulkanDevice());
	vkQueueWaitIdle(this->m_renderer->GetGraphicsQueue());
	this->Destroy();
	this->Recreate(width, height, this->m_renderer);
}

void Hush::VulkanSwapchain::Destroy()
{
	vkDestroySwapchainKHR(this->m_renderer->GetVulkanDevice(), this->m_swapchain, nullptr);

	for (VkImageView& imageView : this->m_imageViews)
	{
		vkDestroyImageView(this->m_renderer->GetVulkanDevice(), imageView, nullptr);
	}
	this->m_imageViews.clear();
}

const VkFormat& Hush::VulkanSwapchain::GetImageFormat() const noexcept
{
	return this->m_swapchainImageFormat;
}

const VkExtent2D& Hush::VulkanSwapchain::GetExtent() const noexcept
{
	return this->m_swapChainExtent;
}

const std::vector<VkImage>& Hush::VulkanSwapchain::GetImages() const noexcept
{
	return this->m_images;
}

const std::vector<VkImageView>& Hush::VulkanSwapchain::GetImageViews() const noexcept
{
	return this->m_imageViews;
}

VkSwapchainKHR Hush::VulkanSwapchain::GetRawSwapchain() noexcept
{
	return this->m_swapchain;
}

vkb::Swapchain Hush::VulkanSwapchain::BuildVkbSwapchain(uint32_t width, uint32_t height, VulkanRenderer* renderer)
{
	VkPhysicalDevice vulkanPhysicalDevice = renderer->GetVulkanPhysicalDevice();
	VkDevice device = renderer->GetVulkanDevice();
	VkSurfaceKHR surface = renderer->GetSurface();

	vkb::SwapchainBuilder swapchainBuilder{ vulkanPhysicalDevice, device, surface };

	//FIXME: Color info
	this->m_swapchainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;

	auto vkSurfaceFormat = VkSurfaceFormatKHR{};
	vkSurfaceFormat.format = this->m_swapchainImageFormat;
	vkSurfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

	return swapchainBuilder.set_desired_format(vkSurfaceFormat)
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();
}
