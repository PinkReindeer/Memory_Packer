#pragma once

#include <cmath>

#include "../Core/GameConfig.h"
#include "../Core/GridState.h"
#include "../Core/ScannerDB.h"
#include "../Core/PlacementState.h"

class GridRenderer
{
public:
    void Init(int areaX, int areaY, int areaW, int areaH);

    // Main draw call
    void Draw(const GridState& grid, const ScannerDB& db, const PlacementState& placements, float time);

    // Draw a ghost/preview scanner (for manual mode hover)
    void DrawGhostScanner(const GridState& grid, const ScannerDB& db, int scannerType, int gridRow, int gridCol, float alpha);

    // Convert screen coords to grid cell. Returns false if outside grid.
    bool ScreenToGrid(int screenX, int screenY, const GridState& grid, int& outRow, int& outCol) const;

    // Get cell size for current grid
    float GetCellSize(const GridState& grid) const;

    // Get screen position of a grid cell
    Vector2 GridToScreen(int row, int col, const GridState& grid) const;

private:
    int m_AreaX = 0, m_AreaY = 0, m_AreaW = 0, m_AreaH = 0;

    void DrawGridLines(const GridState& grid);
    void DrawDataNodes(const GridState& grid, float time);
    void DrawScanners(const GridState& grid, const ScannerDB& db, const PlacementState& placements);
    void DrawOverlaps(const GridState& grid, const PlacementState& placements);
};
