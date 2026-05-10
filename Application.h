#pragma once

#include <string>
#include <memory>
#include "Game/Game.h"

struct ApplicationSpecification
{
	int ScreenWidth = 1440;
	int ScreenHeight = 900;
	std::string Title = "Memory Packer";
};

class Application
{
public:
	Application(const ApplicationSpecification& spec = ApplicationSpecification());
	~Application();

	void Run();
	void Update();
	void Render();

	static Application& Get();
private:
	void Loop();
private:
	ApplicationSpecification m_Specification;
	std::unique_ptr<Game> m_Game;
};