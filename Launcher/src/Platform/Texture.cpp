#include "Platform/Texture.h"

#include <wincodec.h>

#include <vector>

namespace Dark::Launcher
{
	namespace
	{
		template <typename T>
		void SafeRelease(T*& object)
		{
			if (object)
			{
				object->Release();
				object = nullptr;
			}
		}
	}

	bool LoadTextureFromPng(ID3D11Device* device, const std::filesystem::path& path, Texture& texture)
	{
		ReleaseTexture(texture);

		if (!device || path.empty())
			return false;

		// WIC 需要 COM。RPC_E_CHANGED_MODE 表示本线程已经用别的模式初始化过，
		// 那种情况下 WIC 依然可用，所以不算失败。
		const HRESULT comResult = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE)
			return false;

		IWICImagingFactory*     factory = nullptr;
		IWICBitmapDecoder*      decoder = nullptr;
		IWICBitmapFrameDecode*  frame = nullptr;
		IWICFormatConverter*    converter = nullptr;
		ID3D11Texture2D*        gpuTexture = nullptr;
		bool succeeded = false;

		do
		{
			if (FAILED(::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&factory))))
				break;

			if (FAILED(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
				WICDecodeMetadataCacheOnLoad, &decoder)))
				break;

			if (FAILED(decoder->GetFrame(0, &frame)))
				break;

			UINT width = 0;
			UINT height = 0;
			if (FAILED(frame->GetSize(&width, &height)) || width == 0 || height == 0)
				break;

			if (FAILED(factory->CreateFormatConverter(&converter)))
				break;

			// 统一转成 32bpp RGBA：通道顺序与 DXGI_FORMAT_R8G8B8A8_UNORM 一致，
			// 且保持直通 alpha（ImGui 的混合状态用的就是直通 alpha）。
			if (FAILED(converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
				WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)))
				break;

			const UINT stride = width * 4;
			std::vector<unsigned char> pixels(static_cast<std::size_t>(stride) * height);
			if (FAILED(converter->CopyPixels(nullptr, stride,
				static_cast<UINT>(pixels.size()), pixels.data())))
				break;

			D3D11_TEXTURE2D_DESC description = {};
			description.Width = width;
			description.Height = height;
			description.MipLevels = 1;
			description.ArraySize = 1;
			description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			description.SampleDesc.Count = 1;
			description.Usage = D3D11_USAGE_DEFAULT;
			description.BindFlags = D3D11_BIND_SHADER_RESOURCE;

			D3D11_SUBRESOURCE_DATA initial = {};
			initial.pSysMem = pixels.data();
			initial.SysMemPitch = stride;

			if (FAILED(device->CreateTexture2D(&description, &initial, &gpuTexture)))
				break;

			D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription = {};
			viewDescription.Format = description.Format;
			viewDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			viewDescription.Texture2D.MipLevels = 1;

			if (FAILED(device->CreateShaderResourceView(gpuTexture, &viewDescription, &texture.View)))
				break;

			texture.Width = static_cast<int>(width);
			texture.Height = static_cast<int>(height);
			succeeded = true;
		}
		while (false);

		SafeRelease(gpuTexture);
		SafeRelease(converter);
		SafeRelease(frame);
		SafeRelease(decoder);
		SafeRelease(factory);

		if (!succeeded)
			ReleaseTexture(texture);

		return succeeded;
	}

	void ReleaseTexture(Texture& texture)
	{
		SafeRelease(texture.View);
		texture.Width = 0;
		texture.Height = 0;
	}
}
