#include "gpu/resource-box.hpp"

namespace gpu
{
#define DEF_DELETER(name)                                                                                    \
	template <>                                                                                              \
	void ResourceBox<SDL_GPU##name>::delete_resource() noexcept                                              \
	{                                                                                                        \
		if (device == nullptr || resource == nullptr) return;                                                \
		SDL_ReleaseGPU##name(device, resource);                                                              \
	}

	DEF_DELETER(Buffer)
	DEF_DELETER(ComputePipeline)
	DEF_DELETER(Fence)
	DEF_DELETER(GraphicsPipeline)
	DEF_DELETER(Sampler)
	DEF_DELETER(Shader)
	DEF_DELETER(Texture)
	DEF_DELETER(TransferBuffer)
}
