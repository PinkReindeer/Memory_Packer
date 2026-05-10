#include "GeneticAlgorithm.h"
#include <algorithm>
#include <cmath>

// ============================================================
// GENETIC ALGORITHM: Population-based evolutionary search
//
// Each individual is a binary chromosome where genes[i] = true
// means "place candidate i on the grid".
//
// Pipeline per generation:
//   1. Select parents via tournament selection
//   2. Single-point crossover to create children
//   3. Random bit-flip mutation
//   4. Repair: greedily add scanners until all nodes covered
//   5. Trim: remove any scanner that isn't needed
//   6. Evaluate fitness (weight + penalty for uncovered nodes)
//
// Elitism: the best individual always survives to next generation.
// ============================================================

void GeneticAlgorithmImpl::Init(const GridState& grid, const ScannerDB& db)
{
    m_Grid = &grid;
    m_DB = &db;
    m_Status = AlgoStatus::Running;
    m_StepsExecuted = 0;
    m_FoundSolution = false;
    m_Generation = 0;
    m_BestFitness = 1e18f;
    m_BestPlacement.Clear();
    StartTimer();

    // Generate all candidate placements that cover at least 1 node
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

    int n = (int)m_Candidates.size();

    // Create random population with low inclusion rate
    float inclusionRate = std::min(0.15f, 10.0f / std::max(1, n));

    m_Population.resize(m_PopulationSize);
    for (auto& ind : m_Population)
    {
        ind.genes.resize(n);
        for (int i = 0; i < n; ++i)
            ind.genes[i] = (RandFloat() < inclusionRate);

        Repair(ind);     // ensure all nodes are covered
        Trim(ind);       // remove redundant scanners
        Evaluate(ind);   // compute fitness
    }
}

// --- Evaluate: compute fitness from genes ---
void GeneticAlgorithmImpl::Evaluate(Individual& ind)
{
    std::bitset<Config::MAX_CELLS> covered;
    int weight = 0;

    int n = (int)m_Candidates.size();
    for (int i = 0; i < n; ++i)
    {
        if (ind.genes[i])
        {
            covered |= m_Candidates[i].nodeCoverage;
            weight += m_DB->weights[m_Candidates[i].scannerType];
        }
    }

    ind.totalWeight = weight;
    ind.coveredNodes = (int)(covered & m_Grid->nodeMask).count();
    ind.complete = (covered & m_Grid->nodeMask) == m_Grid->nodeMask;

    // Fitness: lower is better. Heavy penalty for missing nodes.
    int uncovered = m_Grid->nodeCount - ind.coveredNodes;
    ind.fitness = (float)weight + uncovered * 500.0f;
}

// --- Repair: greedily add candidates until all nodes covered ---
void GeneticAlgorithmImpl::Repair(Individual& ind)
{
    std::bitset<Config::MAX_CELLS> covered;
    int n = (int)m_Candidates.size();

    // Compute current coverage
    for (int i = 0; i < n; ++i)
    {
        if (ind.genes[i])
            covered |= m_Candidates[i].nodeCoverage;
    }

    // Add candidates that cover the most new nodes
    auto uncovered = m_Grid->nodeMask & ~covered;
    while (uncovered.any())
    {
        int bestIdx = -1;
        int bestNewCov = 0;

        for (int i = 0; i < n; ++i)
        {
            if (ind.genes[i]) continue;
            int newCov = (int)(m_Candidates[i].nodeCoverage & uncovered).count();
            if (newCov > bestNewCov)
            {
                bestNewCov = newCov;
                bestIdx = i;
            }
        }

        if (bestIdx < 0) break;  // no candidate can help

        ind.genes[bestIdx] = true;
        covered |= m_Candidates[bestIdx].nodeCoverage;
        uncovered = m_Grid->nodeMask & ~covered;
    }
}

// --- Trim: remove scanners that aren't needed ---
void GeneticAlgorithmImpl::Trim(Individual& ind)
{
    int n = (int)m_Candidates.size();

    // Try removing each active gene; keep removal if coverage stays complete
    for (int i = n - 1; i >= 0; --i)
    {
        if (!ind.genes[i]) continue;

        // Compute coverage WITHOUT this candidate
        std::bitset<Config::MAX_CELLS> covered;
        for (int j = 0; j < n; ++j)
        {
            if (j != i && ind.genes[j])
                covered |= m_Candidates[j].nodeCoverage;
        }

        // If still complete, this candidate is redundant
        if ((covered & m_Grid->nodeMask) == m_Grid->nodeMask)
            ind.genes[i] = false;
    }
}

// --- Crossover: single-point crossover ---
GeneticAlgorithmImpl::Individual GeneticAlgorithmImpl::Crossover(
    const Individual& a, const Individual& b)
{
    Individual child;
    int n = (int)a.genes.size();
    child.genes.resize(n);

    int point = std::rand() % n;  // random crossover point
    for (int i = 0; i < n; ++i)
        child.genes[i] = (i < point) ? a.genes[i] : b.genes[i];

    return child;
}

// --- Mutate: random bit-flip ---
void GeneticAlgorithmImpl::Mutate(Individual& ind)
{
    int n = (int)ind.genes.size();
    for (int i = 0; i < n; ++i)
    {
        if (RandFloat() < Config::GA_MUTATION_RATE)
            ind.genes[i] = !ind.genes[i];
    }
}

// --- Tournament selection: pick 3 random, return best index ---
int GeneticAlgorithmImpl::TournamentSelect()
{
    int a = std::rand() % m_PopulationSize;
    int b = std::rand() % m_PopulationSize;
    int c = std::rand() % m_PopulationSize;

    int best = a;
    if (m_Population[b].fitness < m_Population[best].fitness) best = b;
    if (m_Population[c].fitness < m_Population[best].fitness) best = c;
    return best;
}

// --- Convert individual to PlacementState for rendering ---
PlacementState GeneticAlgorithmImpl::ToPlacement(const Individual& ind)
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

// --- One Step = One Generation ---
AlgoStatus GeneticAlgorithmImpl::Step()
{
    if (m_Status != AlgoStatus::Running) return m_Status;
    m_StepsExecuted++;
    m_Generation++;

    // Track the best individual
    for (auto& ind : m_Population)
    {
        if (ind.fitness < m_BestFitness)
        {
            m_BestFitness = ind.fitness;
            m_BestPlacement = ToPlacement(ind);
            if (ind.complete) m_FoundSolution = true;
        }
    }

    // --- Create next generation ---
    std::vector<Individual> newPop;
    newPop.reserve(m_PopulationSize);

    // Elitism: keep the best individual unchanged
    auto bestIt = std::min_element(m_Population.begin(), m_Population.end(),
        [](const Individual& a, const Individual& b) {
            return a.fitness < b.fitness;
        });
    newPop.push_back(*bestIt);

    // Fill the rest with offspring
    while ((int)newPop.size() < m_PopulationSize)
    {
        int p1 = TournamentSelect();
        int p2 = TournamentSelect();

        Individual child;
        if (RandFloat() < Config::GA_CROSSOVER_RATE)
            child = Crossover(m_Population[p1], m_Population[p2]);
        else
            child = (RandFloat() < 0.5f) ? m_Population[p1] : m_Population[p2];

        Mutate(child);
        Repair(child);
        Trim(child);
        Evaluate(child);
        newPop.push_back(child);
    }

    m_Population = std::move(newPop);

    // Stop after max generations
    if (m_Generation >= m_MaxGenerations)
        m_Status = AlgoStatus::Finished;

    UpdateTimer();
    return m_Status;
}
