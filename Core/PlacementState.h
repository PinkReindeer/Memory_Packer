#pragma once

#include <bitset>

#include "GameConfig.h"
#include "GridState.h"
#include "ScannerDB.h"

struct Placement
{
    int scannerType = -1;
    int gridRow = 0;
    int gridCol = 0;
};

struct PlacementState
{
    int placementCount = 0;
    Placement placements[Config::MAX_PLACEMENTS] = {};

    // Aggregate coverage of ALL placed scanners (OR of individual masks)
    std::bitset<Config::MAX_CELLS> coveredMask;

    // Per-cell overlap count (for rendering overlap intensity)
    uint8_t overlapCount[Config::MAX_CELLS] = {};

    int totalWeight = 0;

    // --- Methods ---

    void Clear()
    {
        placementCount = 0;
        coveredMask.reset();
        std::memset(overlapCount, 0, sizeof(overlapCount));
        totalWeight = 0;
    }

    // Add a placement and incrementally update coverage
    void AddPlacement(const GridState& grid, const ScannerDB& db, Placement p)
    {
        placements[placementCount++] = p;
        totalWeight += db.weights[p.scannerType];

        auto mask = db.ComputeCoverage(p.scannerType, p.gridRow, p.gridCol, grid.rows, grid.cols);
        coveredMask |= mask;

        // Update overlap counts
        for (int i = 0; i < grid.TotalCells(); ++i)
        {
            if (mask.test(i))
            {
                overlapCount[i]++;
            }
        }
    }

    // Remove the last placement and recompute state
    void PopPlacement(const GridState& grid, const ScannerDB& db)
    {
        if (placementCount <= 0) return;
        placementCount--;
        Recompute(grid, db);
    }

    // Full recompute from scratch (used after pop or load)
    void Recompute(const GridState& grid, const ScannerDB& db)
    {
        coveredMask.reset();
        std::memset(overlapCount, 0, sizeof(overlapCount));
        totalWeight = 0;

        for (int i = 0; i < placementCount; ++i)
        {
            const auto& p = placements[i];
            totalWeight += db.weights[p.scannerType];

            auto mask = db.ComputeCoverage(p.scannerType, p.gridRow, p.gridCol, grid.rows, grid.cols);
            coveredMask |= mask;

            for (int j = 0; j < grid.TotalCells(); ++j)
            {
                if (mask.test(j))
                {
                    overlapCount[j]++;
                }
            }
        }
    }

    // Count how many universe nodes (not just cells) are covered
    int CoveredNodeCount(const GridState& grid) const
    {
        return (int)(coveredMask & grid.nodeMask).count();
    }

    // Check if all nodes are covered
    bool IsComplete(const GridState& grid) const
    {
        return (coveredMask & grid.nodeMask) == grid.nodeMask;
    }
};
