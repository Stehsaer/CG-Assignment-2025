#pragma once

#include "render/target/gbuffer.hpp"

namespace app::render
{
	///
	/// @brief Acquire and begin G-buffer Render Pass
	///
	/// @param command_buffer Command Buffer
	/// @param gbuffer G-buffer Target
	/// @return Acquired Render Pass
	///
	std::expected<gpu::Render_pass, util::Error> acquire_gbuffer_pass(
		const gpu::Command_buffer& command_buffer,
		const target::Gbuffer& gbuffer
	) noexcept;
}