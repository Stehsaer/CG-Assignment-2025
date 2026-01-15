#pragma once

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

struct LineVertex
{
	glm::vec2 position;
	glm::u8vec4 color;

	static const std::array<SDL_GPUVertexAttribute, 2> attributes;
	static const std::array<SDL_GPUVertexBufferDescription, 1> buffer_description;

	bool operator==(const LineVertex& other) const = default;
};
