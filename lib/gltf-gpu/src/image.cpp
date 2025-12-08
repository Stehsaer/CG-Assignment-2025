#include "gltf/image.hpp"

#include <graphic/util/tool.hpp>
#include <image/algo/mipmap.hpp>
#include <image/compress.hpp>
#include <util/as-byte.hpp>

namespace gltf
{
	std::vector<Image_refcount> compute_image_refcounts(const tinygltf::Model& model) noexcept
	{
		std::vector<Image_refcount> refcount_list(model.images.size());

		const auto count_color_texture = [&model, &refcount_list](const auto& texture_info) {
			if (const auto index = texture_info.index;
				index >= 0 && std::cmp_less(index, model.textures.size()))
			{
				if (const auto source = model.textures[index].source;
					source >= 0 && std::cmp_less(source, model.images.size()))
				{
					refcount_list[source].color_refcount++;
				}
			}
		};

		const auto count_normal_texture = [&model, &refcount_list](const auto& texture_info) {
			if (const auto index = texture_info.index;
				index >= 0 && std::cmp_less(index, model.textures.size()))
			{
				if (const auto source = model.textures[index].source;
					source >= 0 && std::cmp_less(source, model.images.size()))
				{
					refcount_list[source].normal_refcount++;
				}
			}
		};

		for (const auto& material : model.materials)
		{
			count_color_texture(material.pbrMetallicRoughness.baseColorTexture);
			count_color_texture(material.pbrMetallicRoughness.metallicRoughnessTexture);
			count_color_texture(material.occlusionTexture);
			count_color_texture(material.emissiveTexture);
			count_normal_texture(material.normalTexture);
		}

		return refcount_list;
	}

	template <typename T>
	static std::span<const T> as_span(const tinygltf::Image& image) noexcept
	{
		return std::span<const T>(reinterpret_cast<const T*>(image.image.data()), image.width * image.height);
	}

	static std::expected<image::Image<image::Precision::U8, image::Format::RGBA>, util::Error>
	extract_u8_rgba(const tinygltf::Image& image) noexcept
	{
		const bool is_8bit = image.bits == 8 && image.pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
		const bool is_16bit = image.bits == 16 && image.pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;

		image::Image<image::Precision::U8, image::Format::RGBA> img{
			.size = glm::u32vec2(uint32_t(image.width), uint32_t(image.height)),
			.pixels = std::vector<image::Pixel_t<image::Precision::U8, image::Format::RGBA>>(
				image.width * image.height
			)
		};

		if (image.component == 4)
		{
			if (is_8bit)
				std::ranges::copy(as_span<glm::u8vec4>(image), img.pixels.begin());
			else if (is_16bit)
				std::ranges::copy(
					as_span<glm::u16vec4>(image) | std::views::transform([](const glm::u16vec4& rgba) {
						return glm::u8vec4(rgba / uint16_t(256));
					}),
					img.pixels.begin()
				);
			else
				return util::Error(
					std::format(
						"Mismatched image bit depth ({}) or pixel type ({})",
						image.bits,
						image.pixel_type
					)
				);
		}
		else if (image.component == 3)
		{
			if (is_8bit)
				std::ranges::copy(
					as_span<glm::u8vec3>(image) | std::views::transform([](const glm::u8vec3& rgb) {
						return glm::u8vec4(rgb, 255);
					}),
					img.pixels.begin()
				);
			else if (is_16bit)
				std::ranges::copy(
					as_span<glm::u16vec3>(image) | std::views::transform([](const glm::u16vec3& rgb) {
						return glm::u8vec4(rgb / uint16_t(256), 255);
					}),
					img.pixels.begin()
				);
			else
				return util::Error(
					std::format(
						"Mismatched image bit depth ({}) or pixel type ({})",
						image.bits,
						image.pixel_type
					)
				);
		}
		else
			return util::Error(
				std::format("Unsupported number of components ({}) for color texture.", image.component)
			);

		return std::move(img);
	}

