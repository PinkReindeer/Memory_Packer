#pragma once

#include <bitset>
#include <cstdlib>
#include <ctime>

#include "GameConfig.h"

struct GridState
{
    int rows = Config::DEFAULT_ROWS;
    int cols = Config::DEFAULT_COLS;
    int nodeCount = 0;

    // Bit i is set if cell i contains a data node (universe element)
    std::bitset<Config::MAX_CELLS> nodeMask;

    // Contiguous array of flat indices where nodes exist (for fast iteration)
    int  nodePositions[Config::MAX_CELLS] = {};

    // --- Methods ---

    // Flatten 2D coord to 1D index
    inline int Idx(int r, int c) const { return r * cols + c; }

    // Total cells
    inline int TotalCells() const { return rows * cols; }

    // Generate random nodes with given density percentage
    void GenerateRandom(int r, int c, int densityPercent)
    {
        rows = r;
        cols = c;
        nodeMask.reset();
        nodeCount = 0;

        for (int i = 0; i < rows * cols; ++i)
        {
            if ((std::rand() % 100) < densityPercent)
            {
                nodeMask.set(i);
                nodePositions[nodeCount++] = i;
            }
        }

        // Ensure at least 1 node
        if (nodeCount == 0)
        {
            int center = Idx(rows / 2, cols / 2);
            nodeMask.set(center);
            nodePositions[nodeCount++] = center;
        }
    }

    // Rebuild the nodePositions array from nodeMask
    void RebuildPositions()
    {
        nodeCount = 0;
        for (int i = 0; i < rows * cols; ++i)
        {
            if (nodeMask.test(i))
            {
                nodePositions[nodeCount++] = i;
            }
        }
    }
};
