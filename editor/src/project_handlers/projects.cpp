#include "projects.h"

#include "../editor_compile_configs.h"
#include "ui/view_register.h"
#include "ui/ui_shared_vars.h"
#include "ui/views/content_browser_view.h"
#include "serializers/assets_serializer.h"

#include "core/lumina_file_system.h"

#include "yaml-cpp/yaml.h"
#include "spdlog/spdlog.h"

#include <future>

LUMINA_SINGLETON_DECL_INSTANCE(lumina_editor::project_handler);

#define LUMINA_EDITOR_TEMPLATE_DIR_PATH "\\resources\\template"
#define LUMINA_EDITOR_BIN_DIR_PATH "\\resources\\bin"
#define LUMINA_EDITOR_COMPILED_SCRIPT_DIR_PATH "\\compiled\\net4.5"
#define LUMINA_EDITOR_SCRIPT_CORE_DIR_PATH "\\net4.5"

#define LUMINA_EDITOR_RUNTIME_PLAYER_DEFAULT_NAME "lumina_runtime_player.exe"
#define LUMINA_EDITOR_SCRIPT_CORE_DEFAULT_NAME "lumina_scripting_core.dll"
#define LUMINA_EDITOR_SCRIPT_COMPILED_DEFAULT_NAME "LuminaScript.dll"
#define LUMINA_EDITOR_GENERATION_PREMAKE_DEFAULT_NAME "premake5.exe"
#define LUMINA_EDITOR_GENERATION_BATCH_DEFAULT_NAME "run_generation.bat"
#define LUMINA_EDITOR_SLN_STARTUP_BATCH_DEFAULT_NAME "run_sln.bat"
#define LUMINA_EDITOR_GENERATION_SCRIPT_DEFAULT_NAME "proj_solution_gen.lua"
#define LUMINA_EDITOR_APP_SCRIPT_DEFAULT_NAME "app.cs"
#define LUMINA_EDITOR_RUNTIME_PLAYER_CONFIGS_DEFAULT_NAME "runtime.config"

namespace lumina_editor
{
	void project_handler::create_project(const std::string& directory_path, const std::string& project_name)
	{
		// If a project is already loaded unloads it first
		if (has_opened_project())
			unload_project();
		else
		{
			destroy_scenes_context();
			destroy_assets();
		}

		// Log the save msg
		spdlog::warn("Creating new project...");

		// Create's a new project object and assign it to loaded project
		loaded_project_ = std::make_shared<project>();
		loaded_project_->name = project_name;
		loaded_project_->scenes_dir_path = LUMINA_EDITOR_PROJECT_SCENES_DEFAULT_PATH;
		loaded_project_->project_dir_path = directory_path + "\\" + project_name;

		// Set the content browser directory
		content_browser_.set_content_directory(loaded_project_->project_dir_path);

		// Create's the projects folders
		lumina::lumina_file_system_s::create_folder(loaded_project_->project_dir_path);
		lumina::lumina_file_system_s::create_folder(loaded_project_->project_dir_path + "\\" + LUMINA_EDITOR_PROJECT_SCENES_DEFAULT_PATH);
		lumina::lumina_file_system_s::create_folder(loaded_project_->project_dir_path + "\\" + LUMINA_EDITOR_PROJECT_ASSETS_DEFAULT_PATH);

		// Create's an empty scene and loads it
		lumina::scenes_system::get_singleton().create_scene("Untitled Scene");

		// Save the project
		save_project();

		// Generates the scripting project
		generate_scripting_proj();

		// Refresh the content browser view
		content_browser_.reload_content();
		reinterpret_cast<content_browser_view*>(
			view_register_s::get_view_by_tag(ui_shared_vars::CONTENT_BROWSER_VIEW_TAG).get())->set_selected_directory_id(
				content_browser_.get_content().get_root_node().id
			);

		spdlog::info("Project Created");
		LUMINA_LOG_INFO("Project successfully created!");
	}

