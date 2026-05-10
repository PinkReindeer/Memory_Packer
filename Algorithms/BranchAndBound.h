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
    struct DFSNode
    {
        PlacementState state;
        int candidateIdx;
    };

    struct CandidatePlacement
    {
        int scannerType;
        int row, col;
        std::bitset<Config::MAX_CELLS> nodeCoverage;  // which nodes this covers
    };

    std::vector<CandidatePlacement> m_Candidates;

    // Union of all candidate coverages from index i onward
    std::vector<std::bitset<Config::MAX_CELLS>> m_SuffixCoverage;

    std::stack<DFSNode> m_Stack;
    PlacementState m_Current;
    PlacementState m_Best;
    int m_BestWeight = INT_MAX;
    int m_PrunedCount = 0;

    void PrecomputeSuffixCoverage();
    bool CanCoverRemaining(const PlacementState& state, int fromIdx) const;
    int  EstimateLowerBound(const PlacementState& state, int fromIdx) const;
    void RemoveDominatedCandidates();
};
