#pragma once

#include <bitset>
#include <cstring>

#include "GameConfig.h"

struct ScannerDB
{
    int typeCount = 0;

    // --- Parallel arrays (SoA) ---
    const char* names[Config::MAX_SCANNER_TYPES] = {};
    int weights[Config::MAX_SCANNER_TYPES] = {};
    Color colors[Config::MAX_SCANNER_TYPES] = {};
    int shapeWidth[Config::MAX_SCANNER_TYPES] = {};
    int shapeHeight[Config::MAX_SCANNER_TYPES] = {};

    // shapeMasks[i]: bit (dr * MAX_SHAPE_DIM + dc) set if scanner covers
    // local offset (dr, dc) from placement origin.
    std::bitset<Config::MAX_SHAPE_CELLS> shapeMasks[Config::MAX_SCANNER_TYPES];

    // --- Methods ---

    // Add a scanner type. shape is a row-major bool array of (h x w).
    int AddType(const char* name, int weight, Color color, int w, int h, const bool* shape)
    {
        int idx = typeCount++;
        names[idx] = name;
        weights[idx] = weight;
        colors[idx] = color;
        shapeWidth[idx] = w;
        shapeHeight[idx] = h;
        shapeMasks[idx].reset();

        for (int dr = 0; dr < h; ++dr)
        {
            for (int dc = 0; dc < w; ++dc)
            {
                if (shape[dr * w + dc])
                {
                    shapeMasks[idx].set(dr * Config::MAX_SHAPE_DIM + dc);
                }
            }
        }
        return idx;
    }

    // Compute the global-grid coverage bitset when scanner type `t` is placed
    // at grid position (row, col). Returns only bits within grid bounds.
    std::bitset<Config::MAX_CELLS> ComputeCoverage(int t, int row, int col, int gridRows, int gridCols) const
    {
        std::bitset<Config::MAX_CELLS> result;
        int h = shapeHeight[t];
        int w = shapeWidth[t];
        const auto& mask = shapeMasks[t];

        for (int dr = 0; dr < h; ++dr)
        {
            int gr = row + dr;
            if (gr < 0 || gr >= gridRows) continue;
            for (int dc = 0; dc < w; ++dc)
            {
                int gc = col + dc;
                if (gc < 0 || gc >= gridCols) continue;
                if (mask.test(dr * Config::MAX_SHAPE_DIM + dc))
                {
                    result.set(gr * gridCols + gc);
                }
            }
        }
        return result;
    }

    // Populate with default scanner types for gameplay
    void LoadDefaults()
    {
        typeCount = 0;

        // 1x1 Probe (weight 1)
        {
            bool shape[] = { true };
            AddType("Probe", 1, Color{0, 200, 255, 255}, 1, 1, shape);
        }

        // 2x2 Quad Scanner (weight 3)
        {
            bool shape[] = { true, true,
                             true, true };
            AddType("Quad", 3, Color{100, 255, 100, 255}, 2, 2, shape);
        }

        // 3x3 Block Scanner (weight 6)
        {
            bool shape[] = { true, true, true,
                             true, true, true,
                             true, true, true };
            AddType("Block 3x3", 6, Color{255, 180, 50, 255}, 3, 3, shape);
        }

        // Cross Scanner 3x3 (weight 3)
        {
            bool shape[] = { false, true, false,
                             true,  true, true,
                             false, true, false };
            AddType("Cross", 3, Color{255, 80, 200, 255}, 3, 3, shape);
        }
    }
};
