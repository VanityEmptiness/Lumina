#include "d3d11_vertex_buffer.h"

#include "spdlog/spdlog.h"

namespace lumina
{
	bool d3d11_vertex_buffer::allocate(const d3d11_vertex_buffer_alloc_info_t& alloc_info)
	{
		// Check if the buffer is already allocated
		if (is_allocated())
			return false;

		topology_ = alloc_info.topology;
		vertex_stride_ = alloc_info.data_stride;
		vertex_buffer_size_ = alloc_info.buffer_size;

		// @enhancement-[Graphics]: The buffer usage init flag should be changed to DEFAULT for speed improvement and a staging buffer should be prefered to upload data
		D3D11_BUFFER_DESC buffer_description;
		ZeroMemory(&buffer_description, sizeof(buffer_description));
		buffer_description.Usage = D3D11_USAGE_DYNAMIC;
		buffer_description.ByteWidth = alloc_info.buffer_size;
		buffer_description.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer_description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		
		if (FAILED(
			d3d11_instance::get_singleton().get_device()->CreateBuffer(
				&buffer_description, 
				nullptr, 
				&vertex_buffer_
			)
		))
		{
			spdlog::error("D3D11 Vertex Buffer -> Error allocating vertex buffer.");
			return false;
		}

		if(alloc_info.data_to_load != nullptr)
			load_data(alloc_info.data_to_load, alloc_info.data_to_load_size);

		return is_allocated();
	}

	void d3d11_vertex_buffer::load_data(const void* data, uint32_t size) const 
	{
		if (!is_allocated())
		{
			spdlog::error("D3D11 Vertex Buffer -> Trying to update data to a non allocated buffer");
			return;
		}

		D3D11_MAPPED_SUBRESOURCE ms;
		d3d11_instance::get_singleton().get_device_context()->Map(vertex_buffer_, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
		memcpy(ms.pData, data, size);
		d3d11_instance::get_singleton().get_device_context()->Unmap(vertex_buffer_, NULL);
	}

	void d3d11_vertex_buffer::enable() const 
	{
		if (!is_allocated())
		{
			spdlog::error("D3D11 Vertex Buffer -> Trying to enable a vertex buffer to a non allocated buffer");
			return;
		}

		if (vertex_stride_ == 0)
		{
			spdlog::error("D3D11 Vertex Buffer -> Trying to enable a vertex buffer with a null stride, did you forget to specify the stride?");
			return;
		}

		uint32_t offset = 0;

		d3d11_instance::get_singleton().get_device_context()->IASetVertexBuffers(0, 1, &vertex_buffer_, &vertex_stride_, &offset);
		d3d11_instance::get_singleton().get_device_context()->IASetPrimitiveTopology(topology_);
	}
}