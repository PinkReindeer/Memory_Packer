#include <cstdlib>
#include <ctime>

#include "Game.h"

#include "../Algorithms/Greedy.h"
#include "../Algorithms/Backtracking.h"
#include "../Algorithms/BranchAndBound.h"
#include "../Algorithms/GeneticAlgorithm.h"

Game::Game()
{
    std::srand((unsigned int)std::time(nullptr));

    // Initialize scanner database with default types
    m_ScannerDB.LoadDefaults();

    // Generate random grid
    m_Grid.GenerateRandom(Config::DEFAULT_ROWS, Config::DEFAULT_COLS, Config::DEFAULT_NODE_PCT);

    // Clear placements
    m_Placements.Clear();

    // Initialize renderers
    m_GridRenderer.Init(0, 0, Config::GRID_AREA_WIDTH, Config::SCREEN_HEIGHT);
    m_UIPanel.Init(Config::GRID_AREA_WIDTH, 0, Config::PANEL_WIDTH, Config::SCREEN_HEIGHT);
}

void Game::Update(float dt)
{
    m_Time += dt;

    if (m_Mode == GameMode::Manual)
    {
        HandleManualMode();
    }
    else
    {
        HandleVisualizerMode(dt);
    }
}

void Game::Render()
{
    // Determine which placement state to show
    const PlacementState* displayState = &m_Placements;
    if (m_Mode == GameMode::Visualizer && m_Algorithm && m_Algorithm->GetStatus() != AlgoStatus::Idle)
    {
        displayState = &m_Algorithm->CurrentState();
    }

    // Draw grid viewport
    m_GridRenderer.Draw(m_Grid, m_ScannerDB, *displayState, m_Time);

    // Draw ghost preview in manual mode
    if (m_Mode == GameMode::Manual && m_HoverRow >= 0 && m_HoverCol >= 0 && m_SelectedScanner >= 0 && m_SelectedScanner < m_ScannerDB.typeCount)
    {
        m_GridRenderer.DrawGhostScanner(m_Grid, m_ScannerDB, m_SelectedScanner, m_HoverRow, m_HoverCol, 0.6f);
    }

    // Draw UI panel
    UIPanelOutput ui = m_UIPanel.Draw(m_Grid, m_ScannerDB, *displayState, m_Algorithm.get(), m_Mode, m_AlgoPaused, m_SelectedScanner);

    // --- Process UI commands ---

    // Mode change
    if (ui.modeChanged && ui.newMode != m_Mode)
    {
        m_Mode = ui.newMode;
        ResetAlgorithm();
    }

    // Manual mode commands
    if (ui.selectedScanner >= 0)
    {
        m_SelectedScanner = ui.selectedScanner;
    }
    if (ui.clearPlacements)
    {
        m_Placements.Clear();
    }

    // Visualizer commands
    m_StepsPerSec = ui.stepsPerSec;
    if (ui.startAlgorithm)
    {
        StartAlgorithm(ui.algoType);
    }
    if (ui.pauseResume)
    {
        m_AlgoPaused = !m_AlgoPaused;
    }
    if (ui.resetAlgorithm)
    {
        ResetAlgorithm();
    }

    // Grid regeneration
    if (ui.regenerateGrid)
    {
        RegenerateGrid(ui.gridRows, ui.gridCols, ui.nodeDensity);
    }
}

void Game::HandleManualMode()
{
    // Update hover position
    int mx = GetMouseX();
    int my = GetMouseY();
    if (!m_GridRenderer.ScreenToGrid(mx, my, m_Grid, m_HoverRow, m_HoverCol))
    {
        m_HoverRow = -1;
        m_HoverCol = -1;
    }

    // Only process clicks in the grid area (not on the UI panel)
    if (mx < Config::GRID_AREA_WIDTH)
    {
        // Left click to place
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && m_HoverRow >= 0 && m_HoverCol >= 0 && m_SelectedScanner >= 0)
        {
            m_Placements.AddPlacement(m_Grid, m_ScannerDB, { m_SelectedScanner, m_HoverRow, m_HoverCol });
        }

        // Right click to undo last
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
        {
            m_Placements.PopPlacement(m_Grid, m_ScannerDB);
        }
    }
}

void Game::HandleVisualizerMode(float dt)
{
    if (!m_Algorithm || m_Algorithm->GetStatus() != AlgoStatus::Running || m_AlgoPaused)
    {
        return;
    }

    m_StepTimer += dt;
    float stepInterval = 1.0f / (float)m_StepsPerSec;

    while (m_StepTimer >= stepInterval)
    {
        m_StepTimer -= stepInterval;

        AlgoStatus status = m_Algorithm->Step();
        if (status == AlgoStatus::Finished)
        {
            // Record result in chart
            auto result = m_Algorithm->GetResult();
            m_UIPanel.GetChart().AddEntry(m_Algorithm->Name(), (float)result.bestSolution.totalWeight, (float)result.elapsedMs, result.foundSolution);

            // Copy best solution to main placements for display
            m_Placements = result.bestSolution;
            break;
        }
    }
}

void Game::StartAlgorithm(AlgorithmType type)
{
    ResetAlgorithm();

    switch (type)
    {
        case AlgorithmType::Greedy:
            m_Algorithm = std::make_unique<GreedyAlgorithm>();
            break;
        case AlgorithmType::Backtracking:
            m_Algorithm = std::make_unique<BacktrackingAlgorithm>();
            break;
        case AlgorithmType::BranchAndBound:
            m_Algorithm = std::make_unique<BranchAndBoundAlgorithm>();
            break;
        case AlgorithmType::GeneticAlgorithm:
            m_Algorithm = std::make_unique<GeneticAlgorithmImpl>();
            break;
        default:
            return;
    }

    m_Algorithm->Init(m_Grid, m_ScannerDB);
    m_AlgoPaused = false;
    m_StepTimer = 0.0f;
}

void Game::ResetAlgorithm()
{
    m_Algorithm.reset();
    m_AlgoPaused = false;
    m_StepTimer = 0.0f;
    m_Placements.Clear();
}

void Game::RegenerateGrid(int rows, int cols, int density)
{
    m_Grid.GenerateRandom(rows, cols, density);
    m_Placements.Clear();
    ResetAlgorithm();
}
