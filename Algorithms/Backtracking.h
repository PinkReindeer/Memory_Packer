#pragma once

#include <vector>
#include <stack>

#include "AlgorithmBase.h"

class BacktrackingAlgorithm : public AlgorithmBase
{
public:
    const char* Name() const override { return "Backtracking"; }

    void Init(const GridState& grid, const ScannerDB& db) override;
    AlgoStatus Step() override;

    const PlacementState& CurrentState() const override { return m_Current; }
    const PlacementState& BestState() const override { return m_Best; }

private:
    // A DFS node: "we've decided on placements[0..depth-1], next consider candidateIdx"
    struct DFSNode
    {
        PlacementState state;
        int candidateIdx;   // next candidate to consider
    };

    // All candidate placements (pre-generated)
    struct CandidatePlacement
    {
        int scannerType;
        int row, col;
        std::bitset<Config::MAX_CELLS> nodeCoverage; // intersection with nodeMask
    };

    std::vector<CandidatePlacement> m_Candidates;
    std::stack<DFSNode> m_Stack;
    PlacementState m_Current;  // current working state (for visualization)
    PlacementState m_Best;     // best complete solution found
    int m_BestWeight = INT_MAX;
    int m_PrunedCount = 0;

    // Suffix coverage: m_SuffixCoverage[i] = union of nodeCoverage for candidates[i..end]
    std::vector<std::bitset<Config::MAX_CELLS>> m_SuffixCoverage;

    void PrecomputeSuffixCoverage();
    bool CanCoverRemaining(const PlacementState& state, int fromIdx) const;
    int  EstimateLowerBound(const PlacementState& state, int fromIdx) const;
    void RemoveDominatedCandidates();
};
