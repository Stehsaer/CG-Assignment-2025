#include "geometry/vertex.hpp"

#include <SDL3/SDL_gpu.h>
#include <cstddef>

const std::array<SDL_GPUVertexAttribute, 2> LineVertex::attributes = std::to_array<SDL_GPUVertexAttribute>({
	{
     .location = 0,
     .buffer_slot = 0,
     .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
     .offset = 0,
	 },
	{
     .location = 1,
     .buffer_slot = 0,
     .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
     .offset = offsetof(LineVertex, color),
	 }
});

const std::array<SDL_GPUVertexBufferDescription, 1> LineVertex::buffer_description =
	std::to_array<SDL_GPUVertexBufferDescription>({
		{
         .slot = 0,
         .pitch = sizeof(LineVertex),
         .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
         .instance_step_rate = 0,
		 }
});
