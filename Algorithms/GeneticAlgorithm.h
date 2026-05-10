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

    const PlacementState& CurrentState() const override { return m_BestIndividual; }
    const PlacementState& BestState() const override { return m_BestIndividual; }

    int GetGeneration() const { return m_Generation; }

private:
    // An individual is a bitmask of which candidates are "selected"
    struct Individual
    {
        std::vector<bool> genes;  // genes[i] = true means candidate i is placed
        float fitness = 0.0f;
        int totalWeight = 0;
        int coveredNodes = 0;
        bool complete = false;
    };

    struct CandidatePlacement
    {
        int scannerType;
        int row, col;
        std::bitset<Config::MAX_CELLS> nodeCoverage;  // cached node coverage
    };

    std::vector<CandidatePlacement> m_Candidates;
    std::vector<Individual> m_Population;

    PlacementState m_BestIndividual;
    float m_BestFitness = 1e18f;
    int m_Generation = 0;
    int m_MaxGenerations = Config::GA_MAX_GENERATIONS;
    int m_PopulationSize = Config::GA_POPULATION_SIZE;

    // Helper methods
    void EvaluateIndividual(Individual& ind);
    PlacementState IndividualToPlacement(const Individual& ind);
    Individual Crossover(const Individual& a, const Individual& b);
    void Mutate(Individual& ind);
    void Repair(Individual& ind);
    Individual& TournamentSelect();
    void RemoveDominatedCandidates();
    float RandFloat() const { return (float)std::rand() / (float)RAND_MAX; }
};
