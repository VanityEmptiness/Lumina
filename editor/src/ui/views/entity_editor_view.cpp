#include "entity_editor_view.h"

#include "scene/editor_camera.h"
#include "ui/colors.h"
#include "ui/ui_objects_lib/lumina_ui_drag_objects.h"

#include "ImGui/imgui.h"
#include "ImGuizmo/ImGuizmo.h"

namespace lumina_editor
{
	entity_editor_view::entity_editor_view(lumina::entity entity, lumina::scene* scene)
		: entity_{ entity }, scene_ { scene }
	{}

	void entity_editor_view::on_create()
	{
		// Bind the entity name to a text input buffer
		view_instance_data_.entity_name_input_text.bind_text_buffer(
			&entity_.get_component<lumina::identity_component>().name
		);
	}

	void entity_editor_view::on_render()
	{
		ImGui::Begin("Entity Editor");

		// Render the popup menu
		render_entity_popup_menu();

		// Render components if needed (IN ORDER)
		render_identifier_component();
		render_transform_component();
		render_sprite_component();
		render_camera_component();

		ImGui::End();
	}

	void entity_editor_view::render_entity_popup_menu()
	{
		// Edit Scene popup menu (opened on right mouse button click
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			ImGui::OpenPopup("entity_edit_menu_popup");

		if (ImGui::BeginPopupContextWindow("entity_edit_menu_popup"))
		{
			if (ImGui::SmallButton("Add Transform"))
			{
				entity_.add_component<lumina::transform_component>();
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::SmallButton("Add Sprite"))
			{
				entity_.add_component<lumina::sprite_component>();
				ImGui::CloseCurrentPopup();
			}

			if (ImGui::SmallButton("Add Camera"))
			{
				entity_.add_component<lumina::camera_component>();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}

	void entity_editor_view::render_identifier_component()
	{
		ImGui::SeparatorText("Identifier");
		ImGui::PushStyleColor(ImGuiCol_Separator, { 0.35f, 0.35f, 0.35f, 1.0f });
		ImGui::Separator();
		ImGui::PopStyleColor();

		view_instance_data_.entity_name_input_text.render("Name");
	}

	void entity_editor_view::render_transform_component()
	{
		// Check if the entity has not the component, to avoid rendering or crashing
		if (!entity_.has_component<lumina::transform_component>())
			return;

		ImGui::SeparatorText("Transform");
		ImGui::PushStyleColor(ImGuiCol_Separator, { 0.35f, 0.35f, 0.35f, 1.0f });
		ImGui::Separator();
		ImGui::PopStyleColor();

		glm::vec3 position_ptr;
		glm::vec3 rotation_ptr;
		glm::vec3 scale_ptr;

		ImGuizmo::DecomposeMatrixToComponents(
			(const float*)&entity_.get_component<lumina::transform_component>().get_model_matrix(),
			(float*)&position_ptr,
			(float*)&rotation_ptr,
			(float*)&scale_ptr
			);

		// Position 
		lumina_ui_drag_objects::drag_float_3_multi_text_colored(
			"Position",
			editor_ui_colors::RED,
			editor_ui_colors::GREEN,
			editor_ui_colors::BLUE,
			(float*)&position_ptr,
			0.01f
		);

		// Rotation 
		lumina_ui_drag_objects::drag_float_3_multi_text_colored(
			"Rotation",
			editor_ui_colors::RED,
			editor_ui_colors::GREEN,
			editor_ui_colors::BLUE,
			(float*)&rotation_ptr,
			0.01f
		);

		// Scale 
		lumina_ui_drag_objects::drag_float_3_multi_text_colored(
			"Scale",
			editor_ui_colors::RED,
			editor_ui_colors::GREEN,
			editor_ui_colors::BLUE,
			(float*)&scale_ptr,
			0.01f
		);

		ImGuizmo::RecomposeMatrixFromComponents(
			(const float*)&position_ptr,
			(const float*)&rotation_ptr,
			(const float*)&scale_ptr,
			(float*)&entity_.get_component<lumina::transform_component>().get_model_matrix()
		);

		// Render Object Gizmos
		ImGuizmo::Manipulate(
			(const float*)&editor_camera::get_singleton().get_camera()->get_view_matrix(),
			(const float*)&editor_camera::get_singleton().get_camera()->get_projection_matrix(),
			ImGuizmo::OPERATION::TRANSLATE,
			ImGuizmo::MODE::WORLD,
			(float*)&entity_.get_component<lumina::transform_component>().get_model_matrix()
		);
	}

	void entity_editor_view::render_sprite_component()
	{
		// Check if the entity has not the component, to avoid rendering or crashing
		if (!entity_.has_component<lumina::sprite_component>())
			return;

		ImGui::SeparatorText("Sprite");
		ImGui::PushStyleColor(ImGuiCol_Separator, { 0.35f, 0.35f, 0.35f, 1.0f });
		ImGui::Separator();
		ImGui::PopStyleColor();

		// Color edit 
		ImGui::ColorEdit4("Color", (float*)&entity_.get_component<lumina::sprite_component>().color);
	}

	void entity_editor_view::render_camera_component()
	{
		// Check if the entity has not the component, to avoid rendering or crashing
		if (!entity_.has_component<lumina::camera_component>())
			return;

		ImGui::SeparatorText("Camera");
		ImGui::PushStyleColor(ImGuiCol_Separator, { 0.35f, 0.35f, 0.35f, 1.0f });
		ImGui::Separator();
		ImGui::PopStyleColor();

		lumina::camera_component& camera = entity_.get_component<lumina::camera_component>();

		// Set active camera button
		if (scene_->is_active_camera(&camera))
			ImGui::TextColored(editor_ui_colors::GREEN, "Active");
		else if(ImGui::SmallButton("Set Active"))
			scene_->set_camera(&camera);
	}
}