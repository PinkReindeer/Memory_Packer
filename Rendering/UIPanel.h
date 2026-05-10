#pragma once

#include "ChartWidget.h" 

#include "../Core/GameConfig.h"
#include "../Core/GridState.h"
#include "../Core/ScannerDB.h"
#include "../Core/PlacementState.h"
#include "../Algorithms/AlgorithmBase.h"

enum class GameMode
{
    Manual,
    Visualizer
};

enum class AlgorithmType
{
    Greedy = 0,
    Backtracking,
    BranchAndBound,
    SimulatedAnnealing,
    GeneticAlgorithm,
    COUNT
};

struct UIPanelOutput
{
    bool modeChanged = false;
    GameMode newMode = GameMode::Manual;

    int selectedScanner = -1;
    bool clearPlacements = false;

    bool startAlgorithm = false;
    bool pauseResume = false;
    bool resetAlgorithm = false;
    AlgorithmType algoType = AlgorithmType::Greedy;
    int stepsPerSec = Config::DEFAULT_STEPS_PER_SEC;

    bool regenerateGrid = false;
    int gridRows = Config::DEFAULT_ROWS;
    int gridCols = Config::DEFAULT_COLS;
    int nodeDensity = Config::DEFAULT_NODE_PCT;
};

class UIPanel
{
public:
    void Init(int x, int y, int w, int h);

    UIPanelOutput Draw(const GridState& grid, const ScannerDB& db, const PlacementState& placements, const AlgorithmBase* algorithm,
                       GameMode currentMode, bool algoPaused, int currentSelectedScanner);

    ChartWidget& GetChart() { return m_Chart; }

private:
    int m_X = 0, m_Y = 0, m_W = 0, m_H = 0;

    int m_SelectedAlgo = 0;
    int m_StepsPerSec = Config::DEFAULT_STEPS_PER_SEC;
    int m_GridRows = Config::DEFAULT_ROWS;
    int m_GridCols = Config::DEFAULT_COLS;
    int m_NodeDensity = Config::DEFAULT_NODE_PCT;

    ChartWidget m_Chart;

    bool DrawButton(int x, int y, int w, int h, const char* text, Color bgColor, Color textColor);
    void DrawSlider(int x, int y, int w, const char* label, int& value, int minVal, int maxVal);
    void DrawLabel(int x, int y, const char* text, int fontSize, Color color);
};
