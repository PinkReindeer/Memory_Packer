#pragma once

#include <memory>

#include "../Core/GameConfig.h"
#include "../Core/GridState.h"
#include "../Core/ScannerDB.h"
#include "../Core/PlacementState.h"
#include "../Algorithms/AlgorithmBase.h"
#include "../Renderer/GridRenderer.h"
#include "../Renderer/UIPanel.h"

class Game
{
public:
    Game();
    ~Game() = default;

    void Update(float dt);
    void Render();

private:
    // --- Core data ---
    GridState m_Grid;
    ScannerDB m_ScannerDB;
    PlacementState m_Placements;

    // --- Mode ---
    GameMode m_Mode = GameMode::Manual;

    // --- Manual mode state ---
    int m_SelectedScanner = 0;
    int m_HoverRow = -1, m_HoverCol = -1;

    // --- Visualizer mode state ---
    std::unique_ptr<AlgorithmBase> m_Algorithm;
    bool  m_AlgoPaused = false;
    float m_StepTimer = 0.0f;
    int   m_StepsPerSec = Config::DEFAULT_STEPS_PER_SEC;

    // --- Rendering ---
    GridRenderer m_GridRenderer;
    UIPanel m_UIPanel;
    float m_Time = 0.0f;

    // --- Methods ---
    void HandleManualMode();
    void HandleVisualizerMode(float dt);
    void StartAlgorithm(AlgorithmType type);
    void ResetAlgorithm();
    void RegenerateGrid(int rows, int cols, int density);
};
