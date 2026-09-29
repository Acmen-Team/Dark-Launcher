#pragma once

#include <Windows.h>

#include <d3d11.h>

#include <filesystem>

namespace Dark::Launcher
{
	// 一张 GPU 贴图。目前只用于界面里的品牌图标（Dark logo）。
	struct Texture
	{
		ID3D11ShaderResourceView* View = nullptr;
		int Width = 0;
		int Height = 0;

		bool Valid() const { return View != nullptr; }
	};

	// 从 PNG 文件加载贴图。解码走 Windows 自带的 WIC，不引入第三方解码库。
	// 失败时会把 texture 置空并返回 false。
	bool LoadTextureFromPng(ID3D11Device* device, const std::filesystem::path& path, Texture& texture);

	void ReleaseTexture(Texture& texture);
}
