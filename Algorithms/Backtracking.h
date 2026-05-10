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
    // A candidate = one possible scanner placement on the grid
    struct Candidate
    {
        int scannerType;
        int row, col;
        std::bitset<Config::MAX_CELLS> nodeCoverage;  // which nodes this covers
    };

    // A DFS node = a snapshot of the search state
    struct DFSNode
    {
        PlacementState state;       // placements made so far
        int candidateIdx;           // next candidate to decide on (include or skip)
    };

    std::vector<Candidate> m_Candidates;   // all possible placements
    std::stack<DFSNode> m_Stack;           // DFS exploration stack
    PlacementState m_Current;              // current working state (for display)
    PlacementState m_Best;                 // best complete solution found
    int m_BestWeight = INT_MAX;            // weight of best solution
};