	void project_handler::save_project()
	{
		spdlog::warn("Saving project...");

		// Save the project settings
		save_project_settings();

		// Serialize all the assets in the project
		save_assets();

		// Serialize all the scenes in the project
		save_scenes();

		spdlog::info("Project saved...");
	}

	bool project_handler::load_project(const std::string& project_file)
	{
		// If a project is already loaded unloads it first
		if (has_opened_project())
			unload_project();
		else
		{
			destroy_scenes_context();
			destroy_assets();
		}

		// Tries first to deserialize project settings
		YAML::Node project_yaml;

		try
		{
			project_yaml = YAML::LoadFile(project_file);
		}
		catch (const std::exception&)
		{
			return false;
		}

		// Check if the file isn't good
		if (!project_yaml || !project_yaml["Project Name"] || !project_yaml["Scenes Path"])
			return false;

		// Load project metadata
		load_project_settings(project_yaml, project_file);

		// Set the content browser directory
		content_browser_.set_content_directory(loaded_project_->project_dir_path);

		// Tries to deserialize assets (assets must be deserialized first before scenes)
		load_assets();

		// Tries to deserialize scenes
		load_scenes();

		// As last passage force copy the latest scripting core assembly
		try
		{
			update_script_core_assemblies();
		}
		catch (const std::exception&)
		{
			LUMINA_LOG_ERROR("An error occured trying to update lumina script core assemblies.");
			spdlog::error("Error copying lumina script core assemblies in the project script directory");
			return false;
		}
		
		// Refresh the content browser view
		content_browser_.reload_content();
		reinterpret_cast<content_browser_view*>(
			view_register_s::get_view_by_tag(ui_shared_vars::CONTENT_BROWSER_VIEW_TAG).get())->set_selected_directory_id(
				content_browser_.get_content().get_root_node().id
			);

		// Automatically opens the scripting project solution
		open_script_editor();

		return true;
	}

	void project_handler::unload_project()
	{
		// Skip if there is not a loaded project
		if (!has_opened_project())
			return;

		// Save the project first
		save_project();

		// Cleanup the scene context
		destroy_scenes_context();

		// Destroy's all the assets that belongs to the project
		destroy_assets();

		// Unloads the project
		loaded_project_.reset();
		loaded_project_ = nullptr;
	}

