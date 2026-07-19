#include "VisualEngine.h"

#include "GfxBase/RenderCaps.h"
#include "GfxBase/RenderStats.h"
#include "GfxBase/FrameRateManager.h"

#include "GfxCore/Device.h"

#include "GlobalShaderData.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "TextureCompositor.h"
#include "LightGrid.h"
#include "Water.h"
#include "EmitterShared.h"
#include "MaterialGenerator.h"
#include "SceneManager.h"
#include "TypesetterBitmap.h"
#include "TypesetterDynamic.h"
#include "AdornRender.h"
#include "VertexStreamer.h"
#include "SceneUpdater.h"
#include "SmoothCluster.h"
#include "TextureAtlas.h"
#include "ViewportRenderer.h"

#include "GfxBase/Typesetter.h"

#include "v8datamodel/DataModel.h"
#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/MeshContentProvider.h"
#include "v8datamodel/SolidModelContentProvider.h"
#include "v8datamodel/Camera.h"
#include "v8datamodel/MegaCluster.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/Lighting.h"
#include "rbx/SystemUtil.h"

LOGGROUP(Graphics)

FASTFLAGVARIABLE(CancelPendingTextureLoads, true)
FASTFLAG(UseDynamicTypesetterUTF8)

FASTFLAG(CameraVR)

