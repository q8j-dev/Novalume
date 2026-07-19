#pragma once

#include "TextureRef.h"
#include "util/ContentId.h"

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include <rbx/threadsafe.h>

namespace RBX
{
    class ThreadPool;
}

namespace RBX
{
namespace Graphics
{

class VisualEngine;
class Image;

struct TextureManagerStats
{
    unsigned int queuedCount;

    unsigned int totalBudget;

    unsigned int liveCount;
    unsigned int liveSize;

    unsigned int orphanedCount;
    unsigned int orphanedSize;
};

class TextureManager
{
public:
    enum Fallback
	{
        Fallback_None,

        Fallback_White,
        Fallback_Gray,
        Fallback_Black,
        Fallback_BlackTransparent,

        Fallback_NormalMap,
        Fallback_Reflection,

        Fallback_Count
	};

    TextureManager(VisualEngine* visualEngine);
    ~TextureManager();

    void processPendingRequests();
	void cancelPendingRequests();

    void garbageCollectIncremental();
    void garbageCollectFull();

    void reloadImage(const ContentId& id, const std::string& context = "");
    TextureRef load(const ContentId& id, Fallback fallback, const std::string& context = "");

	const std::shared_ptr<Texture>& getFallbackTexture(Fallback fallback) { return fallbackTextures[fallback]; }
    bool isFallbackTexture(const std::shared_ptr<Texture>& tex);

	bool isQueueEmpty() const { return outstandingRequests == 0; }

	TextureManagerStats getStatistics() const;

private:
    struct LoadedImage
	{
		std::string context;
        ContentId id;

        std::shared_ptr<Image> image;
        ImageInfo info;

		Time::Interval loadTime;
	};

    struct TextureData
	{
        ContentId id;

        std::vector<TextureRef> external;
        TextureRef object;

        bool orphaned;
        TextureData* orphanedPrev;
        TextureData* orphanedNext;

        TextureData();

        TextureRef addExternalRef(const std::shared_ptr<Texture>& fallback);

        void removeUnusedExternalRefs();

        void updateAllRefsToLoaded(const std::shared_ptr<Texture>& texture, const ImageInfo& info);
        void updateAllRefsToFailed();
        void updateAllRefsToWaiting();
	};

    VisualEngine* visualEngine;

    typedef boost::unordered_map<ContentId, TextureData> Textures;
	Textures textures;

    std::shared_ptr<Texture> fallbackTextures[Fallback_Count];

    boost::shared_ptr<rbx::safe_queue<LoadedImage> > pendingImages;
    boost::shared_ptr<ThreadPool> loadingPool;
    boost::unordered_set<ContentId> pendingReloads;
    unsigned int outstandingRequests;

    unsigned int liveCount;
    unsigned int liveSize;

    unsigned int orphanedCount;
    unsigned int orphanedSize;

    TextureData* orphanedHead;
    TextureData* orphanedTail;

    size_t gcSizeLast;
    ContentId gcKeyNext;

    unsigned int totalSizeBudget;

    bool loadAsync(const ContentId& id, const std::string& context);

    void orphanUnusedTextures(size_t visitCount);
    void collectOrphanedTextures(unsigned int maxOrphanedSize);

    void addToOrphanedTail(TextureData* data);
    void removeFromOrphaned(TextureData* data);

    std::shared_ptr<Texture> createSingleColorTexture(unsigned char r, unsigned char g, unsigned char b, unsigned char a, bool cube = false);
    std::shared_ptr<Texture> createTexture(const Image& image);

	static void loadImageHttpCallback(const boost::weak_ptr<ThreadPool>& loadingPool, const boost::weak_ptr<rbx::safe_queue<LoadedImage> >& pendingImages, const boost::shared_ptr<const std::string>& content, const ContentId& id, unsigned int maxTextureSize, unsigned int flags, const std::string& context);

    static void loadImageFile(const boost::shared_ptr<rbx::safe_queue<LoadedImage> >& pendingImages, const ContentId& id, const ContentId& loadId, unsigned int maxTextureSize, unsigned int flags, bool useRetina, const std::string& context = "");
	static void loadImageLocalContent(const boost::shared_ptr<rbx::safe_queue<LoadedImage> >& pendingImages, const ContentId& id, const std::string& path, unsigned int maxTextureSize, unsigned int flags, const std::string& context = "");
	static void loadImageHttp(const boost::shared_ptr<rbx::safe_queue<LoadedImage> >& pendingImages, const ContentId& id, const boost::shared_ptr<const std::string>& content, unsigned int maxTextureSize, unsigned int flags, const std::string& context = "");
    static void loadImage(const boost::shared_ptr<rbx::safe_queue<LoadedImage> >& pendingImages, const ContentId& id, std::istream& stream, unsigned int maxTextureSize, unsigned int flags, int scale, const std::string& context = "");
    static void loadImageError(const boost::shared_ptr<rbx::safe_queue<LoadedImage> >& pendingImages, const ContentId& id, const char* error, const std::string& context);
};

}
}
