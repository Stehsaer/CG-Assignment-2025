#include "graphics/util/smart-texture.hpp"

#include <glm/fwd.hpp>
#include <ranges>

namespace graphics
{
	glm::u32vec2 AutoTexture::get_size() const noexcept
	{
		return size;
	}

	std::expected<void, util::Error> AutoTexture::resize(
		SDL_GPUDevice* device,
		glm::u32vec2 new_size
	) noexcept
	{
		if (texture != nullptr && size == new_size) return {};
		if (new_size.x == 0 || new_size.y == 0) return util::Error("Invalid texture size");
		if (!format.supported_on(device)) return util::Error("Texture format not supported on device");

		size = new_size;
		auto create_texture_result =
			gpu::Texture::create(device, format.create(size.x, size.y, 1, mip_levels), name);
		if (!create_texture_result) return create_texture_result.error().forward("Resize failed");

		texture = std::make_unique<gpu::Texture>(std::move(create_texture_result.value()));
		return {};
	}

	const gpu::Texture& AutoTexture::operator*() const noexcept
	{
		assert(texture != nullptr && "Texture not initialized. Call resize() first.");
		return *texture;
	}

	const gpu::Texture* AutoTexture::operator->() const noexcept
	{
		assert(texture != nullptr && "Texture not initialized. Call resize() first.");
		return texture.get();
	}

	std::expected<void, util::Error> CycleTexture::resize_and_cycle(
		SDL_GPUDevice* device,
		glm::u32vec2 new_size
	) noexcept
	{
		if (new_size == size && !texture_pool.empty())
		{
			auto new_current = std::move(texture_pool.back());
			texture_pool.pop_back();
			texture_pool.insert(texture_pool.begin(), std::move(new_current));

			return {};
		}

		if (new_size.x == 0 || new_size.y == 0) return util::Error("Invalid texture size");
		if (!format.supported_on(device)) return util::Error("Texture format not supported on device");

		texture_pool.clear();
		size = new_size;

		for (const auto idx : std::views::iota(0zu, extra_pool_size + 2))
		{
			auto create_texture_result = gpu::Texture::create(
				device,
				format.create(size.x, size.y, 1, mip_levels),
				std::format("{} [Index {}]", name, idx)
			);
			if (!create_texture_result)
				return create_texture_result.error().forward("Create new texture failed");

			texture_pool.emplace_back(
				std::make_unique<gpu::Texture>(std::move(create_texture_result.value()))
			);
		}

		return {};
	}

	glm::u32vec2 CycleTexture::get_size() const noexcept
	{
		return size;
	}

	const gpu::Texture& CycleTexture::current() const noexcept
	{
		assert(!texture_pool.empty() && "Texture not initialized. Call resize_and_cycle() first.");
		return *texture_pool[0];
	}

	const gpu::Texture& CycleTexture::prev() const noexcept
	{
		assert(texture_pool.size() >= 2 && "Texture not initialized. Call resize_and_cycle() first.");
		return *texture_pool[1];
	}
}