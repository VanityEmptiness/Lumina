#pragma once

#include "graphics/graphics_driver.h"

#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

#include <string>

namespace lumina
{
	// Represent the abstracted surface of the application on a desktop environment it would be the "Window"
	class app_surface
	{
	public:

		// Disable the cursor pointer (acquire the mouse input directly without the cursor to be on the screen)
		void enable_cursor(bool on) { glfwSetInputMode(window_, GLFW_CURSOR, on ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED); }

		// Get's the surface handle class when app is running on a Desktop device
		GLFWwindow* get_desktop_handler() const { return window_; }

		// Wheter if the surface is opened or it's been closed (on desktop devices for example closed by cliking exit button)
		const bool is_surface_opened() const { return !glfwWindowShouldClose(window_); }

		// Get's the graphics driver used to render everything on screen
		graphics_driver& get_graphics_driver() { return graphics_driver_; }

		// Get's the surface width and height
		const glm::vec2 get_dimensions() const { int width, height; glfwGetWindowSize(window_, &width, &height); return { width, height }; }

		// Change the surface name
		void set_name(const std::string& name) const { glfwSetWindowTitle(window_, name.c_str()); }

	private:

		// Creates the surface
		void init(float surface_width, float surface_height, const std::string& surface_name);

		// Poll and updates surface events
		void update_events();

		// Destroys the surface
		void destroy();

		// Install the event handler and register all the surface callbacks
		void install_events_handler();

		// Clear the surface screen
		void clear_screen();

		// Present all the render data to the surface screen
		void present();

		// Called on surface destruction
		void on_destroy();

	private:

		// Holds the graphic driver the application uses to render everything on screen
		graphics_driver graphics_driver_;

		// Holds Glfw Window that represent the surface on desktop devices
		GLFWwindow* window_ = nullptr;

	private:
		
		friend class application_player;
	
	};
}