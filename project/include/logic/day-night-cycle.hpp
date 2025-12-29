#pragma once

#include <glm/glm.hpp>

namespace logic
{
	///
	/// @brief 昼夜循环系统
	/// @details 管理时间流动、光照参数计算和UI控制
	///
	class Day_night_cycle
	{
	  public:

		///
		/// @brief 光照参数结构
		///
		struct Light_params
		{
			float azimuth;              // 方位角
			float pitch;                // 高度角
			glm::vec3 color;            // 光照颜色
			float intensity;            // 光照强度
			float ambient_intensity;    // 环境光强度
		};

		///
		/// @brief 更新昼夜循环
		/// @param delta_time 帧间隔时间（秒）
		///
		void update(float delta_time) noexcept;

		///
		/// @brief 获取当前时间对应的光照参数
		/// @return 光照参数
		///
		Light_params get_light_params() const noexcept;

		///
		/// @brief 显示昼夜循环控制UI
		///
		void control_ui() noexcept;

		///
		/// @brief 获取当前时间（0-24小时）
		///
		float get_time_of_day() const noexcept { return time_of_day; }

		///
		/// @brief 设置当前时间
		/// @param time 时间（0-24小时）
		///
		void set_time_of_day(float time) noexcept;

		///
		/// @brief 是否启用昼夜循环
		///
		bool is_enabled() const noexcept { return enable_cycle; }

		///
		/// @brief 是否自动控制光照
		///
		bool is_auto_light_control() const noexcept { return auto_light_control; }

	  private:

		bool enable_cycle = true;        // 是否启用昼夜循环
		float time_of_day = 8.0f;        // 当前时间 (0-24小时)
		float time_speed = 1.0f;         // 时间流速倍率
		bool auto_light_control = true;  // 是否自动控制光照

		///
		/// @brief 根据当前时间计算光照参数
		///
		Light_params calculate_light_params() const noexcept;
	};
}
