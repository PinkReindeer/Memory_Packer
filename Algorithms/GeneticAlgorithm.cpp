#include "GeneticAlgorithm.h"
#include <algorithm>
#include <cmath>

void GeneticAlgorithmImpl::Init(const GridState& grid, const ScannerDB& db)
{
    m_Grid = &grid;
    m_DB = &db;
    m_Status = AlgoStatus::Running;
    m_StepsExecuted = 0;
    m_FoundSolution = false;
    m_Generation = 0;
    m_BestFitness = 1e18f;
    m_BestIndividual.Clear();
    StartTimer();

    // Pre-generate candidates with cached node coverage
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

    // Remove dominated candidates — dramatically shrinks chromosome length
    RemoveDominatedCandidates();

    int n = (int)m_Candidates.size();

    float inclusionRate = std::min(0.15f, 10.0f / std::max(1, n));

    m_Population.resize(m_PopulationSize);
    for (auto& ind : m_Population)
    {
        ind.genes.resize(n);
        for (int i = 0; i < n; ++i)
        {
            ind.genes[i] = (RandFloat() < inclusionRate);
        }
        Repair(ind);
        EvaluateIndividual(ind);
    }
}

// Fast evaluation using cached bitsets — avoids per-cell AddPlacement loop
void GeneticAlgorithmImpl::EvaluateIndividual(Individual& ind) 
{
    std::bitset<Config::MAX_CELLS> covered;
    covered.reset();
    int weight = 0;

    int n = (int)m_Candidates.size();
    for (int i = 0; i < n; ++i)
    {
        if (ind.genes[i])
        {
            covered |= m_Candidates[i].nodeCoverage;
            weight  += m_DB->weights[m_Candidates[i].scannerType];
        }
    }

    ind.totalWeight = weight;
    ind.coveredNodes = (int)(covered & m_Grid->nodeMask).count();
    ind.complete = (covered & m_Grid->nodeMask) == m_Grid->nodeMask;

    int uncovered = m_Grid->nodeCount - ind.coveredNodes;

    // Fitness: lower is better. Heavy penalty for missing coverage.
    ind.fitness = (float)ind.totalWeight + uncovered * 500.0f;
}

PlacementState GeneticAlgorithmImpl::IndividualToPlacement(const Individual& ind)
{
    PlacementState ps;
    ps.Clear();

    int n = (int)m_Candidates.size();
    for (int i = 0; i < n; ++i)
    {
        if (ind.genes[i])
        {
            const auto& c = m_Candidates[i];
            ps.AddPlacement(*m_Grid, *m_DB, { c.scannerType, c.row, c.col });
        }
    }
    return ps;
}

GeneticAlgorithmImpl::Individual GeneticAlgorithmImpl::Crossover(const Individual& a, const Individual& b)
{
    Individual child;
    int n = (int)a.genes.size();
    child.genes.resize(n);

    // Uniform crossover
    for (int i = 0; i < n; ++i)
    {
        child.genes[i] = (RandFloat() < 0.5f) ? a.genes[i] : b.genes[i];
    }
    return child;
}

void GeneticAlgorithmImpl::Mutate(Individual& ind)
{
    int n = (int)ind.genes.size();
    for (int i = 0; i < n; ++i)
    {
        if (RandFloat() < Config::GA_MUTATION_RATE)
        {
            ind.genes[i] = !ind.genes[i];
        }
    }
}

void GeneticAlgorithmImpl::Repair(Individual& ind)
{
    std::bitset<Config::MAX_CELLS> covered;
    covered.reset();
    int n = (int)m_Candidates.size();

    for (int i = 0; i < n; ++i)
    {
        if (ind.genes[i])
            covered |= m_Candidates[i].nodeCoverage;
    }

    auto uncovered = m_Grid->nodeMask & ~covered;
    if (uncovered.none()) return;  // already complete

    for (int i = 0; i < n; ++i)
    {
        if (ind.genes[i]) continue;  // already included

        int newCov = (int)(m_Candidates[i].nodeCoverage & uncovered).count();
        if (newCov > 0)
        {
            ind.genes[i] = true;
            covered |= m_Candidates[i].nodeCoverage;
            uncovered = m_Grid->nodeMask & ~covered;
            if (uncovered.none()) break;
        }
    }
}

GeneticAlgorithmImpl::Individual& GeneticAlgorithmImpl::TournamentSelect()
{
    // Tournament of 3
    int a = std::rand() % m_PopulationSize;
    int b = std::rand() % m_PopulationSize;
    int c = std::rand() % m_PopulationSize;

    Individual* best = &m_Population[a];
    if (m_Population[b].fitness < best->fitness) best = &m_Population[b];
    if (m_Population[c].fitness < best->fitness) best = &m_Population[c];
    return *best;
}

void GeneticAlgorithmImpl::RemoveDominatedCandidates()
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

            if (wi <= wj && (cj & ~ci).none())
            {
                dominated[j] = true;
            } 
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

AlgoStatus GeneticAlgorithmImpl::Step()
{
    if (m_Status != AlgoStatus::Running) return m_Status;
    m_StepsExecuted++;
    m_Generation++;

    // Evaluate and track best
    for (auto& ind : m_Population)
    {
        if (ind.fitness < m_BestFitness)
        {
            m_BestFitness = ind.fitness;
            m_BestIndividual = IndividualToPlacement(ind);
            if (ind.complete)
            {
                m_FoundSolution = true;
            }
        }
    }

    // Create new generation
    std::vector<Individual> newPop;
    newPop.reserve(m_PopulationSize);

    // Elitism: keep the best individual
    auto bestIt = std::min_element(m_Population.begin(), m_Population.end(),
        [](const Individual& a, const Individual& b) {
            return a.fitness < b.fitness;
        });
    newPop.push_back(*bestIt);

    while ((int)newPop.size() < m_PopulationSize)
    {
        Individual& p1 = TournamentSelect();
        Individual& p2 = TournamentSelect();

        Individual child;
        if (RandFloat() < Config::GA_CROSSOVER_RATE) 
        {
            child = Crossover(p1, p2);
        }
        else
        {
            child = (RandFloat() < 0.5f) ? p1 : p2;
        }

        Mutate(child);
        Repair(child);
        EvaluateIndividual(child);
        newPop.push_back(child);
    }

    m_Population = std::move(newPop);

    if (m_Generation >= m_MaxGenerations)
    {
        m_Status = AlgoStatus::Finished;
    }

    UpdateTimer();
    return m_Status;
}
