#pragma once

#include <SDL3/SDL_gpu.h>
#include <cassert>
#include <utility>

namespace gpu
{
	///
	/// @brief GPU Pass Template
	///
	template <typename T>
	class ScopedPass
	{
	  protected:

		friend class CommandBuffer;

		T* resource;

		ScopedPass(T* resource) noexcept :
			resource(resource)
		{
			assert(resource != nullptr);
		}

		void delete_resource() noexcept;

	  public:

		ScopedPass(const ScopedPass&) = delete;
		ScopedPass& operator=(const ScopedPass&) = delete;

		ScopedPass(ScopedPass&& other) noexcept :
			resource(other.resource)
		{
			other.resource = nullptr;
		}

		ScopedPass& operator=(ScopedPass&& other) noexcept
		{
			if (&other != this)
			{
				this->~ScopedPass();
				new (this) ScopedPass(std::move(other));
			}

			return *this;
		}

		operator T*() const noexcept { return this->resource; }

		///
		/// @brief Ends the current pass
		///
		///
		void end() noexcept
		{
			assert(resource != nullptr);
			delete_resource();
			resource = nullptr;
		}

		~ScopedPass() noexcept { assert(resource == nullptr); }
	};
}