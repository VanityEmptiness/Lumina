#pragma once

#include "core/lumina_singleton.h"

#include <d3d11.h>
#include <tchar.h>

namespace lumina
{
	// D3d11 api graphic abstraction (holds the instance of device, devicecontext, swapchain, etc..) (singleton)
	class LUMINA_SINGLETON_CLASS(d3d11_instance)
	{
	public:

		inline ID3D11Device* get_device() { return device_; }
		inline ID3D11DeviceContext* get_device_context() { return device_context_; }
		inline IDXGISwapChain* get_swap_chain() { return swap_chain_; }
		inline ID3D11RenderTargetView* get_main_render_target_view() { return main_render_target_view_; }

	private:
		
		ID3D11Device* device_ = nullptr;
		ID3D11DeviceContext* device_context_ = nullptr;
		IDXGISwapChain* swap_chain_ = nullptr;
		ID3D11RenderTargetView* main_render_target_view_ = nullptr;

		bool create_device(HWND hwnd);
		void cleanup_device();
		void create_render_target();
		void cleanup_render_target();

	private:

		friend class d3d11_renderer;

	};
}