#pragma once

#include <vector>
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
    // Each individual = a selection of candidates (on/off per candidate)
    struct Individual
    {
        std::vector<bool> genes;   // genes[i] = true means candidate i is placed
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

    // GA operators
    void Evaluate(Individual& ind);
    void Repair(Individual& ind);
    void Trim(Individual& ind);
    Individual Crossover(const Individual& a, const Individual& b);
    void Mutate(Individual& ind);
    int TournamentSelect();
    PlacementState ToPlacement(const Individual& ind);

    float RandFloat() const { return (float)std::rand() / (float)RAND_MAX; }
};
