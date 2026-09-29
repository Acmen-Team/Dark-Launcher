#include "Platform/Paths.h"

#include <Windows.h>

#include <vector>

namespace Dark::Launcher::Paths
{
	namespace
	{
		std::filesystem::path ComputeExecutableDirectory()
		{
			std::vector<wchar_t> buffer(MAX_PATH);
			for (;;)
			{
				const DWORD written = ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
				if (written == 0)
					return std::filesystem::current_path();
				if (static_cast<std::size_t>(written) + 1 < buffer.size())
					break;
				buffer.resize(buffer.size() * 2);
			}
			return std::filesystem::path(buffer.data()).parent_path();
		}

		const std::filesystem::path& ExeDir()
		{
			static const std::filesystem::path dir = ComputeExecutableDirectory();
			return dir;
		}

		const std::filesystem::path& WorkingDir()
		{
			static const std::filesystem::path dir = std::filesystem::current_path();
			return dir;
		}

		std::filesystem::path FirstExisting(const std::vector<std::filesystem::path>& candidates)
		{
			std::error_code ec;
			for (const std::filesystem::path& candidate : candidates)
			{
				if (candidate.empty())
					continue;
				if (std::filesystem::is_regular_file(candidate, ec))
					return candidate;
			}
			return {};
		}
	}

	const std::filesystem::path& ExecutableDirectory()
	{
		return ExeDir();
	}

	std::filesystem::path ResolveAsset(const std::filesystem::path& relative)
	{
		return FirstExisting({
			ExeDir() / relative,
			WorkingDir() / relative,
			WorkingDir() / "Launcher" / relative,
			WorkingDir() / "Dark-Editor" / relative,
			WorkingDir() / ".." / "Dark-Editor" / relative,
		});
	}

	std::filesystem::path ResolveUiFont()
	{
		std::filesystem::path font = ResolveAsset("Content/Fonts/Poppins-Medium.ttf");
		if (!font.empty())
			return font;

		return FirstExisting({
			"C:/Windows/Fonts/segoeui.ttf",
			"C:/Windows/Fonts/tahoma.ttf",
			"C:/Windows/Fonts/arial.ttf",
		});
	}

	std::filesystem::path ResolveIconFont()
	{
		return FirstExisting({
			ResolveAsset("Content/Fonts/fa-solid-900.ttf"),
			ResolveAsset("Content/Fonts/fa-solid-900-v5.ttf"),
		});
	}

	std::filesystem::path ResolveCjkFont()
	{
		return FirstExisting({
			ResolveAsset("Content/Fonts/NotoSansSC-Regular.ttf"),
			"C:/Windows/Fonts/msyh.ttc",
			"C:/Windows/Fonts/msyhl.ttc",
			"C:/Windows/Fonts/simhei.ttf",
			"C:/Windows/Fonts/msjh.ttc",
		});
	}
}
