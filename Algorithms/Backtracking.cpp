#include "Backtracking.h"
#include <climits>
#include <algorithm>

void BacktrackingAlgorithm::Init(const GridState& grid, const ScannerDB& db)
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

    // Pre-generate candidate placements: only those that cover >= 1 node
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

    // Sort candidates by efficiency (low weight/coverage ratio first)
    std::sort(m_Candidates.begin(), m_Candidates.end(), [&](const CandidatePlacement& a, const CandidatePlacement& b) {
            float ra = (float)db.weights[a.scannerType] / a.nodeCoverage.count();
            float rb = (float)db.weights[b.scannerType] / b.nodeCoverage.count();
            return ra < rb;
        });

    RemoveDominatedCandidates();
    PrecomputeSuffixCoverage();

    // Push initial state
    while (!m_Stack.empty()) m_Stack.pop();
    DFSNode root;
    root.state.Clear();
    root.candidateIdx = 0;
    m_Stack.push(root);
}

// ---- Optimization helpers ----

void BacktrackingAlgorithm::PrecomputeSuffixCoverage()
{
    int n = (int)m_Candidates.size();
    m_SuffixCoverage.resize(n + 1);
    m_SuffixCoverage[n].reset();

    for (int i = n - 1; i >= 0; --i)
    {
        m_SuffixCoverage[i] = m_SuffixCoverage[i + 1] | m_Candidates[i].nodeCoverage;
    }
}

bool BacktrackingAlgorithm::CanCoverRemaining(const PlacementState& state, int fromIdx) const
{
    auto uncovered = m_Grid->nodeMask & ~state.coveredMask;
    auto reachable = m_SuffixCoverage[fromIdx];
    return (uncovered & ~reachable).none();
}

int BacktrackingAlgorithm::EstimateLowerBound(const PlacementState& state, int fromIdx) const
{
    auto uncovered = m_Grid->nodeMask & ~state.coveredMask;
    if (uncovered.none()) return 0;

    int n = (int)m_Candidates.size();
    int totalCells = m_Grid->rows * m_Grid->cols;

    // Precompute the efficiency ratio for each remaining candidate
    // against the current uncovered set (avoids redundant bitset ops).
    std::vector<float> ratios(n, (float)INT_MAX);
    for (int j = fromIdx; j < n; ++j)
    {
        int newCov = (int)(m_Candidates[j].nodeCoverage & uncovered).count();
        if (newCov > 0)
            ratios[j] = (float)m_DB->weights[m_Candidates[j].scannerType] / newCov;
    }

    float lb = 0.0f;
    for (int i = 0; i < totalCells; ++i)
    {
        if (!uncovered.test(i)) continue;

        float bestRatio = (float)INT_MAX;
        for (int j = fromIdx; j < n; ++j)
        {
            if (ratios[j] >= bestRatio) continue;        // early skip
            if (m_Candidates[j].nodeCoverage.test(i))
                bestRatio = ratios[j];
        }
        lb += bestRatio;
    }

    return (int)lb;  // floor is still a valid lower bound (costs are integers)
}

void BacktrackingAlgorithm::RemoveDominatedCandidates()
{
    int n = (int)m_Candidates.size();
    std::vector<bool> dominated(n, false);

    for (int i = 0; i < n; ++i)
    {
        if (dominated[i]) continue;
        for (int j = i + 1; j < n; ++j)
        {
            if (dominated[j]) continue;

            int wi = m_DB->weights[m_Candidates[i].scannerType];
            int wj = m_DB->weights[m_Candidates[j].scannerType];
            const auto& ci = m_Candidates[i].nodeCoverage;
            const auto& cj = m_Candidates[j].nodeCoverage;

            // j dominated by i: i covers superset, i costs <=
            if (wi <= wj && (cj & ~ci).none())
            {
                dominated[j] = true;
            }
            // i dominated by j: j covers superset, j costs <=
            else if (wj <= wi && (ci & ~cj).none())
            {
                dominated[i] = true;
                break;
            }
        }
    }

    std::vector<CandidatePlacement> filtered;
    filtered.reserve(n);
    for (int i = 0; i < n; ++i)
    {
        if (!dominated[i])
            filtered.push_back(m_Candidates[i]);
    }
    m_Candidates = std::move(filtered);
}

// ---- Main step logic ----

AlgoStatus BacktrackingAlgorithm::Step()
{
    if (m_Status != AlgoStatus::Running) return m_Status;
    m_StepsExecuted++;

    if (m_Stack.empty())
    {
        m_Status = AlgoStatus::Finished;
        UpdateTimer();
        return m_Status;
    }

    DFSNode node = m_Stack.top();
    m_Stack.pop();

    m_Current = node.state;

    // If all nodes covered, record solution if better
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

    // If past all candidates, dead end
    if (node.candidateIdx >= (int)m_Candidates.size())
    {
        UpdateTimer();
        return AlgoStatus::Running;
    }

    // PRUNE: coverage feasibility — can remaining candidates cover what's left?
    if (!CanCoverRemaining(node.state, node.candidateIdx))
    {
        m_PrunedCount++;
        UpdateTimer();
        return AlgoStatus::Running;
    }

    // PRUNE: LP-dual lower bound — is it even possible to beat the best?
    if (m_FoundSolution)
    {
        int lb = EstimateLowerBound(node.state, node.candidateIdx);
        if (node.state.totalWeight + lb >= m_BestWeight)
        {
            m_PrunedCount++;
            UpdateTimer();
            return AlgoStatus::Running;
        }
    }

    // Branch: SKIP this candidate
    DFSNode skipNode;
    skipNode.state = node.state;
    skipNode.candidateIdx = node.candidateIdx + 1;
    m_Stack.push(skipNode);

    // Branch: INCLUDE this candidate
    const auto& cand = m_Candidates[node.candidateIdx];

    // Only consider including if it covers at least one NEW node
    if ((cand.nodeCoverage & ~node.state.coveredMask).any())
    {
        DFSNode includeNode;
        includeNode.state = node.state;
        includeNode.state.AddPlacement(*m_Grid, *m_DB, { cand.scannerType, cand.row, cand.col });
        includeNode.candidateIdx = node.candidateIdx + 1;

        // PRUNE: cost bound
        if (includeNode.state.totalWeight < m_BestWeight)
        {
            m_Stack.push(includeNode);
        }
        else
        {
            m_PrunedCount++;
        }
    }
    else
    {
        m_PrunedCount++;
    }

    UpdateTimer();
    return AlgoStatus::Running;
}
