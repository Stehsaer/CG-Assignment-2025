#include "graphic/util/smart-texture.hpp"

namespace graphic
{
	Smart_texture::Smart_texture(gpu::Texture::Format format) noexcept :
		format(format)
	{}

	std::expected<void, util::Error> Smart_texture::resize(
		SDL_GPUDevice* device,
		glm::u32vec2 new_size
	) noexcept
	{
		if (texture != nullptr && size == new_size) return {};

		if (new_size.x == 0 || new_size.y == 0) return util::Error("Invalid texture size");

		size = new_size;
		auto create_texture_result = gpu::Texture::create(device, format.create(size.x, size.y, 1, 1));
		if (!create_texture_result) return create_texture_result.error().propagate("Resize failed");

		texture = std::make_unique<gpu::Texture>(std::move(create_texture_result.value()));
		return {};
	}

	SDL_GPUTexture* Smart_texture::operator*() const noexcept
	{
		return *texture;
	}

}