#include <algorithm>
#include <cmath>

#include "GeneticAlgorithm.h"

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

    m_CandidateCount = (int)m_Candidates.size();

    // Clamp to MAX_CANDIDATES to stay within bitset bounds
    if (m_CandidateCount > MAX_CANDIDATES)
        m_CandidateCount = MAX_CANDIDATES;

    // Create random population with low inclusion rate
    float inclusionRate = std::min(0.15f, 10.0f / std::max(1, m_CandidateCount));

    m_Population.resize(m_PopulationSize);
    for (auto& ind : m_Population)
    {
        ind.genes.reset();
        ind.coverage.reset();
        for (int i = 0; i < m_CandidateCount; ++i)
        {
            if (RandFloat() < inclusionRate)
                ind.genes.set(i);
        }

        RebuildCoverage(ind);
        Repair(ind);     // ensure all nodes are covered
        Trim(ind);       // remove redundant scanners
        Evaluate(ind);   // compute fitness
    }
}

// --- RebuildCoverage: recompute coverage bitset from genes ---
void GeneticAlgorithmImpl::RebuildCoverage(Individual& ind)
{
    ind.coverage.reset();
    for (int i = 0; i < m_CandidateCount; ++i)
    {
        if (ind.genes.test(i))
            ind.coverage |= m_Candidates[i].nodeCoverage;
    }
}

// --- Evaluate: compute fitness from cached coverage ---
void GeneticAlgorithmImpl::Evaluate(Individual& ind)
{
    int weight = 0;
    for (int i = 0; i < m_CandidateCount; ++i)
    {
        if (ind.genes.test(i))
            weight += m_DB->weights[m_Candidates[i].scannerType];
    }

    ind.totalWeight = weight;
    ind.coveredNodes = (int)(ind.coverage & m_Grid->nodeMask).count();
    ind.complete = (ind.coverage & m_Grid->nodeMask) == m_Grid->nodeMask;

    // Fitness: lower is better. Heavy penalty for missing nodes.
    int uncovered = m_Grid->nodeCount - ind.coveredNodes;
    ind.fitness = (float)weight + uncovered * 500.0f;
}

// --- Repair: greedily add candidates until all nodes covered ---
void GeneticAlgorithmImpl::Repair(Individual& ind)
{
    // Use the cached coverage directly
    auto uncovered = m_Grid->nodeMask & ~ind.coverage;
    while (uncovered.any())
    {
        int bestIdx = -1;
        int bestNewCov = 0;

        for (int i = 0; i < m_CandidateCount; ++i)
        {
            if (ind.genes.test(i)) continue;
            int newCov = (int)(m_Candidates[i].nodeCoverage & uncovered).count();
            if (newCov > bestNewCov)
            {
                bestNewCov = newCov;
                bestIdx = i;
            }
        }

        if (bestIdx < 0) break;  // no candidate can help

        ind.genes.set(bestIdx);
        ind.coverage |= m_Candidates[bestIdx].nodeCoverage;
        uncovered = m_Grid->nodeMask & ~ind.coverage;
    }
}

// --- Trim: remove scanners that aren't needed ---
void GeneticAlgorithmImpl::Trim(Individual& ind)
{
    // Try removing each active gene; keep removal if coverage stays complete
    for (int i = m_CandidateCount - 1; i >= 0; --i)
    {
        if (!ind.genes.test(i)) continue;

        auto coverageWithout = ind.coverage & ~m_Candidates[i].nodeCoverage;
        auto candidateNodes = m_Candidates[i].nodeCoverage & m_Grid->nodeMask;
        auto nodesOnlyFromThis = candidateNodes & ~coverageWithout;

        if (nodesOnlyFromThis.none())
        {
            ind.genes.reset(i);

            std::bitset<Config::MAX_CELLS> rebuilt;
            for (int j = 0; j < m_CandidateCount; ++j)
            {
                if (ind.genes.test(j))
                    rebuilt |= m_Candidates[j].nodeCoverage;
            }

            if ((rebuilt & m_Grid->nodeMask) == m_Grid->nodeMask)
            {
                // Redundant — keep it removed, update cached coverage
                ind.coverage = rebuilt;
            }
            else
            {
                // Not redundant — restore
                ind.genes.set(i);
            }
        }
        // else: clearly not redundant, skip
    }
}

// --- Crossover: single-point crossover ---
GeneticAlgorithmImpl::Individual GeneticAlgorithmImpl::Crossover(
    const Individual& a, const Individual& b)
{
    Individual child;
    child.genes.reset();

    int point = std::rand() % m_CandidateCount;  // random crossover point
    for (int i = 0; i < m_CandidateCount; ++i)
    {
        bool val = (i < point) ? a.genes.test(i) : b.genes.test(i);
        if (val) child.genes.set(i);
    }

    RebuildCoverage(child);
    return child;
}

// --- Mutate: random bit-flip ---
void GeneticAlgorithmImpl::Mutate(Individual& ind)
{
    for (int i = 0; i < m_CandidateCount; ++i)
    {
        if (RandFloat() < Config::GA_MUTATION_RATE)
            ind.genes.flip(i);
    }
    // Coverage will be rebuilt by Repair/RebuildCoverage
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

    for (int i = 0; i < m_CandidateCount; ++i)
    {
        if (ind.genes.test(i))
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
        RebuildCoverage(child);  // rebuild after mutation
        Repair(child);
        Trim(child);
        Evaluate(child);
        newPop.push_back(std::move(child));
    }

    m_Population = std::move(newPop);

    // Stop after max generations
    if (m_Generation >= m_MaxGenerations)
        m_Status = AlgoStatus::Finished;

    UpdateTimer();
    return m_Status;
}
