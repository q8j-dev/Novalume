#pragma once

#include "GfxBase/ThumbnailTextureProvider.h"
#include "rbx/Boost.hpp"

namespace RBX {
class DataModel;
}

namespace rbx::player {

class LocalPlayerThumbnailProvider final : public RBX::ThumbnailSceneProvider
{
public:
    explicit LocalPlayerThumbnailProvider(
        const boost::shared_ptr<RBX::DataModel>& dataModel);

    bool resolveThumbnailScene(const RBX::ThumbnailSceneRequest& request,
        RBX::ViewportTextureRequest& viewportRequest) override;

private:
    boost::weak_ptr<RBX::DataModel> dataModel;
};

} // namespace rbx::player