	bool project_handler::build_project()
	{
		// Check if there is not an opened project
		if (!has_opened_project())
			return false;

		LUMINA_LOG_INFO("Build Started.");

		//@enhancement-[projects]: Implement more safety checks.
		//@enhancement-[projects]: Add an option for the user to clear the whole temp_build folder before building.
		const std::string build_path =
			loaded_project_->project_dir_path 
			+ "\\" 
			+ LUMINA_EDITOR_PROJECT_BUILD_DIR_PATH 
			+ "\\" 
			+ loaded_project_->build_info.build_production_name;

		const std::string content_path = 
			build_path + "\\" + LUMINA_EDITOR_PROJECT_BUILD_CONTENT_DIR;

		const std::string bin_path = 
			build_path + "\\" + LUMINA_EDITOR_PROJECT_BUILD_BIN_DIR;

		// Creates the temp build folder
		lumina::lumina_file_system_s::create_folder(
			loaded_project_->project_dir_path
			+ "\\"
			+ LUMINA_EDITOR_PROJECT_BUILD_DIR_PATH
		);

		// Creates the folder that will contain the project build
		lumina::lumina_file_system_s::create_folder(
			build_path
		);

		// Creates the folder that will contain the build content
		lumina::lumina_file_system_s::create_folder(
			content_path
		);

		// Creates the folder that will contain the build bin
		lumina::lumina_file_system_s::create_folder(
			bin_path
		);

		LUMINA_LOG_WARNING("Packing project settings...");

		// Save Runtime Player settings
		YAML::Emitter yaml_stream_emitter;
		yaml_stream_emitter << YAML::BeginMap;
		yaml_stream_emitter << YAML::Key << "Build Production Name";
		yaml_stream_emitter << YAML::Value << loaded_project_->build_info.build_production_name;
		yaml_stream_emitter << YAML::Key << "Runtime Player Name";
		yaml_stream_emitter << YAML::Value << loaded_project_->build_info.runtime_player_name;
		yaml_stream_emitter << YAML::Key << "Runtime Player Process Name";
		yaml_stream_emitter << YAML::Value << loaded_project_->build_info.runtime_player_process_name;
		yaml_stream_emitter << YAML::Key << "Runtime Player Window Name";
		yaml_stream_emitter << YAML::Value << loaded_project_->build_info.runtime_player_window_name;
		yaml_stream_emitter << YAML::EndMap;

		std::ofstream out_stream{ build_path + "\\" + LUMINA_EDITOR_RUNTIME_PLAYER_CONFIGS_DEFAULT_NAME };
		out_stream << yaml_stream_emitter.c_str();

		// Pack assets
		std::future<void> asset_serialization_promise = std::async(
			std::launch::async,
			[&]() -> void
			{
				LUMINA_LOG_WARNING("Packing assets...");
				lumina::assets_serializer::serialize_assets_package(
					lumina::asset_atlas::get_singleton().get_registry().get_registry(),
					content_path + "\\" + LUMINA_EDITOR_PROJECT_ASSET_PACKAGE_NAME
				);
				LUMINA_LOG_INFO("Assets packed.");
			}
		);

		// Pack scenes
		std::future<void> scenes_serialization_promise = std::async(
			std::launch::async,
			[&]() -> void 
			{
				LUMINA_LOG_WARNING("Packing Scenes...");
				lumina::scenes_system& scene_system = lumina::scenes_system::get_singleton();

				for (auto& scene : scene_system.get_all_scenes())
				{
					if (scene == nullptr)
						continue;

					lumina::scene_serializer::serialize_yaml(
						scene.get(),
						content_path + "\\" + scene->get_name() + ".scene"
					);
				}

				LUMINA_LOG_INFO("Scenes packed.");
			}
		);

		// Pack Script Assemblies
		std::future<bool> script_assemblies_packing_promise = std::async(
			std::launch::async,
			[&]() -> bool
			{
				LUMINA_LOG_WARNING("Packing Assemblies...");
				const std::string scripts_compiled_dir = 
					loaded_project_->project_dir_path 
					+ 
					"\\" + 
					LUMINA_EDITOR_PROJECT_SCRIPTS_DEFAULT_PATH + 
					LUMINA_EDITOR_COMPILED_SCRIPT_DIR_PATH;

				try
				{
					bool copy_result = std::filesystem::copy_file(
							scripts_compiled_dir + "\\" + LUMINA_EDITOR_SCRIPT_CORE_DEFAULT_NAME,
							bin_path + "\\" + LUMINA_EDITOR_SCRIPT_CORE_DEFAULT_NAME,
							std::filesystem::copy_options::overwrite_existing
					);

					copy_result = std::filesystem::copy_file(
						scripts_compiled_dir + "\\" + LUMINA_EDITOR_SCRIPT_COMPILED_DEFAULT_NAME,
						bin_path + "\\" + LUMINA_EDITOR_SCRIPT_COMPILED_DEFAULT_NAME,
						std::filesystem::copy_options::overwrite_existing
					);

					std::filesystem::copy(
						lumina::lumina_file_system_s::get_process_directory() + "\\" + LUMINA_EDITOR_PROJECT_MONO_BINS_DIR,
						build_path + "\\" + LUMINA_EDITOR_PROJECT_MONO_BINS_DIR,
						std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing
					);

					LUMINA_LOG_INFO("Assemblies packed.");

					return copy_result;
				}
				catch (std::exception& e)
				{
					LUMINA_LOG_ERROR("Error packing script assemblies!");
					spdlog::error((std::string)"Error packing script assemblies -> " + e.what());
					return false;
				}

				return false;
			}
		);

		// Wait for asset serialization to finish
		asset_serialization_promise.get();
		scenes_serialization_promise.get();
		if (!script_assemblies_packing_promise.get())
			return false;

		LUMINA_LOG_WARNING("Finishing build packaging...");

		// Copy the runtime_player distribution with the selected and correct platform and architecture build
		// NOTE that the file format should be changed based on the arch and platform build selected.
		std::filesystem::path sourceFile = 
			lumina::lumina_file_system_s::get_process_directory() 
			+ LUMINA_EDITOR_BIN_DIR_PATH 
			+ "\\" 
			+ LUMINA_EDITOR_RUNTIME_PLAYER_DEFAULT_NAME;
		
		std::filesystem::path targetParent = build_path;
		auto target = targetParent / std::filesystem::path(loaded_project_->build_info.runtime_player_name + ".exe");

		try 
		{
			bool copy_result = std::filesystem::copy_file(sourceFile, target, std::filesystem::copy_options::overwrite_existing);

			LUMINA_LOG_INFO("Project build files has been built in: " + build_path);
			LUMINA_LOG_INFO("Build has been successfully completed.");

			return true;
		}
		catch (std::exception& e) 
		{
			spdlog::error("Error packing runtime_player.");
		}

		return false;
	}

