#include "Application.h"
#include "Core/GameConfig.h"

int main()
{
	ApplicationSpecification appSpec;
	appSpec.Title = "Memory Packer - Weighted Set Cover Visualizer";
	appSpec.ScreenWidth = Config::SCREEN_WIDTH;
	appSpec.ScreenHeight = Config::SCREEN_HEIGHT;

	Application app(appSpec);
	app.Run();
}