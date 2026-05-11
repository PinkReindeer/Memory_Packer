#include <climits>
#include <algorithm>

#include "Backtracking.h"

void BacktrackingAlgorithm::Init(const GridState& grid, const ScannerDB& db)
{
    m_Grid = &grid;
    m_DB = &db;
    m_Current.Clear();
    m_Best.Clear();
    m_BestWeight = INT_MAX;
    m_Status = AlgoStatus::Running;
    m_StepsExecuted = 0;
    m_FoundSolution = false;
    StartTimer();

    // Step 1: Generate all candidate placements that cover at least 1 node
    m_Candidates.clear();
    for (int t = 0; t < db.typeCount; ++t)
    {
        int sw = db.shapeWidth[t];
        int sh = db.shapeHeight[t];
        for (int r = -(sh - 1); r < grid.rows; ++r)
        {
            for (int c = -(sw - 1); c < grid.cols; ++c)
            {
                auto cov = db.ComputeCoverage(t, r, c, grid.rows, grid.cols);
                auto nodeCov = cov & grid.nodeMask;
                if (nodeCov.count() > 0)
                {
                    m_Candidates.push_back({ t, r, c, nodeCov });
                }
            }
        }
    }

    // Step 2: Sort by coverage count (descending) to find solutions faster
    std::sort(m_Candidates.begin(), m_Candidates.end(),
        [](const Candidate& a, const Candidate& b) {
            return a.nodeCoverage.count() > b.nodeCoverage.count();
        });

    // Step 3: Push initial DFS node onto stack
    while (!m_Stack.empty()) m_Stack.pop();
    DFSNode root;
    root.state.Clear();
    root.candidateIdx = 0;
    m_Stack.push(root);
}

AlgoStatus BacktrackingAlgorithm::Step()
{
    if (m_Status != AlgoStatus::Running) return m_Status;
    m_StepsExecuted++;

    // If stack is empty, we've explored everything
    if (m_Stack.empty())
    {
        m_Status = AlgoStatus::Finished;
        UpdateTimer();
        return m_Status;
    }

    // Pop the next node to explore
    DFSNode node = m_Stack.top();
    m_Stack.pop();
    m_Current = node.state;

    // --- Check: is this a complete solution? ---
    if (node.state.IsComplete(*m_Grid))
    {
        if (node.state.totalWeight < m_BestWeight)
        {
            m_BestWeight = node.state.totalWeight;
            m_Best = node.state;
            m_FoundSolution = true;
        }
        UpdateTimer();
        return AlgoStatus::Running;
    }

    // --- Check: have we run out of candidates? (dead end) ---
    if (node.candidateIdx >= (int)m_Candidates.size())
    {
        UpdateTimer();
        return AlgoStatus::Running;
    }

    // --- Prune: current cost already too high ---
    if (m_FoundSolution && node.state.totalWeight >= m_BestWeight)
    {
        UpdateTimer();
        return AlgoStatus::Running;
    }

    // --- Branch 1: SKIP this candidate ---
    DFSNode skipNode;
    skipNode.state = node.state;
    skipNode.candidateIdx = node.candidateIdx + 1;
    m_Stack.push(skipNode);

    // --- Branch 2: INCLUDE this candidate (only if it covers new nodes) ---
    const auto& cand = m_Candidates[node.candidateIdx];
    bool coversNew = (cand.nodeCoverage & ~node.state.coveredMask).any();

    if (coversNew)
    {
        DFSNode includeNode;
        includeNode.state = node.state;
        includeNode.state.AddPlacement(*m_Grid, *m_DB, { cand.scannerType, cand.row, cand.col });
        includeNode.candidateIdx = node.candidateIdx + 1;

        // Only push if the new cost could still beat the best
        if (!m_FoundSolution || includeNode.state.totalWeight < m_BestWeight)
        {
            m_Stack.push(includeNode);
        }
    }

    UpdateTimer();
    return AlgoStatus::Running;
}
