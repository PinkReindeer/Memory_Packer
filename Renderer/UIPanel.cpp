#include <cstdio>
#include <cmath>

#include "UIPanel.h"
#include "Renderer.h"

static const char* AlgoNames[] = {
    "Greedy", "Backtracking", "Branch & Bound", "Genetic Algorithm"
};

void UIPanel::Init(int x, int y, int w, int h)
{
    m_X = x; m_Y = y; m_W = w; m_H = h;
}

bool UIPanel::DrawButton(int x, int y, int w, int h, const char* text, Color bgColor, Color textColor)
{
    Rectangle rect = { (float)x, (float)y, (float)w, (float)h };
    bool hovered = CheckCollisionPointRec(GetMousePosition(), rect);
    bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    Color bg = hovered ? Color{ (unsigned char)std::min(255, bgColor.r + 30),
                                (unsigned char)std::min(255, bgColor.g + 30),
                                (unsigned char)std::min(255, bgColor.b + 30),
                                bgColor.a }
                       : bgColor;
    DrawRectangleRounded(rect, 0.3f, 6, bg);
    DrawRectangleRoundedLines(rect, 0.3f, 6, Color{ 80, 90, 120, 200 });

    int tw = MeasureText(text, 14);
    Renderer::RenderText(text, x + (w - tw) / 2, y + (h - 14) / 2, 19, textColor);
    return clicked;
}

void UIPanel::DrawSlider(int x, int y, int w, const char* label, int& value, int minVal, int maxVal)
{
    Renderer::RenderText(label, x, y, 17, Color{ 160, 170, 200, 255 });

    int sliderY = y + 18;
    int sliderH = 8;
    float ratio = (float)(value - minVal) / (float)(maxVal - minVal);
    int handleX = x + (int)(ratio * w);

    // Track
    DrawRectangleRounded({ (float)x, (float)sliderY, (float)w, (float)sliderH }, 0.5f, 4, Color{ 40, 45, 60, 255 });
    // Filled portion
    DrawRectangleRounded({ (float)x, (float)sliderY, (float)(handleX - x), (float)sliderH }, 0.5f, 4, Color{ 0, 180, 220, 255 });
    // Handle
    DrawCircleV({ (float)handleX, (float)(sliderY + sliderH / 2) }, 8, Color{ 0, 220, 255, 255 });

    // Interaction
    Rectangle hitArea = { (float)(x - 10), (float)(sliderY - 10), (float)(w + 20), (float)(sliderH + 20) };
    if (CheckCollisionPointRec(GetMousePosition(), hitArea) && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        float mx = GetMousePosition().x;
        float r = (mx - x) / (float)w;
        r = std::max(0.0f, std::min(1.0f, r));
        value = minVal + (int)(r * (maxVal - minVal));
    }

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", value);
    Renderer::RenderText(buf, x + w + 8, sliderY - 2, 17, Color{ 200, 220, 255, 255 });
}

void UIPanel::DrawLabel(int x, int y, const char* text, int fontSize, Color color)
{
    Renderer::RenderText(text, x, y, fontSize + 5, color);
}

