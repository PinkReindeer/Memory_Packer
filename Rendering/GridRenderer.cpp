#include "GridRenderer.h"
#include <algorithm>
#include <cstring>

void GridRenderer::Init(int areaX, int areaY, int areaW, int areaH)
{
    m_AreaX = areaX;
    m_AreaY = areaY;
    m_AreaW = areaW;
    m_AreaH = areaH;
}

float GridRenderer::GetCellSize(const GridState& grid) const
{
    float usableW = (float)(m_AreaW - Config::GRID_PADDING * 2);
    float usableH = (float)(m_AreaH - Config::GRID_PADDING * 2);
    float cellW = usableW / grid.cols;
    float cellH = usableH / grid.rows;
    return std::min(cellW, cellH);
}

Vector2 GridRenderer::GridToScreen(int row, int col, const GridState& grid) const
{
    float cellSize = GetCellSize(grid);
    float gridW = cellSize * grid.cols;
    float gridH = cellSize * grid.rows;
    float offsetX = m_AreaX + (m_AreaW - gridW) / 2.0f;
    float offsetY = m_AreaY + (m_AreaH - gridH) / 2.0f;

    return { offsetX + col * cellSize, offsetY + row * cellSize };
}

bool GridRenderer::ScreenToGrid(int screenX, int screenY, const GridState& grid, int& outRow, int& outCol) const
{
    float cellSize = GetCellSize(grid);
    float gridW = cellSize * grid.cols;
    float gridH = cellSize * grid.rows;
    float offsetX = m_AreaX + (m_AreaW - gridW) / 2.0f;
    float offsetY = m_AreaY + (m_AreaH - gridH) / 2.0f;

    float localX = screenX - offsetX;
    float localY = screenY - offsetY;

    outCol = (int)(localX / cellSize);
    outRow = (int)(localY / cellSize);

    return outRow >= 0 && outRow < grid.rows && outCol >= 0 && outCol < grid.cols;
}

void GridRenderer::Draw(const GridState& grid, const ScannerDB& db, const PlacementState& placements, float time)
{
    // Background for grid area
    DrawRectangle(m_AreaX, m_AreaY, m_AreaW, m_AreaH, Color{ 18, 18, 28, 255 });

    DrawScanners(grid, db, placements);
    DrawOverlaps(grid, placements);
    DrawGridLines(grid);
    DrawDataNodes(grid, time);
}

void GridRenderer::DrawGridLines(const GridState& grid)
{
    float cellSize = GetCellSize(grid);
    float gridW = cellSize * grid.cols;
    float gridH = cellSize * grid.rows;
    float offsetX = m_AreaX + (m_AreaW - gridW) / 2.0f;
    float offsetY = m_AreaY + (m_AreaH - gridH) / 2.0f;

    Color lineColor = { 40, 45, 60, 180 };

    // Vertical lines
    for (int c = 0; c <= grid.cols; ++c)
    {
        float x = offsetX + c * cellSize;
        DrawLineV({ x, offsetY }, { x, offsetY + gridH }, lineColor);
    }

    // Horizontal lines
    for (int r = 0; r <= grid.rows; ++r)
    {
        float y = offsetY + r * cellSize;
        DrawLineV({ offsetX, y }, { offsetX + gridW, y }, lineColor);
    }

    // Border
    DrawRectangleLinesEx({ offsetX - 1, offsetY - 1, gridW + 2, gridH + 2 }, 2.0f, Color{ 60, 70, 100, 255 });
}