	static std::expected<image::Image<image::Precision::U16, image::Format::RGBA>, util::Error>
	extract_u16_rgba(const tinygltf::Image& image) noexcept
	{
		if (image.bits != 16 || image.pixel_type != TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
			return util::Error(
				std::format(
					"Mismatched image bit depth ({}) or pixel type ({})",
					image.bits,
					image.pixel_type
				)
			);

		image::Image<image::Precision::U16, image::Format::RGBA> img{
			.size = glm::u32vec2(uint32_t(image.width), uint32_t(image.height)),
			.pixels = std::vector<image::Pixel_t<image::Precision::U16, image::Format::RGBA>>(
				image.width * image.height
			)
		};

		if (image.component == 4)
			std::ranges::copy(as_span<glm::u16vec4>(image), img.pixels.begin());
		else if (image.component == 3)
			std::ranges::copy(
				as_span<glm::u16vec3>(image) | std::views::transform([](const glm::u16vec3& rgb) {
					return glm::u8vec4(rgb, 255);
				}),
				img.pixels.begin()
			);
		else
			return util::Error(
				std::format("Unsupported number of components ({}) for color texture.", image.component)
			);

		return std::move(img);
	}

	static bool dim_power_of_2(uint32_t value) noexcept
	{
		return (value != 0) && ((value & (value - 1)) == 0);
	}

	static bool image_power_of_2(glm::u32vec2 size) noexcept
	{
		return dim_power_of_2(size.x) && dim_power_of_2(size.y);
	}

	static std::expected<gpu::Texture, util::Error> create_color_uncompressed(
		SDL_GPUDevice* device,
		const tinygltf::Image& image
	) noexcept
	{
		auto img = extract_u8_rgba(image);
		if (!img) return img.error().propagate("Extract image failed");

		const auto mipmap_chain = image::generate_mipmap(*img);

		return graphic::create_texture_from_mipmap_chain(
			device,
			gpu::Texture::Format{
				.type = SDL_GPU_TEXTURETYPE_2D,
				.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
				.usage = {.sampler = true}
			},
			mipmap_chain
		);
	}

	static std::expected<gpu::Texture, util::Error> create_color_bc3(
		SDL_GPUDevice* device,
		const tinygltf::Image& image
	) noexcept
	{
		if (!image_power_of_2({uint32_t(image.width), uint32_t(image.height)}))
			return create_color_uncompressed(device, image);

		auto compressed_mipmap_chain =
			extract_u8_rgba(image)
				.transform([](const image::Image_container<glm::vec<4, unsigned char>>& uncompressed_image) {
					return image::generate_mipmap(uncompressed_image, {4, 4});
				})
				.and_then(image::Compress_mipmap(image::compress_to_bc3));
		if (!compressed_mipmap_chain)
			return compressed_mipmap_chain.error().propagate("Extract image or compress to BC3 failed");

		return graphic::create_texture_from_mipmap_chain(
			device,
			gpu::Texture::Format{
				.type = SDL_GPU_TEXTURETYPE_2D,
				.format = SDL_GPU_TEXTUREFORMAT_BC3_RGBA_UNORM,
				.usage = {.sampler = true}
			},
			*compressed_mipmap_chain
		);
	}

	static std::expected<gpu::Texture, util::Error> create_color_bc7(
		SDL_GPUDevice* device,
		const tinygltf::Image& image
	) noexcept
	{
		if (!image_power_of_2({uint32_t(image.width), uint32_t(image.height)}))
			return create_color_uncompressed(device, image);

		auto compressed_mipmap_chain =
			extract_u8_rgba(image)
				.transform([](const image::Image_container<glm::vec<4, unsigned char>>& uncompressed_image) {
					return image::generate_mipmap(uncompressed_image, {4, 4});
				})
				.and_then(image::Compress_mipmap(image::compress_to_bc7));
		if (!compressed_mipmap_chain)
			return compressed_mipmap_chain.error().propagate("Extract image or compress to BC7 failed");

		return graphic::create_texture_from_mipmap_chain(
			device,
			gpu::Texture::Format{
				.type = SDL_GPU_TEXTURETYPE_2D,
				.format = SDL_GPU_TEXTUREFORMAT_BC7_RGBA_UNORM,
				.usage = {.sampler = true}
			},
			*compressed_mipmap_chain
		);
	}

	static std::expected<gpu::Texture, util::Error> create_normal_8bit(
		SDL_GPUDevice* device,
		const tinygltf::Image& image,
		bool compress
	) noexcept
	{
		auto img = extract_u8_rgba(image);
		if (!img) return img.error().propagate("Extract RGBA image failed");

		// Non Power-of-two textures are not compressed and mipmapped
		if (!image_power_of_2({uint32_t(image.width), uint32_t(image.height)}))
			return graphic::create_texture_from_image(
				device,
				gpu::Texture::Format{
					.type = SDL_GPU_TEXTURETYPE_2D,
					.format = SDL_GPU_TEXTUREFORMAT_R8G8_UNORM,
					.usage = {.sampler = true}
				},
				img->map([](const glm::u8vec4& pixel) -> glm::u8vec2 { return {pixel.r, pixel.g}; })
			);

		// Power-of-two textures can be compressed
		if (compress)
		{
			auto mipmap_chain = image::generate_mipmap(img.value(), {4, 4});

			auto compressed_mipmap_chain =
				image::Compress_mipmap(image::compress_to_bc5)(std::move(mipmap_chain));
			if (!compressed_mipmap_chain)
				return compressed_mipmap_chain.error().propagate("Compress to BC5 failed");

			return graphic::create_texture_from_mipmap_chain(
				device,
				gpu::Texture::Format{
					.type = SDL_GPU_TEXTURETYPE_2D,
					.format = SDL_GPU_TEXTUREFORMAT_BC5_RG_UNORM,
					.usage = {.sampler = true}
				},
				*compressed_mipmap_chain
			);
		}
		else
		{
			auto mipmap_chain = image::generate_mipmap(img->map([](const glm::u8vec4& pixel) -> glm::u8vec2 {
				return {pixel.r, pixel.g};
			}));

			return graphic::create_texture_from_mipmap_chain(
				device,
				gpu::Texture::Format{
					.type = SDL_GPU_TEXTURETYPE_2D,
					.format = SDL_GPU_TEXTUREFORMAT_R8G8_UNORM,
					.usage = {.sampler = true}
				},
				mipmap_chain
			);
		}
	}

	static std::expected<gpu::Texture, util::Error> create_normal_16bit(
		SDL_GPUDevice* device,
		const tinygltf::Image& image,
		bool compress
	) noexcept
	{
		auto img = extract_u16_rgba(image);
		if (!img) return img.error().propagate("Extract RGBA image failed");

		// Non Power-of-two textures are not compressed and mipmapped
		if (!image_power_of_2({uint32_t(image.width), uint32_t(image.height)}))
			return graphic::create_texture_from_image(
				device,
				gpu::Texture::Format{
					.type = SDL_GPU_TEXTURETYPE_2D,
					.format = SDL_GPU_TEXTUREFORMAT_R16G16_UNORM,
					.usage = {.sampler = true}
				},
				img->map([](const glm::u16vec4& pixel) -> glm::u16vec2 { return {pixel.r, pixel.g}; })
			);

		// Power-of-two textures can be compressed
		if (compress)
		{
			auto mipmap_chain = image::generate_mipmap(
				img->map([](const glm::u16vec4& pixel) -> glm::u8vec4 { return pixel / uint16_t(256); }),
				{4, 4}
			);

			auto compressed_mipmap_chain =
				image::Compress_mipmap(image::compress_to_bc5)(std::move(mipmap_chain));
			if (!compressed_mipmap_chain)
				return compressed_mipmap_chain.error().propagate("Compress to BC5 failed");

			return graphic::create_texture_from_mipmap_chain(
				device,
				gpu::Texture::Format{
					.type = SDL_GPU_TEXTURETYPE_2D,
					.format = SDL_GPU_TEXTUREFORMAT_BC5_RG_UNORM,
					.usage = {.sampler = true}
				},
				*compressed_mipmap_chain
			);
		}
		else
		{
			auto mipmap_chain = image::generate_mipmap(
				img->map([](const glm::u16vec4& pixel) -> glm::u16vec2 { return {pixel.r, pixel.g}; })
			);

			return graphic::create_texture_from_mipmap_chain(
				device,
				gpu::Texture::Format{
					.type = SDL_GPU_TEXTURETYPE_2D,
					.format = SDL_GPU_TEXTUREFORMAT_R16G16_UNORM,
					.usage = {.sampler = true}
				},
				mipmap_chain
			);
		}
	}

	static std::expected<gpu::Texture, util::Error> create_normal(
		SDL_GPUDevice* device,
		const tinygltf::Image& image,
		Image_compress_mode compress_mode
	) noexcept
	{
		const bool compress_when_8bit =
			(compress_mode == Image_compress_mode::RGn_BC5
			 || compress_mode == Image_compress_mode::RG16_raw_RG8_BC5);
		const bool compress_when_16bit = (compress_mode == Image_compress_mode::RGn_BC5);

		if (image.bits == 8 && image.pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
			return create_normal_8bit(device, image, compress_when_8bit);
		else if (image.bits == 16 && image.pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
			return create_normal_16bit(device, image, compress_when_16bit);
		else
			return util::Error(
				std::format(
					"Unsupported image bit depth ({}) or pixel type ({})",
					image.bits,
					image.pixel_type
				)
			);
	}

	std::expected<gpu::Texture, util::Error> create_texture_from_image(
		SDL_GPUDevice* device,
		const tinygltf::Image& image,
		Image_compress_mode compress_mode
	) noexcept
	{
		switch (compress_mode)
		{
		case Image_compress_mode::RGBA8_raw:
			return create_color_uncompressed(device, image);
		case Image_compress_mode::RGBA8_BC3:
			return create_color_bc3(device, image);
		case Image_compress_mode::RGBA8_BC7:
			return create_color_bc7(device, image);
		case Image_compress_mode::RGn_BC5:
		case Image_compress_mode::RG16_raw_RG8_BC5:
		case Image_compress_mode::RGn_raw:
			return create_normal(device, image, compress_mode);
		}

		std::unreachable();
	}

	std::expected<gpu::Texture, util::Error> create_placeholder_image(
		SDL_GPUDevice* device,
		glm::vec4 color
	) noexcept
	{
		image::Image<image::Precision::U8, image::Format::RGBA> img{
			.size = glm::u32vec2(1, 1),
			.pixels = std::vector<image::Pixel_t<image::Precision::U8, image::Format::RGBA>>(1)
		};

		img.pixels[0] = glm::u8vec4(
			static_cast<std::uint8_t>(glm::clamp(color.r, 0.0f, 1.0f) * 255.0f),
			static_cast<std::uint8_t>(glm::clamp(color.g, 0.0f, 1.0f) * 255.0f),
			static_cast<std::uint8_t>(glm::clamp(color.b, 0.0f, 1.0f) * 255.0f),
			static_cast<std::uint8_t>(glm::clamp(color.a, 0.0f, 1.0f) * 255.0f)
		);

		return graphic::create_texture_from_image(
			device,
			{.type = SDL_GPU_TEXTURETYPE_2D,
			 .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
			 .usage = {.sampler = true}},
			img
		);
	}
}