UIPanelOutput UIPanel::Draw(const GridState& grid, const ScannerDB& db, const PlacementState& placements, const AlgorithmBase* algorithm,
                            GameMode currentMode, bool algoPaused, int currentSelectedScanner)
{
    UIPanelOutput out;
    out.stepsPerSec = m_StepsPerSec;
    out.algoType = (AlgorithmType)m_SelectedAlgo;

    // Panel background
    DrawRectangle(m_X, m_Y, m_W, m_H, Color{ 22, 22, 35, 255 });
    DrawLineV({ (float)m_X, (float)m_Y }, { (float)m_X, (float)(m_Y + m_H) }, Color{ 50, 55, 80, 255 });

    int px = m_X + 16;
    int py = m_Y + 16;

    // --- Title ---
    Renderer::RenderText("MEMORY PACKER", px, py, 30, Color{ 0, 220, 255, 255 });
    py += 36;
    DrawLineEx({ (float)px, (float)py }, { (float)(m_X + m_W - 16), (float)py }, 1.0f, Color{ 50, 60, 90, 255 });
    py += 12;

    // --- Metrics ---
    char buf[128];
    int covered = placements.CoveredNodeCount(grid);
    std::snprintf(buf, sizeof(buf), "Nodes: %d / %d", covered, grid.nodeCount);
    DrawLabel(px, py, buf, 16, covered == grid.nodeCount ? Color{ 80, 255, 120, 255 } : Color{ 255, 200, 100, 255 });
    py += 22;

    // Coverage bar
    float covRatio = grid.nodeCount > 0 ? (float)covered / grid.nodeCount : 0.0f;
    DrawRectangleRounded({ (float)px, (float)py, (float)(m_W - 32), 10.0f }, 0.5f, 4, Color{ 40, 45, 60, 255 });
    DrawRectangleRounded({ (float)px, (float)py, (float)((m_W - 32) * covRatio), 10.0f }, 0.5f, 4,
        covRatio >= 1.0f ? Color{ 80, 255, 120, 200 } : Color{ 0, 200, 255, 200 });
    py += 18;

    std::snprintf(buf, sizeof(buf), "Total Weight: %d", placements.totalWeight);
    DrawLabel(px, py, buf, 14, Color{ 200, 180, 255, 255 });
    py += 20;

    if (algorithm && algorithm->GetStatus() != AlgoStatus::Idle)
    {
        std::snprintf(buf, sizeof(buf), "Steps: %d  |  %.1f ms", algorithm->GetSteps(), algorithm->GetResult().elapsedMs);
        DrawLabel(px, py, buf, 12, Color{ 150, 160, 180, 255 });
    }
    py += 20;

    DrawLineEx({ (float)px, (float)py }, { (float)(m_X + m_W - 16), (float)py }, 1.0f, Color{ 50, 60, 90, 255 });
    py += 12;

    // --- Mode Toggle ---
    DrawLabel(px, py, "MODE", 12, Color{ 120, 130, 160, 255 });
    py += 18;

    int halfW = (m_W - 48) / 2;
    Color manualBg = currentMode == GameMode::Manual ? Color{ 0, 160, 200, 255 } : Color{ 40, 45, 60, 255 };
    Color vizBg = currentMode == GameMode::Visualizer ? Color{ 0, 160, 200, 255 } : Color{ 40, 45, 60, 255 };

    if (DrawButton(px, py, halfW, 28, "Manual", manualBg, Color{ 230, 240, 255, 255 }))
    {
        out.modeChanged = true;
        out.newMode = GameMode::Manual;
    }
    if (DrawButton(px + halfW + 8, py, halfW, 28, "Visualizer", vizBg, Color{ 230, 240, 255, 255 }))
    {
        out.modeChanged = true;
        out.newMode = GameMode::Visualizer;
    }
    py += 40;

    // --- Mode-specific controls ---
    if (currentMode == GameMode::Manual)
    {
        DrawLabel(px, py, "SCANNERS", 12, Color{ 120, 130, 160, 255 });
        py += 18;

        for (int i = 0; i < db.typeCount; ++i)
        {
            Color bg = (i == currentSelectedScanner) ? Color{ db.colors[i].r, db.colors[i].g, db.colors[i].b, 180 } : Color{ 35, 38, 55, 255 };

            std::snprintf(buf, sizeof(buf), "%s (W:%d)", db.names[i], db.weights[i]);
            if (DrawButton(px, py, m_W - 32, 26, buf, bg, Color{ 220, 230, 250, 255 }))
            {
                out.selectedScanner = i;
            }
            py += 30;
        }

        py += 4;
        if (DrawButton(px, py, m_W - 32, 28, "Clear All", Color{ 180, 50, 50, 255 }, Color{ 255, 220, 220, 255 }))
        {
            out.clearPlacements = true;
        }
        py += 36;
    }
    else
    {
        // Visualizer mode
        DrawLabel(px, py, "ALGORITHM", 12, Color{ 120, 130, 160, 255 });
        py += 18;

        for (int i = 0; i < (int)AlgorithmType::COUNT; ++i)
        {
            Color bg = (i == m_SelectedAlgo) ? Color{ 0, 140, 180, 255 } : Color{ 35, 38, 55, 255 };
            if (DrawButton(px, py, m_W - 32, 24, AlgoNames[i], bg, Color{ 220, 230, 250, 255 }))
            {
                m_SelectedAlgo = i;
                out.algoType = (AlgorithmType)i;
            }
            py += 28;
        }
        py += 8;

        DrawSlider(px, py, m_W - 64, "Speed (steps/sec)", m_StepsPerSec, Config::MIN_STEPS_PER_SEC, Config::MAX_STEPS_PER_SEC);
        out.stepsPerSec = m_StepsPerSec;
        py += 44;

        // Control buttons
        int btnW = (m_W - 52) / 3;
        bool algoRunning = algorithm && algorithm->GetStatus() == AlgoStatus::Running;

        if (DrawButton(px, py, btnW, 28, algoRunning ? "Running..." : "Start", algoRunning ? Color{ 40, 100, 60, 255 } : Color{ 30, 150, 80, 255 }, Color{ 220, 255, 220, 255 }))
        {
            if (!algoRunning) out.startAlgorithm = true;
        }

        const char* pauseText = algoPaused ? "Resume" : "Pause";
        if (DrawButton(px + btnW + 8, py, btnW, 28, pauseText, Color{ 180, 150, 30, 255 }, Color{ 255, 255, 220, 255 }))
        {
            out.pauseResume = true;
        }

        if (DrawButton(px + (btnW + 8) * 2, py, btnW, 28, "Reset", Color{ 160, 50, 50, 255 }, Color{ 255, 220, 220, 255 }))
        {
            out.resetAlgorithm = true;
        }
        py += 40;
    }

    DrawLineEx({ (float)px, (float)py }, { (float)(m_X + m_W - 16), (float)py }, 1.0f, Color{ 50, 60, 90, 255 });
    py += 8;

    // --- Grid Settings ---
    DrawLabel(px, py, "GRID SETTINGS", 12, Color{ 120, 130, 160, 255 });
    py += 18;

    DrawSlider(px, py, m_W - 64, "Rows", m_GridRows, 4, 20);
    py += 40;
    DrawSlider(px, py, m_W - 64, "Cols", m_GridCols, 4, 20);
    py += 40;
    DrawSlider(px, py, m_W - 64, "Node Density %", m_NodeDensity, 10, 80);
    py += 40;

    if (DrawButton(px, py, m_W - 32, 28, "Regenerate Grid", Color{ 80, 60, 160, 255 }, Color{ 220, 210, 255, 255 }))
    {
        out.regenerateGrid = true;
        out.gridRows = m_GridRows;
        out.gridCols = m_GridCols;
        out.nodeDensity = m_NodeDensity;
    }
    py += 40;

    DrawLineEx({ (float)px, (float)py }, { (float)(m_X + m_W - 16), (float)py }, 1.0f, Color{ 50, 60, 90, 255 });
    py += 8;

    // --- Comparison Chart ---
    DrawLabel(px, py, "ALGORITHM COMPARISON", 12, Color{ 120, 130, 160, 255 });
    py += 16;

    int chartH = std::max(80, m_Y + m_H - py - 16);
    m_Chart.Draw(px, py, m_W - 32, chartH);

    return out;
}
