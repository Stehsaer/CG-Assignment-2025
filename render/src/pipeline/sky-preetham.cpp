#include "render/pipeline/sky-preetham.hpp"
#include "asset/shader/sky-preetham.frag.hpp"
#include "render/target/gbuffer.hpp"
#include "render/target/light.hpp"
#include "util/as-byte.hpp"

#include <numbers>

namespace render::pipeline
{
	std::expected<Sky_preetham, util::Error> Sky_preetham::create(SDL_GPUDevice* device) noexcept
	{
		const auto sampler_nearest_info = gpu::Sampler::Create_info{
			.min_filter = gpu::Sampler::Filter::Nearest,
			.mag_filter = gpu::Sampler::Filter::Nearest,
			.mipmap_mode = gpu::Sampler::Mipmap_mode::Nearest,
			.max_lod = 0.0f
		};

		auto sampler = gpu::Sampler::create(device, sampler_nearest_info);
		if (!sampler) return sampler.error().forward("Create sky sampler failed");

		auto fragment_shader = gpu::Graphics_shader::create(
			device,
			shader_asset::sky_preetham_frag,
			gpu::Graphics_shader::Stage::Fragment,
			0,
			0,
			0,
			1
		);
		if (!fragment_shader) return fragment_shader.error().forward("Create sky fragment shader failed");

		auto fullscreen_pass = graphics::Fullscreen_pass<false>::create(
			device,
			*fragment_shader,
			target::Light_buffer::light_buffer_format,
			"Sky Preetham Pipeline",
			graphics::Fullscreen_blend_mode::Add,
			graphics::Fullscreen_stencil_state{
				.depth_format = target::Gbuffer::depth_format.format,
				.enable_stencil_test = true,
				.compare_mask = 0xFF,
				.write_mask = 0x00,
				.compare_op = SDL_GPU_COMPAREOP_EQUAL,
				.reference = 0x00
			}
		);
		if (!fullscreen_pass) return fullscreen_pass.error().forward("Create sky fullscreen pass failed");

		return Sky_preetham(std::move(*sampler), std::move(*fullscreen_pass));
	}

	static float zenith_chromacity(
		const glm::vec4& c0,
		const glm::vec4& c1,
		const glm::vec4& c2,
		float sunTheta,
		float turbidity
	)
	{
		const glm::vec4 thetav = glm::vec4(sunTheta * sunTheta * sunTheta, sunTheta * sunTheta, sunTheta, 1);
		return glm::dot(
			glm::vec3(turbidity * turbidity, turbidity, 1),
			glm::vec3(glm::dot(thetav, c0), glm::dot(thetav, c1), glm::dot(thetav, c2))
		);
	}

	static float zenith_luminance(float sunTheta, float turbidity)
	{
		const float chi = (4.f / 9.f - turbidity / 120) * (std::numbers::pi_v<float> - 2 * sunTheta);
		return (4.0453f * turbidity - 4.9710f) * glm::tan(chi) - 0.2155f * turbidity + 2.4192f;
	}

	static float perez(float theta, float gamma, float A, float B, float C, float D, float E)
	{
		return (1.f + A * glm::exp(B / (glm::cos(theta) + 0.01f)))
			* (1.f + C * glm::exp(D * gamma) + E * glm::cos(gamma) * glm::cos(gamma));
	}

	Sky_preetham::Preetham_params::Preetham_params(float turbidity, glm::vec3 sun_direction) noexcept
	{
		const float sunTheta = glm::acos(glm::clamp(sun_direction.y, 0.f, 1.f));

		// A.2 Skylight Distribution Coefficients and Zenith Values: compute Perez distribution coefficients
		A = glm::vec3(-0.0193, -0.0167, 0.1787) * turbidity + glm::vec3(-0.2592, -0.2608, -1.4630);
		B = glm::vec3(-0.0665, -0.0950, -0.3554) * turbidity + glm::vec3(0.0008, 0.0092, 0.4275);
		C = glm::vec3(-0.0004, -0.0079, -0.0227) * turbidity + glm::vec3(0.2125, 0.2102, 5.3251);
		D = glm::vec3(-0.0641, -0.0441, 0.1206) * turbidity + glm::vec3(-0.8989, -1.6537, -2.5771);
		E = glm::vec3(-0.0033, -0.0109, -0.0670) * turbidity + glm::vec3(0.0452, 0.0529, 0.3703);

		// A.2 Skylight Distribution Coefficients and Zenith Values: compute zenith color
		Z.x = zenith_chromacity(
			glm::vec4(0.00166, -0.00375, 0.00209, 0),
			glm::vec4(-0.02903, 0.06377, -0.03202, 0.00394),
			glm::vec4(0.11693, -0.21196, 0.06052, 0.25886),
			sunTheta,
			turbidity
		);
		Z.y = zenith_chromacity(
			glm::vec4(0.00275, -0.00610, 0.00317, 0),
			glm::vec4(-0.04214, 0.08970, -0.04153, 0.00516),
			glm::vec4(0.15346, -0.26756, 0.06670, 0.26688),
			sunTheta,
			turbidity
		);
		Z.z = zenith_luminance(sunTheta, turbidity);
		Z.z *= 1000;  // conversion from kcd/m^2 to cd/m^2

		// 3.2 Skylight Model: pre-divide zenith color by distribution denominator
		Z.x /= perez(0, sunTheta, A.x, B.x, C.x, D.x, E.x);
		Z.y /= perez(0, sunTheta, A.y, B.y, C.y, D.y, E.y);
		Z.z /= perez(0, sunTheta, A.z, B.z, C.z, D.z, E.z);

		const float m_normalized_sun_y = 1.15;

		Z.z = m_normalized_sun_y / perez(sunTheta, 0, A.z, B.z, C.z, D.z, E.z);
	}

	Sky_preetham::Internal_params::Internal_params(const Params& params) noexcept :
		camera_mat_inv(params.camera_mat_inv),
		screen_size(glm::vec2(params.screen_size)),
		eye_position(params.eye_position),
		sun_direction(glm::normalize(params.sun_direction)),
		sun_intensity(params.sun_intensity),
		intensity(glm::dot(params.sun_intensity, glm::vec3(0.2126f, 0.7152f, 0.0722f))),
		preetham(params.turbidity, glm::normalize(params.sun_direction))
	{}

	void Sky_preetham::render(
		const gpu::Command_buffer& command_buffer,
		const gpu::Render_pass& render_pass,
		const Params& params
	) const noexcept
	{
		const Internal_params internal_params(params);
		command_buffer.push_uniform_to_fragment(0, util::as_bytes(internal_params));

		command_buffer.push_debug_group("Sky Preetham Pass");
		fullscreen_pass.render_to_renderpass(render_pass, {}, {}, {});
		command_buffer.pop_debug_group();
	}
}