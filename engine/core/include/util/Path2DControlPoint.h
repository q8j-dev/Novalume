#pragma once

#include "util/UDim.h"

namespace RBX {

struct Path2DControlPoint
{
    UDim2 position;
    UDim2 leftTangent;
    UDim2 rightTangent;

    Path2DControlPoint() = default;
    explicit Path2DControlPoint(const UDim2& position)
        : position(position)
    {
    }
    Path2DControlPoint(const UDim2& position, const UDim2& leftTangent,
        const UDim2& rightTangent)
        : position(position)
        , leftTangent(leftTangent)
        , rightTangent(rightTangent)
    {
    }

    bool operator==(const Path2DControlPoint& other) const
    {
        return position == other.position && leftTangent == other.leftTangent &&
            rightTangent == other.rightTangent;
    }
    bool operator!=(const Path2DControlPoint& other) const { return !(*this == other); }
};

} // namespace RBX
