#pragma once

#include <vector>
#include <stack>

#include "AlgorithmBase.h"

class BranchAndBoundAlgorithm : public AlgorithmBase
{
public:
    const char* Name() const override { return "Branch & Bound"; }

    void Init(const GridState& grid, const ScannerDB& db) override;
    AlgoStatus Step() override;

    const PlacementState& CurrentState() const override { return m_Current; }
    const PlacementState& BestState() const override { return m_Best; }

    int GetPrunedCount() const { return m_PrunedCount; }

private:
    // A candidate = one possible scanner placement
    struct Candidate
    {
        int scannerType;
        int row, col;
        std::bitset<Config::MAX_CELLS> nodeCoverage;
    };

    // A DFS node = search state snapshot
    struct DFSNode
    {
        PlacementState state;
        int candidateIdx;
    };

    std::vector<Candidate> m_Candidates;
    std::stack<DFSNode> m_Stack;
    PlacementState m_Current;
    PlacementState m_Best;
    int m_BestWeight = INT_MAX;
    int m_PrunedCount = 0;

    // Simple lower bound on remaining cost
    int EstimateLowerBound(const PlacementState& state) const;
};
