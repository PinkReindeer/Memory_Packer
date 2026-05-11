#pragma once

#include <vector>
#include <bitset>
#include <cstdlib>

#include "AlgorithmBase.h"

class GeneticAlgorithmImpl : public AlgorithmBase
{
public:
    const char* Name() const override { return "Genetic Algorithm"; }

    void Init(const GridState& grid, const ScannerDB& db) override;
    AlgoStatus Step() override;

    const PlacementState& CurrentState() const override { return m_BestPlacement; }
    const PlacementState& BestState() const override { return m_BestPlacement; }

    int GetGeneration() const { return m_Generation; }

private:
    // Max candidates we support (generous upper bound)
    static constexpr int MAX_CANDIDATES = 2048;

    // Each individual = a selection of candidates (on/off per candidate)
    struct Individual
    {
        std::bitset<MAX_CANDIDATES> genes; // compact fixed-size bitset
        std::bitset<Config::MAX_CELLS> coverage; // cached node coverage
        int totalWeight = 0;
        int coveredNodes = 0;
        bool complete = false;     // true if all nodes covered
        float fitness = 0.0f;     // lower is better
    };

    // A candidate = one possible scanner placement
    struct Candidate
    {
        int scannerType;
        int row, col;
        std::bitset<Config::MAX_CELLS> nodeCoverage;
    };

    std::vector<Candidate> m_Candidates;
    std::vector<Individual> m_Population;
    PlacementState m_BestPlacement;       // best solution for rendering
    float m_BestFitness = 1e18f;
    int m_Generation = 0;
    int m_MaxGenerations = Config::GA_MAX_GENERATIONS;
    int m_PopulationSize = Config::GA_POPULATION_SIZE;
    int m_CandidateCount = 0;             // actual number of candidates

    // GA operators
    void RebuildCoverage(Individual& ind); // recompute coverage from genes
    void Evaluate(Individual& ind);
    void Repair(Individual& ind);
    void Trim(Individual& ind);
    Individual Crossover(const Individual& a, const Individual& b);
    void Mutate(Individual& ind);
    int TournamentSelect();
    PlacementState ToPlacement(const Individual& ind);

    float RandFloat() const { return (float)std::rand() / (float)RAND_MAX; }
};
