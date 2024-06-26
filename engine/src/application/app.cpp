#include "app.h"

#include "spdlog/spdlog.h"

#include "lumina.h"

namespace lumina
{
	application_player* application_player::instance_ = nullptr;

	void application_player::run(const app_info_t& app_info)
	{
		spdlog::warn("App Starting...");
		spdlog::warn("Please wait while initializing.");
		spdlog::info("Built against Lumina Engine ver. --> " + (std::string)lumina::VERSION);

		// Set the app in running status
		is_running_ = true;

		// Init the application logger register
		LUMINA_SINGLETON_INIT_INSTANCE(lumina_logger);

		// Init surface
		surface_.get_graphics_driver().set_graphics_api(app_info.graphics_api);
		surface_.init(app_info.surface_width, app_info.surface_height, app_info.surface_name);

		// Calls On Init
        on_init();

		// Instance a variable to calculate the delta timem of the application
		float last_frame_time = 0.0f;

		// Log the app has successfully initialized
		spdlog::info("App successfully initialized.");

		// Join main application loop
        while (surface_.is_surface_opened() && is_running_)
        {
			// Update the application events and renders
			surface_.update_events();
            on_update();
			surface_.clear_screen();
            on_render();
			surface_.present();

			// Calculate the delta time
			float current_frame = (float)glfwGetTime();
			delta_time_ = current_frame - last_frame_time;
			last_frame_time = current_frame;
        }

		// Cleanup the surface
		surface_.on_destroy();
		surface_.destroy();

		// Cleanup the logger
		LUMINA_SINGLETON_DESTROY_INSTANCE(lumina_logger);
	}

	void application_player::exit()
	{
		spdlog::info("Exiting application player...");
		is_running_ = false;
	}
}