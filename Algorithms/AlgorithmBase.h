#pragma once

#include <chrono>

#include "../Core/GridState.h"
#include "../Core/ScannerDB.h"
#include "../Core/PlacementState.h"

enum class AlgoStatus
{
    Idle,       // Not started
    Running,    // Step() can be called
    Finished,   // Solution found (or exhausted)
    Failed      // Error or infeasible
};

struct AlgoResult
{
    PlacementState bestSolution;
    double elapsedMs = 0.0;
    int stepsExecuted  = 0;
    bool foundSolution  = false;
};

class AlgorithmBase
{
public:
    virtual ~AlgorithmBase() = default;

    // Initialize with problem instance
    virtual void Init(const GridState& grid, const ScannerDB& db) = 0;

    // Advance one visual step. Returns current status.
    virtual AlgoStatus Step() = 0;

    // Current working state (for rendering the "in-progress" view)
    virtual const PlacementState& CurrentState() const = 0;

    // Best solution found so far
    virtual const PlacementState& BestState() const = 0;

    // Algorithm name for UI
    virtual const char* Name() const = 0;

    // Get result summary
    AlgoResult GetResult() const
    {
        AlgoResult r;
        r.bestSolution = BestState();
        r.elapsedMs = m_ElapsedMs;
        r.stepsExecuted = m_StepsExecuted;
        r.foundSolution = m_FoundSolution;
        return r;
    }

    AlgoStatus GetStatus() const { return m_Status; }
    int GetSteps() const { return m_StepsExecuted; }

protected:
    AlgoStatus m_Status = AlgoStatus::Idle;
    int m_StepsExecuted = 0;
    double m_ElapsedMs = 0.0;
    bool m_FoundSolution = false;

    // Pointers to shared problem data (not owned)
    const GridState* m_Grid = nullptr;
    const ScannerDB* m_DB = nullptr;

    void StartTimer()
    {
        m_TimerStart = std::chrono::high_resolution_clock::now();
    }

    void UpdateTimer()
    {
        auto now = std::chrono::high_resolution_clock::now();
        m_ElapsedMs = std::chrono::duration<double, std::milli>(now - m_TimerStart).count();
    }

private:
    std::chrono::high_resolution_clock::time_point m_TimerStart;
};
