#pragma once

#include <map>
#include <format>

namespace Renderer
{
	void RenderText(const char* string, int x, int y, int size, Color color);
	void RenderText(const std::string& string, int x, int y, int size, Color color);

}