void GridRenderer::DrawDataNodes(const GridState& grid, float time)
{
    float cellSize = GetCellSize(grid);
    float radius = cellSize * Config::NODE_RADIUS_RATIO;

    for (int i = 0; i < grid.nodeCount; ++i)
    {
        int idx = grid.nodePositions[i];
        int r = idx / grid.cols;
        int c = idx % grid.cols;

        Vector2 pos = GridToScreen(r, c, grid);
        float cx = pos.x + cellSize / 2.0f;
        float cy = pos.y + cellSize / 2.0f;

        // Glow pulse
        float pulse = 0.7f + 0.3f * sinf(time * Config::NODE_GLOW_SPEED + i * 0.5f);
        float glowR = radius * (1.0f + 0.4f * pulse);

        // Outer glow
        DrawCircleV({ cx, cy }, glowR * 2.0f, Color{ 0, 200, 255, (unsigned char)(30 * pulse) });
        DrawCircleV({ cx, cy }, glowR * 1.5f, Color{ 0, 200, 255, (unsigned char)(50 * pulse) });

        // Core node
        DrawCircleV({ cx, cy }, radius, Color{ 0, (unsigned char)(180 + 75 * pulse), 255, 255 });

        // Inner bright dot
        DrawCircleV({ cx, cy }, radius * 0.4f, Color{ 200, 255, 255, (unsigned char)(200 * pulse) });
    }
}

void GridRenderer::DrawScanners(const GridState& grid, const ScannerDB& db, const PlacementState& placements)
{
    float cellSize = GetCellSize(grid);

    for (int p = 0; p < placements.placementCount; ++p)
    {
        const auto& pl = placements.placements[p];
        int t = pl.scannerType;
        Color baseColor = db.colors[t];

        int h = db.shapeHeight[t];
        int w = db.shapeWidth[t];
        const auto& mask = db.shapeMasks[t];

        for (int dr = 0; dr < h; ++dr)
        {
            int gr = pl.gridRow + dr;
            if (gr < 0 || gr >= grid.rows) continue;

            for (int dc = 0; dc < w; ++dc)
            {
                int gc = pl.gridCol + dc;
                if (gc < 0 || gc >= grid.cols) continue;

                if (mask.test(dr * Config::MAX_SHAPE_DIM + dc))
                {
                    Vector2 pos = GridToScreen(gr, gc, grid);
                    Color fillColor = baseColor;
                    fillColor.a = (unsigned char)(255 * Config::SCANNER_ALPHA);

                    DrawRectangleV(pos, { cellSize, cellSize }, fillColor);

                    // Subtle border
                    DrawRectangleLinesEx({ pos.x, pos.y, cellSize, cellSize }, 1.0f, Color{ baseColor.r, baseColor.g, baseColor.b, 100 });
                }
            }
        }
    }
}

void GridRenderer::DrawOverlaps(const GridState& grid, const PlacementState& placements)
{
    float cellSize = GetCellSize(grid);

    for (int r = 0; r < grid.rows; ++r)
    {
        for (int c = 0; c < grid.cols; ++c)
        {
            int idx = r * grid.cols + c;
            int overlap = placements.overlapCount[idx];
            if (overlap > 1)
            {
                Vector2 pos = GridToScreen(r, c, grid);
                unsigned char alpha = (unsigned char)std::min(200, (int)(overlap * Config::OVERLAP_ALPHA_BOOST * 255));
                DrawRectangleV(pos, { cellSize, cellSize }, Color{ 255, 50, 50, alpha });
            }
        }
    }
}

void GridRenderer::DrawGhostScanner(const GridState& grid, const ScannerDB& db, int scannerType, int gridRow, int gridCol, float alpha)
{
    float cellSize = GetCellSize(grid);
    int t = scannerType;
    int h = db.shapeHeight[t];
    int w = db.shapeWidth[t];
    const auto& mask = db.shapeMasks[t];
    Color baseColor = db.colors[t];

    for (int dr = 0; dr < h; ++dr)
    {
        int gr = gridRow + dr;
        if (gr < 0 || gr >= grid.rows) continue;

        for (int dc = 0; dc < w; ++dc)
        {
            int gc = gridCol + dc;
            if (gc < 0 || gc >= grid.cols) continue;

            if (mask.test(dr * Config::MAX_SHAPE_DIM + dc))
            {
                Vector2 pos = GridToScreen(gr, gc, grid);
                Color ghostColor = baseColor;
                ghostColor.a = (unsigned char)(255 * alpha * 0.5f);
                DrawRectangleV(pos, { cellSize, cellSize }, ghostColor);

                // Dashed border
                DrawRectangleLinesEx({ pos.x + 1, pos.y + 1, cellSize - 2, cellSize - 2 }, 1.5f, Color{ 255, 255, 255, (unsigned char)(180 * alpha) });
            }
        }
    }
}
