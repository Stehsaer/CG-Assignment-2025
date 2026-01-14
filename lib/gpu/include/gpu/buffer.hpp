#pragma once

#include <SDL3/SDL_gpu.h>
#include <expected>
#include <functional>
#include <span>

#include "resource-box.hpp"
#include "util/error.hpp"

namespace gpu
{
	///
	/// @brief GPU Buffer
	///
	///
	class Buffer : public ResourceBox<SDL_GPUBuffer>
	{
	  public:

		Buffer(const Buffer&) = delete;
		Buffer& operator=(const Buffer&) = delete;
		Buffer(Buffer&&) = default;
		Buffer& operator=(Buffer&&) = default;
		~Buffer() noexcept = default;

		///
		/// @brief Usage flags for GPU buffer
		///
		///
		struct Usage
		{
			bool vertex                : 1 = false;
			bool index                 : 1 = false;
			bool indirect              : 1 = false;
			bool graphic_storage_read  : 1 = false;
			bool compute_storage_read  : 1 = false;
			bool compute_storage_write : 1 = false;

			operator SDL_GPUBufferUsageFlags(this Usage self) noexcept;

			auto operator<=>(const Usage&) const = default;
			bool operator==(const Usage&) const = default;
		};

		///
		/// @brief Create a GPU buffer
		///
		/// @param usage Usage of the buffer
		/// @param size Size of the buffer in bytes, must be greater than 0
		/// @param name Optional name for the buffer
		/// @return Buffer object, or error if failed
		///
		static std::expected<Buffer, util::Error> create(
			SDL_GPUDevice* device,
			Usage usage,
			uint32_t size,
			const std::string& name
		) noexcept;

	  private:

		using ResourceBox<SDL_GPUBuffer>::ResourceBox;
	};

	///
	/// @brief CPU-side transfer buffer
	///
	///
	class TransferBuffer : public ResourceBox<SDL_GPUTransferBuffer>
	{
	  public:

		TransferBuffer(const TransferBuffer&) = delete;
		TransferBuffer& operator=(const TransferBuffer&) = delete;
		TransferBuffer(TransferBuffer&&) = default;
		TransferBuffer& operator=(TransferBuffer&&) = default;
		~TransferBuffer() noexcept = default;

		enum class Usage
		{
			Upload = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			Download = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
		};

		///
		/// @brief Creates a transfer buffer
		///
		/// @param usage Transfer buffer usage
		/// @param size Size of the buffer in bytes, must be greater than 0
		/// @return Transfer buffer object, or error if failed
		///
		static std::expected<TransferBuffer, util::Error> create(
			SDL_GPUDevice* device,
			Usage usage,
			uint32_t size
		) noexcept;

		///
		/// @brief Transfer data using a callback function
		/// @details The functions maps the buffer, calls the callback with the mapped pointer, then unmaps
		/// the buffer
		/// @param callback Callback function that performs the data transfer
		/// @param cycle Cycle mode
		///
		std::expected<void, util::Error> transfer(
			const std::function<void(void* mapped_ptr)>& callback,
			bool cycle
		) const noexcept;

		///
		/// @brief Maps and uploads data to the transfer buffer
		/// @warning Size of the transfer buffer must match the size of the data span
		///
		/// @param data Data to upload
		/// @param cycle Cycle mode
		///
		std::expected<void, util::Error> upload_to_buffer(std::span<const std::byte> data, bool cycle);

		///
		/// @brief Maps and downloads data from the transfer buffer
		/// @warning Size of the transfer buffer must match the size of the output data span
		///
		/// @param out_data Output data span
		///
		std::expected<void, util::Error> download_from_buffer(std::span<std::byte> out_data) const;

		///
		/// @brief Create a transfer buffer and upload existing data
		///
		/// @param data Data to upload
		/// @return Transfer buffer object, or error if failed
		///
		static std::expected<TransferBuffer, util::Error> create_from_data(
			SDL_GPUDevice* device,
			std::span<const std::byte> data
		) noexcept;

	  private:

		using ResourceBox<SDL_GPUTransferBuffer>::ResourceBox;

		uint32_t size;
		Usage usage;
	};
}