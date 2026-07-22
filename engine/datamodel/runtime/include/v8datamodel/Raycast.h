#pragma once

#include "v8datamodel/InteractionEnums.h"
#include "v8tree/Instance.h"
#include "util/PartMaterial.h"
#include "g3d/Vector3.h"

#include <string>

namespace RBX {

struct RaycastParams
{
    RaycastParams()
        : filterDescendantsInstances(new Instances())
        , filterType(Enums::RAYCAST_FILTER_EXCLUDE)
        , ignoreWater(false)
        , collisionGroup("Default")
        , respectCanCollide(false)
        , bruteForceAllSlow(false)
    {
    }

    bool operator==(const RaycastParams& other) const
    {
        return filterDescendantsInstances == other.filterDescendantsInstances &&
            filterType == other.filterType && ignoreWater == other.ignoreWater &&
            collisionGroup == other.collisionGroup &&
            respectCanCollide == other.respectCanCollide &&
            bruteForceAllSlow == other.bruteForceAllSlow;
    }

    shared_ptr<const Instances> filterDescendantsInstances;
    Enums::RaycastFilterType filterType;
    bool ignoreWater;
    std::string collisionGroup;
    bool respectCanCollide;
    bool bruteForceAllSlow;
};

struct OverlapParams
{
    OverlapParams()
        : filterDescendantsInstances(new Instances())
        , filterType(Enums::RAYCAST_FILTER_EXCLUDE)
        , maxParts(0)
        , collisionGroup("Default")
        , respectCanCollide(false)
        , bruteForceAllSlow(false)
    {
    }

    bool operator==(const OverlapParams& other) const
    {
        return filterDescendantsInstances == other.filterDescendantsInstances &&
            filterType == other.filterType && maxParts == other.maxParts &&
            collisionGroup == other.collisionGroup &&
            respectCanCollide == other.respectCanCollide &&
            bruteForceAllSlow == other.bruteForceAllSlow;
    }

    shared_ptr<const Instances> filterDescendantsInstances;
    Enums::RaycastFilterType filterType;
    int maxParts;
    std::string collisionGroup;
    bool respectCanCollide;
    bool bruteForceAllSlow;
};

struct RaycastResult
{
    RaycastResult()
        : position(G3D::Vector3::zero())
        , normal(G3D::Vector3::zero())
        , material(AIR_MATERIAL)
        , distance(0.0f)
    {
    }

    bool operator==(const RaycastResult& other) const
    {
        return instance == other.instance && position == other.position &&
            normal == other.normal && material == other.material &&
            distance == other.distance;
    }

    shared_ptr<Instance> instance;
    G3D::Vector3 position;
    G3D::Vector3 normal;
    PartMaterial material;
    float distance;
};

} // namespace RBX
