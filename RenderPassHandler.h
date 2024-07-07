#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//vulkan.h is loaded above

class RenderPassHandler{
	VkRenderPass renderPass;
	
	VkDevice& logicalDevice;

public:
	RenderPassHandler(VkDevice& _ld, VkFormat _scif) : logicalDevice(_ld) {
		createRenderPass(_scif);
	}

	~RenderPassHandler(){
		vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
	}

	inline VkRenderPass& getRenderPass() { return renderPass; }

private:
    void createRenderPass(VkFormat swapchainImageFormat){
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapchainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

		//what to do with the data in the attachment before and after rendering
		//options for load op:
		//LOAD_OP_LOAD: preserve the existing contents of the attachment
		//LOAD_OP_CLEAR: clear the value to a constant at the start
		//LOAD_OP_DONT_CARE: existing conents are undefined, we dont care about them
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		//options for store op:
		//STORE_OP_STORE: rendered contents will be stored in memory and can be read later
		//STORE_OP_DONT_CARE: conents of the frambuffer will be undefined after the rendering operation
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		//we dont do anything with stencils so we dont care
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		//image layout before the render pass begins
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; //we dont care what the previous was
		//image layout to automatically convert to after the render pass
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0; //which attachment to reference to by index below
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1; //referenced directly from fragment shader with layout(location = 0)out vec4 outColor;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if(vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) throw std::runtime_error("Failed to create render pass!\n");
	}
};