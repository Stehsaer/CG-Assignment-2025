#include "renderer/gbuffer-gltf.hpp"
#include "gltf/model.hpp"
#include "graphics/corner.hpp"
#include "graphics/culling.hpp"

#include <algorithm>
#include <ranges>

namespace renderer
{
	Gbuffer_gltf::Drawdata::Drawdata(const glm::mat4& camera_matrix, const glm::vec3& eye_position) noexcept :
		camera_matrix(camera_matrix),
		eye_position(eye_position)
	{
		frustum_planes = graphics::compute_frustum_planes(camera_matrix);

		glm::vec4 near_plane_pos_homo(0.0, 0.0, 1.0, 1.0);
		near_plane_pos_homo = glm::inverse(camera_matrix) * near_plane_pos_homo;
		eye_to_nearplane = glm::vec3(near_plane_pos_homo) / near_plane_pos_homo.w - eye_position;
		near_distance = glm::length(eye_to_nearplane);
		eye_to_nearplane = glm::normalize(eye_to_nearplane);
	}

	void Gbuffer_gltf::Drawdata::append(const gltf::Drawdata& drawdata) noexcept
	{
		const auto current_resource_set_idx = resource_sets.size();
		resource_sets.emplace_back(
			Resource{
				.material_cache = drawdata.material_cache,
				.deferred_skinning_resource = drawdata.deferred_skin_resource
			}
		);

		auto visible_nonrigged_drawcalls =
			drawdata.drawcalls | std::views::filter([this](const auto& drawcall) -> bool {
				return graphics::box_in_frustum(
					drawcall.world_position_min,
					drawcall.world_position_max,
					frustum_planes
				);
			});

		/* Process Non-rigged Drawcalls */

		const auto point_in_range = [this](const glm::vec3& p) {
			const auto eye_to_p = p - eye_position;
			const auto eye_to_p_norm = eye_to_p / near_distance;
			return glm::dot(eye_to_p_norm, eye_to_nearplane) > 1;
		};

		const auto clip_to_world = [this](const glm::vec3& world) {
			const auto homo = camera_matrix * glm::vec4(world, 1.0f);
			return glm::vec3(homo) / homo.w;
		};

		for (const auto& drawcall : visible_nonrigged_drawcalls)
		{
			const auto& pipeline_mode = drawdata.material_cache[drawcall.material_index].params.pipeline;
			auto& target = drawcalls[std::pair(pipeline_mode, drawcall.is_rigged())];

			const auto [local_min_z, local_max_z] = std::ranges::minmax(
				graphics::get_corner_points(drawcall.world_position_min, drawcall.world_position_max)
					| std::views::filter(point_in_range)
					| std::views::transform(clip_to_world),
				{},
				&glm::vec3::z
			);
			min_z = std::min(local_min_z.z, min_z);

			if (target.empty()) target.reserve(1024);
			target.emplace_back(
				Drawcall{
					.drawcall = drawcall,
					.resource_set_index = current_resource_set_idx,
					.max_z = local_max_z.z
				}
			);
		}
	}

	void Gbuffer_gltf::Drawdata::sort() noexcept
	{
		for (auto& drawcalls_vec : drawcalls | std::views::values)
			std::ranges::sort(drawcalls_vec, std::greater{}, &Drawcall::max_z);
	}

	float Gbuffer_gltf::Drawdata::get_min_z() const noexcept
	{
		return glm::clamp(min_z, 0.0f, 0.9999f);
	}

	float Gbuffer_gltf::Drawdata::get_max_distance() const noexcept
	{
		glm::vec4 max_distance_point_h = glm::inverse(camera_matrix) * glm::vec4(1.0, 1.0, get_min_z(), 1.0);
		return -max_distance_point_h.z / max_distance_point_h.w;
	}

	std::expected<Gbuffer_gltf, util::Error> Gbuffer_gltf::create(SDL_GPUDevice* device) noexcept
	{
		return pipeline::Gbuffer_gltf::create(device)
			.transform([](pipeline::Gbuffer_gltf&& gbuffer_pipeline) noexcept {
				return Gbuffer_gltf{std::move(gbuffer_pipeline)};
			})
			.transform_error(util::Error::forward_fn("Create gbuffer pipeline failed"));
	}

	void Gbuffer_gltf::render(
		const gpu::Command_buffer& command_buffer,
		const gpu::Render_pass& gbuffer_pass,
		const Drawdata& drawdata
	) const noexcept
	{
		command_buffer.push_debug_group("Gbuffer Pass");
		for (const auto& [pipeline_cfg, drawcalls] : drawdata.drawcalls)
		{
			const auto [pipeline_mode, rigged] = pipeline_cfg;
			const auto& draw_pipeline = pipeline.get_pipeline_instance(pipeline_mode, rigged);

			draw_pipeline.bind(command_buffer, gbuffer_pass, drawdata.camera_matrix);

			for (const auto& [drawcall, set_idx, _] : drawcalls)
			{
				const auto& resource_set = drawdata.resource_sets[set_idx];

				draw_pipeline.set_material(
					command_buffer,
					gbuffer_pass,
					resource_set.material_cache[drawcall.material_index]
				);
				if (resource_set.deferred_skinning_resource != nullptr)
					draw_pipeline.set_skin(gbuffer_pass, *resource_set.deferred_skinning_resource);
				draw_pipeline.draw(command_buffer, gbuffer_pass, drawcall);
			}
		}
		command_buffer.pop_debug_group();
	}
}