	void project_handler::launch_runtime_player()
	{
		const std::string build_path =
			loaded_project_->project_dir_path
			+ "\\"
			+ LUMINA_EDITOR_PROJECT_BUILD_DIR_PATH
			+ "\\"
			+ loaded_project_->build_info.build_production_name;

		lumina::lumina_file_system_s::execute_process(
			build_path + "\\" + loaded_project_->build_info.runtime_player_name + ".exe",
			build_path
		);
	}

	void project_handler::save_project_settings()
	{
		YAML::Emitter yaml_stream_emitter;
		yaml_stream_emitter << YAML::BeginMap;
		yaml_stream_emitter << YAML::Key << "Project Name";
		yaml_stream_emitter << YAML::Value << loaded_project_->name;
		yaml_stream_emitter << YAML::Key << "Scenes Path";
		yaml_stream_emitter << YAML::Value << loaded_project_->scenes_dir_path;
		
		// Build informations
		{
			yaml_stream_emitter << YAML::Key << "Build Info";
			yaml_stream_emitter << YAML::Value << YAML::BeginMap;
			yaml_stream_emitter << YAML::Key << "Build Production Name";
			yaml_stream_emitter << YAML::Value << loaded_project_->build_info.build_production_name;
			yaml_stream_emitter << YAML::Key << "Runtime Player Name";
			yaml_stream_emitter << YAML::Value << loaded_project_->build_info.runtime_player_name;
			yaml_stream_emitter << YAML::Key << "Runtime Player Process Name";
			yaml_stream_emitter << YAML::Value << loaded_project_->build_info.runtime_player_process_name;
			yaml_stream_emitter << YAML::Key << "Runtime Player Window Name";
			yaml_stream_emitter << YAML::Value << loaded_project_->build_info.runtime_player_window_name;
			yaml_stream_emitter << YAML::EndMap;
		}
		
		yaml_stream_emitter << YAML::EndMap;

		std::ofstream out_stream{ loaded_project_->project_dir_path + "\\" + loaded_project_->name + ".lmproj" };
		out_stream << yaml_stream_emitter.c_str();
	}

	void project_handler::save_scenes()
	{
		lumina::scenes_system& scene_system = lumina::scenes_system::get_singleton();

		for (auto& scene : scene_system.get_all_scenes())
		{
			if (scene == nullptr)
				continue;

			lumina::scene_serializer::serialize_yaml(
				scene.get(),
				loaded_project_->project_dir_path + "\\" + loaded_project_->scenes_dir_path + "\\" + scene->get_name() + ".scene"
			);
		}
	}

