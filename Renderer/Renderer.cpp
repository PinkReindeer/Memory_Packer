#include "Renderer.h"

static constexpr const char* FontPath = "Assets/pixel-game.regular.otf";

namespace Renderer
{
	static std::map<int, Font> s_Font;

	void RenderText(const char* string, int x, int y, int size, Color color)
	{
		Font* font = nullptr;

		auto& fonts = s_Font;
		if (!fonts.contains(size))
			fonts[size] = LoadFontEx(FontPath, size, nullptr, 0);

		font = &fonts.at(size);

		DrawTextEx(*font, string, { (float)x, (float)y }, (float)size, 2.0f, color);
	}

	void RenderText(const std::string& string, int x, int y, int size, Color color)
	{
		RenderText(string.c_str(), x, y, size, color);
	}

}