#include "Application.h"

static Application* s_Application = nullptr;

Application::Application(const ApplicationSpecification& spec)
	: m_Specification(spec)
{
	s_Application = this;
}

Application::~Application()
{
	s_Application = nullptr;
}

void Application::Run()
{
	InitWindow(m_Specification.ScreenWidth, m_Specification.ScreenHeight, m_Specification.Title.c_str());
	SetTargetFPS(60);

	m_Game = std::make_unique<Game>();

	while (!WindowShouldClose())
	{
		Loop();
	}

	m_Game.reset();
	CloseWindow();
}

void Application::Update()
{
	float delta = GetFrameTime();
	m_Game->Update(delta);
}

void Application::Render()
{
	BeginDrawing();
	{
		ClearBackground(Color{ 12, 12, 20, 255 });
		m_Game->Render();
	}
	EndDrawing();
}

Application& Application::Get()
{
	return *s_Application;
}

void Application::Loop()
{
	Update();
	Render();
}