	void project_handler::save_assets()
	{
		lumina::assets_serializer::serialize_assets_bundle(
			lumina::asset_atlas::get_singleton().get_registry().get_registry(), 
			loaded_project_->project_dir_path + "\\" + loaded_project_->name + ".lmbundle"
		);
	}

	void project_handler::load_project_settings(const YAML::Node& project_node_yaml, const std::string& project_file)
	{
		loaded_project_ = std::make_shared<project>();
		loaded_project_->name = project_node_yaml["Project Name"].as<std::string>();
		loaded_project_->scenes_dir_path = project_node_yaml["Scenes Path"].as<std::string>();
		loaded_project_->project_dir_path = lumina::lumina_file_system_s::get_file_directory(project_file);
		loaded_project_->build_info.build_production_name = project_node_yaml["Build Info"]["Build Production Name"].as<std::string>();
		loaded_project_->build_info.runtime_player_name = project_node_yaml["Build Info"]["Runtime Player Name"].as<std::string>();
		loaded_project_->build_info.runtime_player_process_name = project_node_yaml["Build Info"]["Runtime Player Process Name"].as<std::string>();
		loaded_project_->build_info.runtime_player_window_name = project_node_yaml["Build Info"]["Runtime Player Window Name"].as<std::string>();
	}

	void project_handler::load_scenes()
	{
		std::vector<std::string> scene_files_path =
			lumina::lumina_file_system_s::get_files_in_directory(
				loaded_project_->project_dir_path + "\\" + loaded_project_->scenes_dir_path
			);

		// Log the save msg
		spdlog::info("Importing scenes...");

		for (auto& path : scene_files_path)
		{
			lumina::scenes_system::get_singleton().create_scene("__importing_scene__");
			lumina::scene_serializer::deserialize_yaml(
				lumina::scenes_system::get_singleton().get_scene("__importing_scene__"),
				path
			);
		}
	}

	void project_handler::load_assets()
	{
		lumina::assets_serializer::deserialize_assets_bundle(
			lumina::asset_atlas::get_singleton().get_registry(),
			loaded_project_->project_dir_path + "\\" + loaded_project_->name + ".lmbundle"
		);
	}

	void project_handler::destroy_scenes_context()
	{
		// Destroy's all the scenes
		lumina::scenes_system::get_singleton().destroy_all();

		// Destroy's all the editor views that belongs to scenes
		view_register_s::destroy_views("scene_view_type");
	}

	void project_handler::destroy_assets()
	{
		// Destroy's all the loaded assets
		lumina::asset_atlas::get_singleton().get_registry().clear();
	}

