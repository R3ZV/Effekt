#pragma once

#include <vector>

class AMPFilter {
   public:
    virtual ~AMPFilter() = default;
    virtual auto apply(const std::vector<float>& input)
        -> std::vector<float> = 0;
};
