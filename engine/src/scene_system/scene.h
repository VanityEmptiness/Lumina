#pragma once

#include "entity.h"

#include "components/components.h"
#include "physics/2d/physics_simulator_2d.h"

#include "entt/entt.hpp"

#include <string>

namespace lumina
{
	// Contains all the objects, components, sounds, lights and everything a scene should contains
	class scene
	{
	public:

		scene(const std::string& name);

		// Get's the scene name
		const std::string& get_name() const { return name_; }

		// Create's an entity
		entity create_entity();

		// Duplicate an existing entity
		entity duplicate_entity(entity src_entity);

		// Destroy's an entity
		void destroy_entity(entt::entity entity);

		// Get's the entity registry
		entt::registry& get_entity_registry() { return registry_; }

		// Get's the scene camera
		camera_component* get_camera() { return camera_; }

		// Get's the 2D physics simulator
		physics_simulator_2d& get_physics_simulator_2d() { return physics_2d_sim_; }

		// Get's a entity by a given id or null entity if not found
		entity get_entity_by_id(const std::string& id);

		// Wheter if the scene has an active scene camera
		bool has_camera() { return camera_ != nullptr; }

		// Wherter if the given camera is the active one
		bool is_active_camera(camera_component* camera) { return camera_ == camera; }

		// Set the scene camera
		void set_camera(camera_component* camera) { camera_ = camera; }

	private:
		
		// The name of the scene
		std::string name_{""};

		// The entity registry (contains all the entities of the scene)
		entt::registry registry_;

		// The active camera used to render the scene
		camera_component* camera_{};

		// The 2d physics simulator of the scene
		physics_simulator_2d physics_2d_sim_;

		// Set the scene name
		void set_name(const std::string& new_name) { name_ = new_name; }

		// Called on scene creation
		void on_create();

		// Called on scene destruction
		void on_destroy();

		// Called on activation
		void on_activate();

		// Called on deactivation
		void on_deactivate();

		// Called whenever the scene get's loaded with all its entities (example: when it get's deserialized)
		void on_load();

	private:

		friend class scenes_system;
		friend class scene_serializer;

	};
}