	void project_handler::generate_scripting_proj()
	{
		// Create's the folder that contains the scripting solution
		lumina::lumina_file_system_s::create_folder(loaded_project_->project_dir_path + "\\" + LUMINA_EDITOR_PROJECT_SCRIPTS_DEFAULT_PATH);

		// Fetch the current process working dir
		const std::string process_dir = lumina::lumina_file_system_s::get_process_directory();
		const std::string script_dir = loaded_project_->project_dir_path + "\\" + LUMINA_EDITOR_PROJECT_SCRIPTS_DEFAULT_PATH;

		// Copies the necessary files to generate the solution
		std::filesystem::path bin_dir =
			process_dir
			+ LUMINA_EDITOR_BIN_DIR_PATH;

		std::filesystem::path template_dir =
			process_dir
			+ LUMINA_EDITOR_TEMPLATE_DIR_PATH;

		try
		{
			// Copy generation files in the script folder
			bool copy_result = std::filesystem::copy_file(
				template_dir.string() + "\\" + LUMINA_EDITOR_GENERATION_BATCH_DEFAULT_NAME,
				std::filesystem::path(script_dir) /
				std::filesystem::path(LUMINA_EDITOR_GENERATION_BATCH_DEFAULT_NAME),
				std::filesystem::copy_options::overwrite_existing
			);

			copy_result = std::filesystem::copy_file(
				template_dir.string() + "\\" + LUMINA_EDITOR_SLN_STARTUP_BATCH_DEFAULT_NAME,
				std::filesystem::path(script_dir) /
				std::filesystem::path(LUMINA_EDITOR_SLN_STARTUP_BATCH_DEFAULT_NAME),
				std::filesystem::copy_options::overwrite_existing
			);

			copy_result = std::filesystem::copy_file(
				template_dir.string() + "\\" + LUMINA_EDITOR_GENERATION_SCRIPT_DEFAULT_NAME,
				std::filesystem::path(script_dir) /
				std::filesystem::path(LUMINA_EDITOR_GENERATION_SCRIPT_DEFAULT_NAME),
				std::filesystem::copy_options::overwrite_existing
			);

			copy_result = std::filesystem::copy_file(
				template_dir.string() + "\\" + LUMINA_EDITOR_APP_SCRIPT_DEFAULT_NAME,
				std::filesystem::path(script_dir) /
				std::filesystem::path(LUMINA_EDITOR_APP_SCRIPT_DEFAULT_NAME),
				std::filesystem::copy_options::overwrite_existing
			);

			// Copy binaries files needed for linking and generating
			copy_result = std::filesystem::copy_file(
				bin_dir.string() + "\\" + LUMINA_EDITOR_GENERATION_PREMAKE_DEFAULT_NAME,
				std::filesystem::path(loaded_project_->project_dir_path) /
				std::filesystem::path(LUMINA_EDITOR_GENERATION_PREMAKE_DEFAULT_NAME),
				std::filesystem::copy_options::overwrite_existing
			);

			copy_result = std::filesystem::copy_file(
				process_dir + LUMINA_EDITOR_SCRIPT_CORE_DIR_PATH + "\\" + LUMINA_EDITOR_SCRIPT_CORE_DEFAULT_NAME,
				std::filesystem::path(script_dir) /
				std::filesystem::path(LUMINA_EDITOR_SCRIPT_CORE_DEFAULT_NAME),
				std::filesystem::copy_options::overwrite_existing
			);

			// Execute project generation
			lumina::lumina_file_system_s::execute_process(
				script_dir + "\\" + LUMINA_EDITOR_GENERATION_BATCH_DEFAULT_NAME,
				script_dir
			);

			open_script_editor();
		}
		catch (std::exception& e)
		{
			spdlog::error("Error copying vitals dependecies during project creation.");
			LUMINA_LOG_ERROR("Error copying vitals dependecies during project creation.");
		}
	}

	void project_handler::open_script_editor()
	{
		const std::string script_dir = loaded_project_->project_dir_path + "\\" + LUMINA_EDITOR_PROJECT_SCRIPTS_DEFAULT_PATH;

		// Open up the project solution
		lumina::lumina_file_system_s::execute_process(
			script_dir + "\\" + LUMINA_EDITOR_SLN_STARTUP_BATCH_DEFAULT_NAME,
			script_dir
		);
	}

	void project_handler::update_script_core_assemblies()
	{
		std::filesystem::copy_file(
			lumina::lumina_file_system_s::get_process_directory() + LUMINA_EDITOR_SCRIPT_CORE_DIR_PATH + "\\" + LUMINA_EDITOR_SCRIPT_CORE_DEFAULT_NAME,
			std::filesystem::path(loaded_project_->project_dir_path + "\\" + LUMINA_EDITOR_PROJECT_SCRIPTS_DEFAULT_PATH) /
			std::filesystem::path(LUMINA_EDITOR_SCRIPT_CORE_DEFAULT_NAME),
			std::filesystem::copy_options::overwrite_existing
		);
	}
}