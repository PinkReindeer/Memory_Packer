#pragma once

namespace Config
{

    // --- Grid ---
    constexpr int MAX_GRID_ROWS     = 32;
    constexpr int MAX_GRID_COLS     = 32;
    constexpr int MAX_CELLS         = MAX_GRID_ROWS * MAX_GRID_COLS; // 1024
    constexpr int DEFAULT_ROWS      = 10;
    constexpr int DEFAULT_COLS      = 10;
    constexpr int DEFAULT_NODE_PCT  = 35;  // % of cells that are data nodes

    // --- Scanners ---
    constexpr int MAX_SCANNER_TYPES = 32;
    constexpr int MAX_SHAPE_DIM     = 8;   // max width/height of a scanner shape
    constexpr int MAX_SHAPE_CELLS   = MAX_SHAPE_DIM * MAX_SHAPE_DIM; // 64
    constexpr int MAX_PLACEMENTS    = 512;

    // --- Window ---
    constexpr int SCREEN_WIDTH      = 1440;
    constexpr int SCREEN_HEIGHT     = 900;
    constexpr int PANEL_WIDTH       = 400;
    constexpr int GRID_AREA_WIDTH   = SCREEN_WIDTH - PANEL_WIDTH;
    constexpr int GRID_PADDING      = 40;

    // --- Algorithm ---
    constexpr int   DEFAULT_STEPS_PER_SEC = 5;
    constexpr int   MAX_STEPS_PER_SEC     = 2000;
    constexpr int   MIN_STEPS_PER_SEC     = 1;
    constexpr float SA_INITIAL_TEMP       = 1000.0f;
    constexpr float SA_COOLING_RATE       = 0.995f;
    constexpr float SA_MIN_TEMP           = 0.01f;
    constexpr int   GA_POPULATION_SIZE    = 50;
    constexpr int   GA_MAX_GENERATIONS    = 500;
    constexpr float GA_MUTATION_RATE      = 0.1f;
    constexpr float GA_CROSSOVER_RATE     = 0.7f;

    // --- Rendering ---
    constexpr float NODE_GLOW_SPEED       = 3.0f;
    constexpr float NODE_RADIUS_RATIO     = 0.3f;  // fraction of cell size
    constexpr float SCANNER_ALPHA         = 0.35f;
    constexpr float OVERLAP_ALPHA_BOOST   = 0.15f;

} // namespace Config
