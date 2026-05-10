#include "BranchAndBound.h"
#include <climits>
#include <algorithm>

// ============================================================
// BRANCH AND BOUND: DFS with a lower-bound cost estimate
//
// Same include/skip branching as Backtracking, but smarter:
//   - Estimates the MINIMUM additional cost to cover remaining nodes
//   - Prunes if (current cost + lower bound) >= best known cost
//   - This cuts far more branches than simple cost pruning alone
//
// Lower bound idea:
//   We need at least ceil(uncovered / maxCoverage) more scanners,
//   each costing at least the cheapest scanner weight.
// ============================================================

void BranchAndBoundAlgorithm::Init(const GridState& grid, const ScannerDB& db)
{
    m_Grid = &grid;
    m_DB = &db;
    m_Current.Clear();
    m_Best.Clear();
    m_BestWeight = INT_MAX;
    m_PrunedCount = 0;
    m_Status = AlgoStatus::Running;
    m_StepsExecuted = 0;
    m_FoundSolution = false;
    StartTimer();

    // Generate all candidate placements that cover at least 1 node
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

    // Sort by efficiency: low (weight / coverage) first
    std::sort(m_Candidates.begin(), m_Candidates.end(),
        [&](const Candidate& a, const Candidate& b) {
            float ra = (float)db.weights[a.scannerType] / a.nodeCoverage.count();
            float rb = (float)db.weights[b.scannerType] / b.nodeCoverage.count();
            return ra < rb;
        });

    // Push initial DFS node
    while (!m_Stack.empty()) m_Stack.pop();
    DFSNode root;
    root.state.Clear();
    root.candidateIdx = 0;
    m_Stack.push(root);
}

// Simple lower bound: minimum scanners needed * cheapest scanner weight
int BranchAndBoundAlgorithm::EstimateLowerBound(const PlacementState& state) const
{
    auto uncovered = m_Grid->nodeMask & ~state.coveredMask;
    int uncoveredCount = (int)uncovered.count();
    if (uncoveredCount == 0) return 0;

    // Find the max coverage and min weight among all candidates
    int maxCov = 1;
    int minWeight = INT_MAX;
    for (const auto& c : m_Candidates)
    {
        int cov = (int)(c.nodeCoverage & uncovered).count();
        if (cov > maxCov) maxCov = cov;

        int w = m_DB->weights[c.scannerType];
        if (w < minWeight) minWeight = w;
    }

    if (minWeight == INT_MAX) return INT_MAX;

    // At minimum we need ceil(uncovered / maxCov) more scanners
    int minScanners = (uncoveredCount + maxCov - 1) / maxCov;
    return minScanners * minWeight;
}

AlgoStatus BranchAndBoundAlgorithm::Step()
{
    if (m_Status != AlgoStatus::Running) return m_Status;
    m_StepsExecuted++;

    // If stack is empty, search is complete
    if (m_Stack.empty())
    {
        m_Status = AlgoStatus::Finished;
        UpdateTimer();
        return m_Status;
    }

    // Pop next node
    DFSNode node = m_Stack.top();
    m_Stack.pop();
    m_Current = node.state;

    // --- Check: complete solution? ---
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

    // --- Check: out of candidates? ---
    if (node.candidateIdx >= (int)m_Candidates.size())
    {
        UpdateTimer();
        return AlgoStatus::Running;
    }

    // --- Prune: cost bound (simple) ---
    if (m_FoundSolution && node.state.totalWeight >= m_BestWeight)
    {
        m_PrunedCount++;
        UpdateTimer();
        return AlgoStatus::Running;
    }

    // --- Prune: cost + lower bound (the B&B advantage) ---
    if (m_FoundSolution)
    {
        int lb = EstimateLowerBound(node.state);
        if (node.state.totalWeight + lb >= m_BestWeight)
        {
            m_PrunedCount++;
            UpdateTimer();
            return AlgoStatus::Running;
        }
    }

    // --- Branch 1: SKIP this candidate ---
    DFSNode skipNode;
    skipNode.state = node.state;
    skipNode.candidateIdx = node.candidateIdx + 1;
    m_Stack.push(skipNode);

    // --- Branch 2: INCLUDE this candidate ---
    const auto& cand = m_Candidates[node.candidateIdx];
    bool coversNew = (cand.nodeCoverage & ~node.state.coveredMask).any();

    if (coversNew)
    {
        DFSNode includeNode;
        includeNode.state = node.state;
        includeNode.state.AddPlacement(*m_Grid, *m_DB,
            { cand.scannerType, cand.row, cand.col });
        includeNode.candidateIdx = node.candidateIdx + 1;

        if (!m_FoundSolution || includeNode.state.totalWeight < m_BestWeight)
        {
            m_Stack.push(includeNode);
        }
        else
        {
            m_PrunedCount++;
        }
    }

    UpdateTimer();
    return AlgoStatus::Running;
}