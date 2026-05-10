#include "Greedy.h"

void GreedyAlgorithm::Init(const GridState& grid, const ScannerDB& db)
{
    m_Grid = &grid;
    m_DB = &db;
    m_Current.Clear();
    m_Status = AlgoStatus::Running;
    m_StepsExecuted = 0;
    m_FoundSolution = false;
    StartTimer();
}

AlgoStatus GreedyAlgorithm::Step()
{
    if (m_Status != AlgoStatus::Running) return m_Status;
    m_StepsExecuted++;

    // Check if already complete
    if (m_Current.IsComplete(*m_Grid))
    {
        m_FoundSolution = true;
        m_Status = AlgoStatus::Finished;
        UpdateTimer();
        return m_Status;
    }

    // Find the placement with minimum weight/newNodes ratio
    float bestRatio = std::numeric_limits<float>::max();
    Candidate best = { -1, 0, 0, 0, bestRatio };

    for (int t = 0; t < m_DB->typeCount; ++t)
    {
        int w = m_DB->weights[t];
        int sw = m_DB->shapeWidth[t];
        int sh = m_DB->shapeHeight[t];

        for (int r = -(sh - 1); r < m_Grid->rows; ++r)
        {
            for (int c = -(sw - 1); c < m_Grid->cols; ++c)
            {
                auto coverage = m_DB->ComputeCoverage(t, r, c, m_Grid->rows, m_Grid->cols);

                // New nodes = nodes in coverage that aren't already covered
                auto newCoverage = coverage & m_Grid->nodeMask & ~m_Current.coveredMask;
                int newNodes = (int)newCoverage.count();

                if (newNodes > 0)
                {
                    float ratio = (float)w / (float)newNodes;
                    if (ratio < bestRatio || (ratio == bestRatio && newNodes > best.newNodes)) 
                    {
                        bestRatio = ratio;
                        best = { t, r, c, newNodes, ratio };
                    }
                }
            }
        }
    }

    // If no placement can cover new nodes, we're stuck
    if (best.scannerType < 0)
    {
        m_Status = AlgoStatus::Finished;
        m_FoundSolution = m_Current.IsComplete(*m_Grid);
        UpdateTimer();
        return m_Status;
    }

    // Place the best scanner
    m_BestCandidate = best;
    m_Current.AddPlacement(*m_Grid, *m_DB, { best.scannerType, best.row, best.col });

    // Check completion after placement
    if (m_Current.IsComplete(*m_Grid))
    {
        m_FoundSolution = true;
        m_Status = AlgoStatus::Finished;
    }

    UpdateTimer();
    return m_Status;
}
