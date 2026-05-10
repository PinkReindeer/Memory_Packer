#pragma once

#include <vector>
#include <limits>

#include "AlgorithmBase.h"

class GreedyAlgorithm : public AlgorithmBase
{
public:
    const char* Name() const override { return "Greedy"; }

    void Init(const GridState& grid, const ScannerDB& db) override;
    AlgoStatus Step() override;

    const PlacementState& CurrentState() const override { return m_Current; }
    const PlacementState& BestState() const override { return m_Current; }

private:
    PlacementState m_Current;

    // Candidate placement with pre-computed metrics
    struct Candidate
    {
        int scannerType;
        int row, col;
        int newNodes;
        float ratio;  // weight / newNodes
    };

    Candidate m_BestCandidate;
};
