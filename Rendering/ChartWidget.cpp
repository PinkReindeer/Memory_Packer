#include <algorithm>
#include <cstdio>
#include <cmath>

#include "ChartWidget.h"

void ChartWidget::Draw(int x, int y, int w, int h) const
{
    if (m_Entries.empty())
    {
        DrawText("No results yet", x + 10, y + h / 2 - 8, 14, Color{ 100, 100, 120, 255 });
        return;
    }

    // Find max values for scaling
    float maxCost = 1.0f;
    float maxTime = 1.0f;
    for (const auto& e : m_Entries)
    {
        if (e.cost > maxCost)   maxCost = e.cost;
        if (e.timeMs > maxTime) maxTime = e.timeMs;
    }

    int n = (int)m_Entries.size();
    float barGroupW = (float)w / (n + 1);
    float chartH = (float)(h - 50);  // leave room for labels
    int chartY = y + 10;

    // Draw axis
    DrawLineV({ (float)x, (float)(chartY + chartH) }, { (float)(x + w), (float)(chartY + chartH) }, Color{ 60, 65, 80, 255 });

    for (int i = 0; i < n; ++i)
    {
        const auto& e = m_Entries[i];
        float bx = x + barGroupW * (i + 0.5f);
        float barW = barGroupW * 0.35f;

        // Cost bar (cyan)
        float costH = (e.cost / maxCost) * chartH * 0.9f;
        Color costColor = e.foundSolution ? Color{ 0, 200, 220, 200 } : Color{ 200, 60, 60, 200 };
        DrawRectangleV( { bx - barW, (float)chartY + chartH - costH }, { barW, costH }, costColor);

        // Time bar (orange)
        float timeH = (e.timeMs / maxTime) * chartH * 0.9f;
        DrawRectangleV( { bx, (float)chartY + chartH - timeH }, { barW, timeH }, Color{ 255, 160, 50, 200 });

        // Cost label
        char buf[32];
        std::snprintf(buf, sizeof(buf), "W:%d", (int)e.cost);
        DrawText(buf, (int)(bx - barW), (int)(chartY + chartH - costH - 14), 10, Color{ 150, 220, 255, 255 });

        // Time label
        std::snprintf(buf, sizeof(buf), "%.0fms", e.timeMs);
        DrawText(buf, (int)bx, (int)(chartY + chartH - timeH - 14), 10, Color{ 255, 200, 100, 255 });

        // Algorithm name
        int nameW = MeasureText(e.name.c_str(), 10);
        DrawText(e.name.c_str(), (int)(bx - nameW / 2.0f + barW * 0.5f), (int)(chartY + chartH + 4), 10, Color{ 180, 180, 200, 255 });
    }

    // Legend
    DrawRectangleV({ (float)(x + w - 90), (float)(chartY) }, { 8, 8 }, Color{ 0, 200, 220, 200 });
    DrawText("Cost", x + w - 78, chartY - 1, 10, Color{ 150, 200, 220, 255 });

    DrawRectangleV({ (float)(x + w - 90), (float)(chartY + 14) }, { 8, 8 }, Color{ 255, 160, 50, 200 });
    DrawText("Time", x + w - 78, chartY + 13, 10, Color{ 255, 200, 130, 255 });
}
