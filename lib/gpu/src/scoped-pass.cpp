#include "gpu/scoped-pass.hpp"

namespace gpu
{
#define DEF_DELETER(name)                                                                                    \
	template <>                                                                                              \
	void ScopedPass<SDL_GPU##name>::delete_resource() noexcept                                               \
	{                                                                                                        \
		if (resource == nullptr) return;                                                                     \
		SDL_EndGPU##name(resource);                                                                          \
	}

	DEF_DELETER(ComputePass)
	DEF_DELETER(CopyPass)
	DEF_DELETER(RenderPass)

}