namespace RBX
{
namespace Graphics
{

namespace {
const char* dynamicFontPath(Text::Font font)
{
    static const char* const paths[] = {
        "fonts/arial.ttf", "fonts/arial.ttf", "fonts/arialbd.ttf",
        "fonts/SourceSansPro-Regular.ttf", "fonts/SourceSansPro-Bold.ttf",
        "fonts/SourceSansPro-Light.ttf", "fonts/SourceSansPro-It.ttf",
        "fonts/AccanthisADFStd-Regular.otf", "fonts/Guru-Regular.otf",
        "fonts/ComicNeue-Angular-Bold.ttf", "fonts/Inconsolata-Regular.ttf",
        "fonts/HWYGOTH.ttf", "fonts/zekton_rg.ttf",
        "fonts/PressStart2P-Regular.ttf", "fonts/Balthazar-Regular.ttf",
        "fonts/RomanAntique.otf", "fonts/SourceSansPro-Semibold.ttf",
        "fonts/Montserrat-Regular.ttf", "fonts/Montserrat-Medium.ttf",
        "fonts/Montserrat-Bold.ttf", "fonts/Montserrat-Black.ttf",
        "fonts/AmaticSC-Regular.ttf", "fonts/Bangers-Regular.ttf",
        "fonts/Creepster-Regular.ttf", "fonts/DenkOne-Regular.ttf",
        "fonts/Fondamento-Regular.ttf", "fonts/FredokaOne-Regular.ttf",
        "fonts/GrenzeGotisch-Regular.ttf", "fonts/IndieFlower-Regular.ttf",
        "fonts/JosefinSans-Regular.ttf", "fonts/Jura-Regular.ttf",
        "fonts/Kalam-Regular.ttf", "fonts/LuckiestGuy-Regular.ttf",
        "fonts/Merriweather-Regular.ttf", "fonts/Michroma-Regular.ttf",
        "fonts/Nunito-Regular.ttf", "fonts/Oswald-Regular.ttf",
        "fonts/PatrickHand-Regular.ttf", "fonts/PermanentMarker-Regular.ttf",
        "fonts/Roboto-Regular.ttf", "fonts/RobotoCondensed-Regular.ttf",
        "fonts/RobotoMono-Regular.ttf", "fonts/Sarpanch-Regular.ttf",
        "fonts/SpecialElite-Regular.ttf", "fonts/TitilliumWeb-Regular.ttf",
        "fonts/Ubuntu-Regular.ttf", "fonts/BuilderSans-Regular.otf",
        "fonts/BuilderSans-Medium.otf", "fonts/BuilderSans-Bold.otf",
        "fonts/BuilderSans-ExtraBold.otf", "fonts/Arimo-Regular.ttf",
        "fonts/Arimo-Bold.ttf"
    };
    if (font >= Text::FONT_LEGACY && font <= Text::FONT_ARIMOBOLD)
        return paths[font];
    if (font == Text::FONT_BUILDER_ICONS_REGULAR)
        return "LuaPackages/Packages/_Index/BuilderIcons/BuilderIcons/Font/BuilderIcons-Regular.ttf";
    if (font == Text::FONT_BUILDER_ICONS_FILLED)
        return "LuaPackages/Packages/_Index/BuilderIcons/BuilderIcons/Font/BuilderIcons-Filled.ttf";
    return "fonts/SourceSansPro-Regular.ttf";
}
}

VisualEngine::VisualEngine(Device* device, CRenderSettings* settings)
	: device(device)
	, viewWidth(0)
	, viewHeight(0)
    , settings(settings)
    , contentProvider(0)
    , lighting(0)
    , meshContentProvider(0)
{
    renderStats.reset(new RenderStats());
    renderCaps.reset(new RenderCaps("Unknown", SystemUtil::getVideoMemory()));

    FASTLOG1(FLog::Graphics, "Video memory size: %lld", SystemUtil::getVideoMemory());

    GlobalShaderData::define(device);

    // load shaders
    shaderManager.reset(new ShaderManager(this));
	shaderManager->loadShaders(ContentProvider::assetFolder() + "../shaders", device->getShadingLanguage(), /* consoleOutput= */ false);
    
    // initialize texture manager
    textureManager.reset(new TextureManager(this));
    
    // create light grid
    LightGrid::TextureMode gridTextureMode =
        device->getShadingLanguage() == "glsles"
        ? LightGrid::Texture_2D
        :
            (device->getCaps().supportsShaders && device->getCaps().supportsTexture3D)
            ? LightGrid::Texture_3D
            : LightGrid::Texture_None;

    // Note: if there is no texture support we still create a 4x4x4 texture
    // We would really like the height to be constant since anything above the height does not cast shadows, so this affects ceiling heights
    // Having XZ size of 2x2 does not really let us have an efficient sliding window - a character is often too close to one of the edges
    // 3x3 is an option, but it's easier to go with 4x4 for now.
    Vector3int32 gridSize =
        (gridTextureMode != LightGrid::Texture_None && renderCaps->getVidMemSize() > 100*1024*1024)
        ? Vector3int32(8, 4, 8) // 8x4x8 chunks take 16 Mb of VRAM
        : Vector3int32(4, 4, 4);

    LightGrid* lgrid = LightGrid::create(this, gridSize, gridTextureMode);
    RBXASSERT(lgrid);

    lightGrid.reset(lgrid);
    
    // load fonts
    if (FFlag::UseDynamicTypesetterUTF8)
    {
        glyphAtlas.reset(new TextureAtlas(this, 2048, 2048));
		std::vector<std::string> fallbackFontPaths;
		static const char* const fallbackFonts[] = {
			"fonts/NotoNaskhArabicUI-Regular.ttf",
			"fonts/NotoSansBengaliUI-Regular.ttf",
			"fonts/NotoSansDevanagariUI-Regular.ttf",
			"fonts/NotoSansGeorgian-Regular.ttf",
			"fonts/NotoSansKhmerUI-Regular.ttf",
			"fonts/NotoSansMyanmarUI-Regular.ttf",
			"fonts/NotoSansSinhalaUI-Regular.ttf",
			"fonts/NotoSansThaiUI-Regular.ttf",
			"fonts/RobloxEmoji.ttf",
			"fonts/TwemojiMozilla.ttf"
		};
		for (size_t i = 0; i < sizeof(fallbackFonts) / sizeof(fallbackFonts[0]); ++i)
		{
			const std::string path = ContentProvider::findAsset(
				ContentId(std::string("rbxasset://") + fallbackFonts[i]));
			if (!path.empty())
				fallbackFontPaths.push_back(path);
		}
        for (Text::Font font = Text::FONT_LEGACY; font != Text::FONT_LAST; font=Text::Font(font+1))
        {
            if (!Text::isValidFont(font))
                continue;

            float legacyHeightScale = (font == Text::FONT_LEGACY) ? 1.5f : 1.f;
            std::string fontPath = ContentProvider::findAsset(ContentId(std::string("rbxasset://") + dynamicFontPath(font)));
            typesetters[font].reset(new TypesetterDynamic(glyphAtlas.get(), textureManager.get(), fontPath, legacyHeightScale, (unsigned)font, device->getCaps().retina, fallbackFontPaths));
        }
    }
    else
    {
        for (Text::Font font = Text::FONT_LEGACY; font != Text::FONT_LAST; font=Text::Font(font+1))
        {
            if (!Text::isValidFont(font))
                continue;
            const char* bitmapPath = "fonts/SourceSans.font";
            if (font == Text::FONT_LEGACY || font == Text::FONT_ARIAL)
                bitmapPath = "fonts/Arial.font";
            else if (font == Text::FONT_ARIALBOLD)
                bitmapPath = "fonts/ArialBold.font";
            else if (font == Text::FONT_SOURCESANSBOLD)
                bitmapPath = "fonts/SourceSansBold.font";
            else if (font == Text::FONT_SOURCESANSLIGHT)
                bitmapPath = "fonts/SourceSansLight.font";
            else if (font == Text::FONT_SOURCESANSITALIC)
                bitmapPath = "fonts/SourceSansItalic.font";

            static const char* kTexturePath = "rbxasset://fonts/fonts.dds";

            float legacyHeightScale = (font == Text::FONT_LEGACY) ? 1.5f : 1.f;
            typesetters[font].reset(new TypesetterBitmap(getTextureManager(), ContentProvider::assetFolder() + bitmapPath, kTexturePath, legacyHeightScale, device->getCaps().retina));
        }
    }

    materialGenerator.reset(new MaterialGenerator(this));

    sceneManager.reset(new SceneManager(this));
    viewportRenderer.reset(new ViewportRenderer(this));

    frameRateManager.reset(new FrameRateManager());

    vertexStreamer.reset(new VertexStreamer(this));
    
    textureCompositor.reset(new TextureCompositor(this));

    water.reset(new Water(this));

	emitterShared.reset( new EmitterShared );

    // set up caps
	bool gbufferSupported =
        shaderManager->getProgram("PassThroughVS", "SSAOFS").get() != NULL &&
        device->getCaps().maxDrawBuffers >= 2 &&
        (device->getFeatureLevel() == "D3D11" || SystemUtil::getVideoMemory() >= 128*1024*1024);

    renderCaps->setSupportsGBuffer(gbufferSupported);

    if (shared_ptr<ShaderProgram> program = shaderManager->getProgram("DefaultSkinnedVS", "DefaultFS"))
    {
        unsigned int boneCount = program->getMaxWorldTransforms();
        
        FASTLOG1(FLog::Graphics, "Supported bones for skinning: %d", boneCount);

        renderCaps->setSkinningBoneCount(boneCount);
    }
    else
    {
        FASTLOG(FLog::Graphics, "Supported bones for skinning: 0 (no shader support)");
    }

    // configure FRM after caps are valid
    frameRateManager->Configure(renderCaps.get(), settings);
}

