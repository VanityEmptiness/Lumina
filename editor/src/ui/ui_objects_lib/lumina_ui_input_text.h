#pragma once

#include <string>

namespace lumina_editor
{
	// An abstracted ImGui input text used to manage input text easily
	class lumina_ui_input_text
	{
	public:

		void bind_text_buffer(std::string* text_buffer);
		void render(const std::string& label);
		void clear_buffer() { *text_buffer_ = ""; strcpy(input_holder_buffer_, ""); }
		void update_buffer(const std::string& str) { *text_buffer_ = str; strcpy(input_holder_buffer_, str.c_str()); }

	private:

		static constexpr const uint32_t MAX_CHARACTERS = 256;
		char input_holder_buffer_[MAX_CHARACTERS] = "";
		std::string* text_buffer_ = nullptr;

	};
}