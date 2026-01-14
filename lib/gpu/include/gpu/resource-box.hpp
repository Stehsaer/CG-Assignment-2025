#pragma once

#include <SDL3/SDL_gpu.h>
#include <cassert>
#include <utility>

namespace gpu
{
	///
	/// @brief Smart wrapper for SDL-GPU resources
	///
	/// @tparam T Resource Type
	///
	template <typename T>
	class ResourceBox
	{
	  protected:

		SDL_GPUDevice* device;
		T* resource;

		// Resource deleter
		void delete_resource() noexcept;

	  public:

		ResourceBox(const ResourceBox&) = delete;
		ResourceBox& operator=(const ResourceBox&) = delete;

		ResourceBox(ResourceBox&& other) noexcept :
			device(other.device),
			resource(other.resource)
		{
			other.device = nullptr;
			other.resource = nullptr;
		}

		ResourceBox& operator=(ResourceBox&& other) noexcept
		{
			if (&other != this)
			{
				device = std::exchange(other.device, nullptr);
				resource = std::exchange(other.resource, nullptr);
			}

			return *this;
		}

		ResourceBox(SDL_GPUDevice* device, T* resource) noexcept :
			device(device),
			resource(resource)
		{
			assert(device != nullptr);
			assert(resource != nullptr);
		}

		~ResourceBox() noexcept
		{
			if (device == nullptr || resource == nullptr) return;
			delete_resource();
		}

		operator T*() const noexcept { return this->resource; }
	};
}