	const boost::shared_ptr<Typesetter>& VisualEngine::getTypesetter(Text::Font font)
	{
		return typesetters[font]; 
	}

VisualEngine::~VisualEngine()
{
    bindWorkspace(boost::shared_ptr<DataModel>());
}

void VisualEngine::bindWorkspace(const boost::shared_ptr<DataModel>& dm)
{
    if (dm)
    {
        contentProvider = ServiceProvider::create<ContentProvider>(dm.get());
        meshContentProvider = ServiceProvider::create<MeshContentProvider>(dm.get());
        lighting = ServiceProvider::create<Lighting>(dm.get());
        meshContentProvider->setCacheSize(settings->getMeshCacheSize());
        ServiceProvider::create<SolidModelContentProvider>(dm.get());

		RBXASSERT(!sceneUpdater);
		sceneUpdater.reset(new SceneUpdater(dm, this));

		adorn.reset(new AdornRender(this, dm.get()));

        if (lightGrid)
        {
            // Clear the grid and do an initial upload of all chunks to ensure texture has correct data
            lightGrid->lightingClearAll();
            lightGrid->lightingUploadAll();
            lightGrid->lightingUploadCommit();
        }
    }
    else
    {
        contentProvider = NULL;
		meshContentProvider = NULL;
        lighting = NULL;

		// Viewport SceneUpdaters own GfxParts whose destructors route their
		// notifications through the primary SceneUpdater.  Tear the viewport
		// jobs down while that updater (and its queue mutex) is still alive.
		viewportRenderer->clear();

		if (sceneUpdater)
		{
            sceneUpdater->unbind();
            sceneUpdater.reset();
        }

		adorn.reset();

		if (FFlag::CancelPendingTextureLoads)
		{
			textureCompositor->cancelPendingRequests();
			textureManager->cancelPendingRequests();
		}
    }
}

void VisualEngine::setViewport(int width, int height)
{
    viewWidth = width;
    viewHeight = height;
}

void VisualEngine::setCamera(const Camera& value, const G3D::Vector3& poi)
{
	if (FFlag::CameraVR)
		camera.setViewCFrame(value.getRenderingCoordinateFrame());
	else
		camera.setViewCFrame(value.coordinateFrame(), value.getRoll());

	camera.setProjectionPerspective(value.getFieldOfView(), static_cast<float>(viewWidth) / viewHeight, -value.nearPlaneZ(), -value.farPlaneZ());

    sceneManager->setPointOfInterest(poi);
    
    if (sceneUpdater)
        sceneUpdater->setPointOfInterest(poi);

    // get frustum with the far clip plane to max block distance
    float updateFarPlaneZ;
	if (frameRateManager) {
        updateFarPlaneZ = (float) -std::max(1.0, frameRateManager->GetMaxNextViewCullDistance());
        updateFarPlaneZ = std::max(updateFarPlaneZ, value.farPlaneZ());
    } else {
        updateFarPlaneZ = value.farPlaneZ();
    }

	value.frustum(updateFarPlaneZ, updateFrustum);
    
    // update camera for culling
    cameraCull = camera;
    
    if (DeviceVR* vr = device->getVR())
    {
        DeviceVR::State vrState = vr->getState();

		// Use max FOV for culling; ideally we also should account for IPD
		cameraCull.setProjectionPerspective(
			std::max(vrState.eyeFov[0][0], vrState.eyeFov[1][0]),
			std::max(vrState.eyeFov[0][1], vrState.eyeFov[1][1]),
			std::max(vrState.eyeFov[0][2], vrState.eyeFov[1][2]),
			std::max(vrState.eyeFov[0][3], vrState.eyeFov[1][3]),
			-value.nearPlaneZ(), -value.farPlaneZ());
    }
    
    // update camera for FRM culling
    cameraCullFrm = cameraCull;
    
    cameraCullFrm.changeProjectionPerspectiveZ(-value.nearPlaneZ(), std::min(-value.farPlaneZ(), sqrtf(frameRateManager->GetRenderCullSqDistance())));
}

void VisualEngine::reloadShaders()
{
	shaderManager->loadShaders(ContentProvider::assetFolder() + "../shaders", device->getShadingLanguage(), /* consoleOutput= */ true);
}

void VisualEngine::reloadQueuedAssets()
{
    for (FilenameCountdown::iterator it = assetsToReload.begin(); it != assetsToReload.end(); ++it)
    {
        --it->second;

        if (it->second == 0)
        {
            const std::string& filePath = it->first; 
			immediateAssetReload(filePath);
            assetsToReload.erase(it);
            
            break; // one reload per frame is enough
        }
    }
}

void VisualEngine::queueAssetReload(const std::string& filePath)
{
    if (!filePath.empty())
        assetsToReload[filePath] = 2; // wait this amount of frames before reloading
}

static void reloadMaterialTable(MegaClusterInstance* mci)
{
	mci->reloadMaterialTable();

	if (SmoothClusterBase* sc = dynamic_cast<SmoothClusterBase*>(mci->getGfxPart()))
		sc->reloadMaterialTable();
}

void VisualEngine::immediateAssetReload(const std::string& filePath)
{
	if (filePath.find("rbxasset://") == 0 || filePath.find("rbxgameasset://") == 0 || filePath.find("rbxapp://") == 0)
	{
		if (filePath == "rbxasset://terrain/materials.json")
		{
			if (MegaClusterInstance* mci = Instance::fastDynamicCast<MegaClusterInstance>(sceneUpdater->getDataModel()->getWorkspace()->getTerrain()))
				DataModel::get(mci)->submitTask(boost::bind(reloadMaterialTable, mci), DataModelJob::Write);
		}

		getTextureManager()->reloadImage(ContentId(filePath));
	}
    else
    {
        std::string extension = filePath.substr(std::min(filePath.find_last_of(".") + 1, filePath.size()));
        if (extension == "hlsl" || extension == "h")
        {
            StandardOut::singleton()->printf(MESSAGE_INFO, "Reloading shaders");
            reloadShaders();
        }
    }
}

}
}
