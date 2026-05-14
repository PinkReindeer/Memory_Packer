#pragma once

#include <vector>
#include <string>

struct ChartEntry
{
    std::string name;
    float cost;
    float timeMs;
    bool foundSolution;
};

class ChartWidget
{
public:
    void Clear() { m_Entries.clear(); }

    void AddEntry(const std::string& name, float cost, float timeMs, bool found)
    {
        // Replace if same algorithm name exists
        for (auto& e : m_Entries)
        {
            if (e.name == name)
            {
                e.cost = cost;
                e.timeMs = timeMs;
                e.foundSolution = found;
                return;
            }
        }
        m_Entries.push_back({ name, cost, timeMs, found });
    }

    // Draw the chart at given rect
    void Draw(int x, int y, int w, int h) const;

    int EntryCount() const { return (int)m_Entries.size(); }

private:
    std::vector<ChartEntry> m_Entries;
};
