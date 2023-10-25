#pragma once

#include <vector>
#include <cstdint>
#include <string>

struct Interval
{
    uint32_t start;
    uint32_t end;

    Interval(uint32_t start, uint32_t end) : start(start), end(end) {}
};

class IntervalSet
{
private:
    std::vector<Interval> intervals;

public:
    IntervalSet()
    {
        this->intervals.push_back(Interval(0, 0));
    }

    bool insert(uint32_t value)
    {
        auto current = this->intervals.begin();
        while (current != this->intervals.cend() && value > current->end)
        {
            current++;
        }

        if (current == this->intervals.cend())
        {
            if ((current - 1)->end == value - 1)
            {
                (current - 1)->end = value;
                return true;
            }

            this->intervals.insert(current, Interval(value, value));
            return true;
        }

        if (value >= current->start)
        {
            return false;
        }

        if (value > (current - 1)->end + 1 && value < current->start - 1)
        {
            this->intervals.insert(current, Interval(value, value));

            return true;
        }

        if ((current - 1)->end == value - 1)
        {
            (current - 1)->end = value;
        }

        if (current->start == value + 1)
        {
            current->start = value;
        }

        if (current->start == (current - 1)->end)
        {
            current->start = (current - 1)->start;
            this->intervals.erase(current - 1);
        }

        return true;
    }

    std::string to_string()
    {
        std::string result;
        for (auto interval : this->intervals)
        {
            result += "[";
            result += std::to_string(interval.start);
            result += ",";
            result += std::to_string(interval.end);
            result += "] ";
        }
        result += "\n";

        return result;
    }
};