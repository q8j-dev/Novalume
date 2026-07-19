#include "FastCluster.h"

#include "humanoid/Humanoid.h"

#include "GfxBase/AsyncResult.h"
#include "GfxBase/FileMeshData.h"
#include "GfxBase/PartIdentifier.h"
#include "v8datamodel/Bone.h"
#include "v8datamodel/Decal.h"
#include "v8datamodel/Accoutrement.h"
#include "v8datamodel/MeshPart.h"
#include "v8datamodel/PartCookie.h"

#include "v8world/Clump.h"
#include "Util/Math.h"

#include "SceneManager.h"
#include "GeometryGenerator.h"
#include "MaterialGenerator.h"
#include "Util.h"
#include "SceneUpdater.h"
#include "Material.h"

#include "GfxBase/RenderCaps.h"
#include "GfxBase/FrameRateManager.h"

#include "SuperCluster.h"

#include "VisualEngine.h"

#include "rbx/Profiler.h"

#include <array>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

LOGVARIABLE(RenderFastCluster, 0)

namespace RBX
{
namespace Graphics
{

// This should be adjusted so that this number + worst-case number of vertices for any single part <= 65536 (16-bit index buffer limit)
const unsigned int kFastClusterBatchMaxVertices = 32768;

// This should be equal to the maximum number of vertices we can draw in a single call
const unsigned int kFastClusterBatchGroupMaxVertices = 65535;

class FastClusterMeshGenerator
{
public:
    FastClusterMeshGenerator(VisualEngine* visualEngine, RBX::Humanoid* humanoid, unsigned int maxBones, bool isFW)
        : visualEngine(visualEngine)
        , humanoidIdentifier(humanoid)
        , maxBonesPerBatch(0)
		, cpuSkinning(!isFW && visualEngine->getRenderCaps()->getSkinningBoneCount() == 0)
		, cpuIdentityBone(std::numeric_limits<unsigned int>::max())
    {
        bones.reserve(maxBones);
        
        if (isFW || visualEngine->getRenderCaps()->getSkinningBoneCount() == 0)
        {
            maxBonesPerBatch = 1;
        }
        else
        {
            maxBonesPerBatch = visualEngine->getRenderCaps()->getSkinningBoneCount();
        }

		colorOrderBGR = visualEngine->getDevice()->getCaps().colorOrderBGR;
    }

	unsigned int ensureCpuIdentityBone()
	{
		if (cpuIdentityBone == std::numeric_limits<unsigned int>::max())
		{
			cpuIdentityBone = static_cast<unsigned int>(bones.size());
			addBone(NULL);
		}
		return cpuIdentityBone;
	}

	~FastClusterMeshGenerator()
	{
		visualEngine->getMaterialGenerator()->invalidateCompositCache();
	}
    
    const HumanoidIdentifier& getHumanoidIdentifier()
    {
        return humanoidIdentifier;
    }
    
    void addBone(RBX::PartInstance* root)
    {
        Bone bone;
        bone.root = root;
		bone.parent = std::numeric_limits<unsigned int>::max();
		bone.skinned = false;
        bone.boundsMin = Vector3::maxFinite();
        bone.boundsMax = Vector3::minFinite();
        
        bones.push_back(bone);
    }

	std::vector<unsigned int>& getFileSkeleton(
		RBX::PartInstance* part, const boost::shared_ptr<FileMeshData>& mesh)
	{
		const FileSkeletonKey key(part, mesh.get());
		FileSkeletonMap::iterator existing = fileSkeletons.find(key);
		if (existing != fileSkeletons.end())
			return existing->second;

		std::vector<unsigned int> indices;
		indices.reserve(mesh->bones.size());
		Instance* rigRoot = humanoidIdentifier.humanoid
			? humanoidIdentifier.humanoid->getParent() : part->getParent();
		const Vector3 scale = getFileMeshScale(part);

		for (std::size_t index = 0; index < mesh->bones.size(); ++index)
		{
			const FileMeshData::Bone& source = mesh->bones[index];
			if (source.parentIndex != 0xffff && source.parentIndex >= index)
				throw std::runtime_error(
					"FileMesh skeleton is not in parent-before-child order");
			Instance* target = findRigTarget(rigRoot, source.name);
			if (!target && source.name == "Root")
				target = findRigTarget(rigRoot, "HumanoidRootPart");

			Bone bone;
			bone.root = part;
			bone.skinned = true;
			bone.parent = source.parentIndex == 0xffff
				? std::numeric_limits<unsigned int>::max()
				: indices[source.parentIndex];
			bone.bindFrame = source.bindFrame;
			bone.bindFrame.translation = Vector3(
				source.bindFrame.translation.x * scale.x,
				source.bindFrame.translation.y * scale.y,
				source.bindFrame.translation.z * scale.z);
			if (PartInstance* targetPart = Instance::fastDynamicCast<PartInstance>(target))
				bone.targetPart = weak_from(targetPart);
			else if (RBX::Bone* targetBone = Instance::fastDynamicCast<RBX::Bone>(target))
				bone.targetBone = weak_from(targetBone);
			bone.boundsMin = Vector3::maxFinite();
			bone.boundsMax = Vector3::minFinite();
			indices.push_back(static_cast<unsigned int>(bones.size()));
			bones.push_back(bone);
		}

		return fileSkeletons.insert(std::make_pair(key, std::move(indices))).first->second;
	}
    
    void addInstance(size_t boneIndex, RBX::PartInstance* part, RBX::Decal* decal, unsigned int materialFlags, RenderQueue::Id renderQueue, RBX::AsyncResult* asyncResult)
    {
        RBXASSERT(boneIndex < bones.size());
        
        const HumanoidIdentifier* hi = humanoidIdentifier.humanoid ? &humanoidIdentifier : NULL;
        
		MaterialGenerator::Result material = visualEngine->getMaterialGenerator()->createMaterial(part, decal, hi, materialFlags);
        
        if (!material.material)
            return;
        
		RenderQueue::Id effectiveRenderQueue = renderQueue;
		if (renderQueue == RenderQueue::Id_Opaque && part->getCastShadow())
			effectiveRenderQueue = RenderQueue::Id_OpaqueCasters;
		MaterialGroup& mg = materialGroups[std::make_pair(material.material.get(), effectiveRenderQueue)];
        
        if (!mg.material)
        {
            mg.material = material.material;
			mg.renderQueue = effectiveRenderQueue;
            mg.materialResultFlags = material.flags;
            mg.materialFeatures = material.features;

            if (decal)
			{
                unsigned int opaqueMaterialFlags = materialFlags & ~MaterialGenerator::Flag_Transparent;

				mg.decalMaterialOpaque = visualEngine->getMaterialGenerator()->createMaterial(part, decal, hi, opaqueMaterialFlags).material;
            }
        }
        else
        {
            RBXASSERT(mg.materialResultFlags == material.flags);
        }
        
        // fetch any resources the part might need
        unsigned int resourceFlags = ((materialFlags & MaterialGenerator::Flag_UseCompositTexture) || (hi && hi->isPartHead(part)))
                ? GeometryGenerator::Resource_SubstituteBodyParts
                : 0;

        GeometryGenerator::Resources resources = GeometryGenerator::fetchResources(part, hi, resourceFlags, asyncResult);

		auto appendInstance = [&](const std::vector<unsigned int>& requiredBones,
			int fileMeshSubset, const std::vector<unsigned int>* fileSkeleton,
			const FileMeshData::Subset* subset) {
			if (requiredBones.empty() || requiredBones.size() > maxBonesPerBatch)
				throw std::runtime_error("FileMesh subset exceeds the shader bone palette");

			const bool forceBatchBreak = effectiveRenderQueue == RenderQueue::Id_Transparent &&
				part != getLastPart(mg);
			bool createBatch = mg.batches.empty() || forceBatchBreak;
			if (!createBatch)
			{
				const Batch& current = mg.batches.back();
				createBatch = current.counter.getVertexCount() >=
					kFastClusterBatchMaxVertices;
				std::size_t unionCount = current.bones.size();
				for (unsigned int required : requiredBones)
					if (std::find(current.bones.begin(), current.bones.end(), required) ==
						current.bones.end())
						++unionCount;
				createBatch = createBatch || unionCount > maxBonesPerBatch;
			}
			if (createBatch)
				mg.batches.push_back(Batch());

			Batch& batch = mg.batches.back();
			for (unsigned int required : requiredBones)
				if (std::find(batch.bones.begin(), batch.bones.end(), required) ==
					batch.bones.end())
					batch.bones.push_back(required);
			RBXASSERT(batch.bones.size() <= maxBonesPerBatch);

			GeometryGenerator::Options options;
			options.fileMeshSubset = fileMeshSubset;
			options.fileSkinning = fileSkeleton != NULL;
			options.fileCpuSkinning = fileSkeleton != NULL && cpuSkinning;
			if (fileSkeleton && subset)
				for (std::size_t paletteIndex = 0;
					paletteIndex < subset->boneIndices.size(); ++paletteIndex)
				{
					const std::uint16_t fileBone = subset->boneIndices[paletteIndex];
					if (fileBone >= fileSkeleton->size())
						throw std::runtime_error("FileMesh subset has an invalid skeleton index");
					const unsigned int clusterBone = (*fileSkeleton)[fileBone];
					if (cpuSkinning)
					{
						options.fileBoneMap[fileBone] = static_cast<std::uint8_t>(paletteIndex);
						continue;
					}
					const std::vector<unsigned int>::const_iterator local =
						std::find(batch.bones.begin(), batch.bones.end(), clusterBone);
					if (local == batch.bones.end())
						throw std::runtime_error("FileMesh subset palette was not installed");
					options.fileBoneMap[fileBone] = static_cast<std::uint8_t>(
						std::distance(batch.bones.cbegin(), local));
				}

			const std::vector<unsigned int>::const_iterator rigidLocal =
				std::find(batch.bones.begin(), batch.bones.end(), requiredBones.front());
			const unsigned int localBoneIndex = static_cast<unsigned int>(
				std::distance(batch.bones.cbegin(), rigidLocal));
			batch.counter.addInstance(part, decal, options, resources);

			BatchInstance instance;
			instance.part = part;
			instance.decal = decal;
			instance.localBoneIndex = localBoneIndex;
			instance.fileMeshSubset = fileMeshSubset;
			instance.fileSkinning = fileSkeleton != NULL;
			instance.fileCpuSkinning = fileSkeleton != NULL && cpuSkinning;
			instance.fileBoneMap = options.fileBoneMap;
			instance.fileClusterBoneMap.fill(std::numeric_limits<unsigned int>::max());
			if (fileSkeleton && subset)
				for (std::size_t paletteIndex = 0;
					paletteIndex < subset->boneIndices.size(); ++paletteIndex)
					instance.fileClusterBoneMap[paletteIndex] =
						(*fileSkeleton)[subset->boneIndices[paletteIndex]];
			instance.uvOffsetScale = material.uvOffsetScale;
			instance.resources = resources;
			batch.instances.push_back(std::move(instance));
		};

		if (resources.fileMeshData && !resources.fileMeshData->skinning.empty())
		{
			if (resources.fileMeshData->bones.empty() ||
				resources.fileMeshData->subsets.empty())
				throw std::runtime_error("skinned FileMesh has no bone subsets");
			std::vector<unsigned int>& skeleton =
				getFileSkeleton(part, resources.fileMeshData);
			for (std::size_t subsetIndex = 0;
				subsetIndex < resources.fileMeshData->subsets.size(); ++subsetIndex)
			{
				const FileMeshData::Subset& subset =
					resources.fileMeshData->subsets[subsetIndex];
				std::vector<unsigned int> required;
				required.reserve(subset.boneIndices.size());
				for (std::uint16_t fileBone : subset.boneIndices)
				{
					if (fileBone >= skeleton.size())
						throw std::runtime_error("FileMesh subset bone is out of range");
					required.push_back(skeleton[fileBone]);
				}
				if (cpuSkinning)
					required.assign(1, ensureCpuIdentityBone());
				appendInstance(required, static_cast<int>(subsetIndex), &skeleton, &subset);
			}
		}
		else
		{
			const std::vector<unsigned int> rigidBones(1, static_cast<unsigned int>(boneIndex));
			appendInstance(rigidBones, -1, NULL, NULL);
		}
    }
            
    unsigned int finalize(FastCluster* cluster, FastClusterSharedGeometry& sharedGeometry)
    {
        GeometryGenerator::Vertex* sharedVertexData = NULL;			
        unsigned short* sharedIndexData = NULL;
        unsigned int sharedVertexOffset = 0;
        unsigned int sharedIndexOffset = 0;

        // Gather pointers to all batches
        std::vector<std::pair<MaterialGroup*, Batch*> > batches;

        for (MaterialGroupMap::iterator it = materialGroups.begin(); it != materialGroups.end(); ++it)
        {
            MaterialGroup& mg = it->second;

            for (MaterialGroup::BatchList::iterator bit = mg.batches.begin(); bit != mg.batches.end(); ++bit)
            {
                Batch& batch = *bit;
                
                if (batch.instances.empty() || batch.counter.getVertexCount() == 0 || batch.counter.getIndexCount() == 0)
                    continue;

                batches.push_back(std::make_pair(&mg, &batch));
            }
        }

        // Sort batches to make sure batches that can be merged go first
        std::sort(batches.begin(), batches.end(), BatchMaterialPlasticLODComparator());

        // Get total size of the shared geometry data
        unsigned int sharedVertexCount = 0;
        unsigned int sharedIndexCount = 0;

        for (size_t i = 0; i < batches.size(); ++i)
        {
            Batch& batch = *batches[i].second;

            sharedVertexCount += batch.counter.getVertexCount();
            sharedIndexCount += batch.counter.getIndexCount();
        }

        // Update shared geometry so that it has the required amount of memory
        if (sharedVertexCount > 0 && sharedIndexCount > 0)
        {
            setupSharedGeometry(sharedGeometry, sharedVertexCount, sharedIndexCount, cluster->isFW());
            
            sharedVertexData = static_cast<GeometryGenerator::Vertex*>(sharedGeometry.vertexBuffer->lock(GeometryBuffer::Lock_Discard));
            sharedIndexData = static_cast<unsigned short*>(sharedGeometry.indexBuffer->lock(GeometryBuffer::Lock_Discard));
        }
        else
        {
            sharedGeometry.reset();
        }

        // Create as few geometry objects as we can
        shared_ptr<Geometry> geometry;
        unsigned int geometryBaseVertex = 0;
        
        // Create entities in groups
        for (size_t groupBegin = 0; groupBegin < batches.size(); )
        {
            unsigned int groupEnd = groupBegin + 1;

            unsigned int groupVertexCount = batches[groupBegin].second->counter.getVertexCount();

            unsigned int groupVertexOffset = sharedVertexOffset;
            unsigned int groupIndexOffset = sharedIndexOffset;
            Extents groupBounds;

            // Reuse geometry if generated indices stay within 16-bit boundaries
			if (!geometry || groupVertexOffset + groupVertexCount - geometryBaseVertex > kFastClusterBatchGroupMaxVertices)
			{
                geometry = visualEngine->getDevice()->createGeometry(getVertexLayout(), sharedGeometry.vertexBuffer, sharedGeometry.indexBuffer, groupVertexOffset);
                geometryBaseVertex = groupVertexOffset;
			}

            // Generate geometry for high LOD
            for (size_t bi = groupBegin; bi < groupEnd; ++bi)
            {
                const MaterialGroup& mg = *batches[bi].first;
                const Batch& batch = *batches[bi].second;
                
				unsigned int vertexOffset = sharedVertexOffset - geometryBaseVertex;

                // generate geometry
                std::vector<unsigned int> instanceVertexCount;
                
                Extents bounds = generateBatchGeometry(mg, batch, sharedVertexData + geometryBaseVertex, sharedIndexData + sharedIndexOffset, vertexOffset, instanceVertexCount, cluster->isFW());
                
                // create geometry batch
                GeometryBatch geometryBatch(geometry, Geometry::Primitive_Triangles, sharedIndexOffset, batch.counter.getIndexCount(), vertexOffset, vertexOffset + batch.counter.getVertexCount());

                // setup entity
                unsigned char lodMask = (groupEnd - groupBegin > 1) ? ((1 << 0) | (1 << 1)) : 0xff;

                RenderQueue::Id queueId = mg.renderQueue;
                if (mg.renderQueue == RenderQueue::Id_Transparent && cluster->getHumanoidKey())
                    queueId = RenderQueue::Id_TransparentCasters;

                cluster->addEntity(new FastClusterEntity(cluster, geometryBatch, mg.material, mg.decalMaterialOpaque, queueId, lodMask, batch.bones, bounds, mg.materialFeatures));
                
                // update buffer offsets
                sharedVertexOffset += batch.counter.getVertexCount();
                sharedIndexOffset += batch.counter.getIndexCount();
                
				groupBounds.expandToContain(bounds);
            }

            // Create merged entity for low LOD
            if (groupEnd - groupBegin > 1)
            {
                const MaterialGroup& mg = *batches[groupBegin].first;
                const Batch& batch = *batches[groupBegin].second;

                // create geometry batch
				GeometryBatch geometryBatch(geometry, Geometry::Primitive_Triangles, groupIndexOffset, sharedIndexOffset - groupIndexOffset, groupVertexOffset - geometryBaseVertex, sharedVertexOffset - geometryBaseVertex);

                // setup entity
				cluster->addEntity(new FastClusterEntity(cluster, geometryBatch, mg.material, mg.decalMaterialOpaque, mg.renderQueue, /* lodMask= */ 1 << 2, batch.bones, groupBounds, mg.materialFeatures));
            }

            // Move to next batch portion
            groupBegin = groupEnd;
        }
        
        if (sharedVertexData && sharedIndexData)
        {
            RBXASSERT(sharedVertexOffset == sharedVertexCount && sharedIndexOffset == sharedIndexCount);

			RBXPROFILER_SCOPE("Render", "upload");
            
            sharedGeometry.vertexBuffer->unlock();
            sharedGeometry.indexBuffer->unlock();
        }

        return sharedVertexCount;
    }

    unsigned int getBoneCount()
    {
        return bones.size();
    }
    
    RBX::PartInstance* getBoneRoot(unsigned int i)
    {
        return bones[i].root;
    }
    
    Extents getBoneBounds(unsigned int i)
    {
        const Bone& bone = bones[i];
        
        return Extents(bone.boundsMin, bone.boundsMax);
    }

	bool isSkinnedBone(unsigned int i) const { return bones[i].skinned; }
	unsigned int getBoneParent(unsigned int i) const { return bones[i].parent; }
	const CoordinateFrame& getBoneBindFrame(unsigned int i) const
	{
		return bones[i].bindFrame;
	}
	const boost::weak_ptr<PartInstance>& getBoneTargetPart(unsigned int i) const
	{
		return bones[i].targetPart;
	}
	const boost::weak_ptr<RBX::Bone>& getBoneTargetBone(unsigned int i) const
	{
		return bones[i].targetBone;
	}

	CoordinateFrame getSkinTransform(unsigned int i) const
	{
		if (i >= bones.size() || !bones[i].skinned)
			return CoordinateFrame();
		std::vector<CoordinateFrame> cache(bones.size());
		std::vector<unsigned char> state(bones.size(), 0);
		const RBX::Time now = RBX::Time::nowFast();
		const CoordinateFrame current = getSkinWorld(i, now, cache, state);
		return current * bones[i].bindFrame.inverse();
	}

private:
	CoordinateFrame getSkinWorld(unsigned int index, const RBX::Time& now,
		std::vector<CoordinateFrame>& cache,
		std::vector<unsigned char>& state) const
	{
		if (state[index] == 2)
			return cache[index];
		if (state[index] == 1)
			throw std::runtime_error("FileMesh skeleton contains a cycle");
		state[index] = 1;
		const Bone& bone = bones[index];
		CoordinateFrame current;
		if (boost::shared_ptr<PartInstance> target = bone.targetPart.lock())
			current = target->calcRenderingCoordinateFrame(now);
		else if (boost::shared_ptr<RBX::Bone> target = bone.targetBone.lock())
			current = target->getTransformedWorldCFrame();
		else if (bone.parent < bones.size())
			current = getSkinWorld(bone.parent, now, cache, state) *
				bones[bone.parent].bindFrame.toObjectSpace(bone.bindFrame);
		else if (bone.root)
			current = bone.root->calcRenderingCoordinateFrame(now) * bone.bindFrame;
		else
			current = bone.bindFrame;
		cache[index] = current;
		state[index] = 2;
		return current;
	}

    struct BatchInstance
    {
        RBX::PartInstance* part;
        RBX::Decal* decal;
        unsigned int localBoneIndex;
		int fileMeshSubset;
		bool fileSkinning;
		bool fileCpuSkinning;
		std::array<std::uint8_t, 256> fileBoneMap;
		std::array<unsigned int, 256> fileClusterBoneMap;
        G3D::Vector4 uvOffsetScale;
        GeometryGenerator::Resources resources;
    };
    
    struct Batch
    {
        GeometryGenerator counter;
        std::vector<BatchInstance> instances;
        std::vector<unsigned int> bones;
    };
    
    struct MaterialGroup
    {
        shared_ptr<Material> material;
        RenderQueue::Id renderQueue;

        shared_ptr<Material> decalMaterialOpaque;
        unsigned int materialResultFlags;
        unsigned int materialFeatures;
        
        typedef std::list<Batch> BatchList;
        BatchList batches;
    };
    
    struct Bone
    {
        RBX::PartInstance* root;
		boost::weak_ptr<RBX::PartInstance> targetPart;
		boost::weak_ptr<RBX::Bone> targetBone;
		unsigned int parent;
		CoordinateFrame bindFrame;
		bool skinned;
        RBX::Vector3 boundsMin;
        RBX::Vector3 boundsMax;
    };

	typedef std::pair<RBX::PartInstance*, const FileMeshData*> FileSkeletonKey;
	typedef std::map<FileSkeletonKey, std::vector<unsigned int> > FileSkeletonMap;
    
    typedef std::map<std::pair<Material*, RenderQueue::Id>, MaterialGroup> MaterialGroupMap;
    
    VisualEngine* visualEngine;
    HumanoidIdentifier humanoidIdentifier;

    unsigned int maxBonesPerBatch;
    bool colorOrderBGR;
	bool cpuSkinning;
	unsigned int cpuIdentityBone;
    
    MaterialGroupMap materialGroups;
    std::vector<Bone> bones;
	FileSkeletonMap fileSkeletons;

	static Instance* findRigTarget(Instance* root, const std::string& name)
	{
		if (!root)
			return NULL;
		if (root->getName() == name &&
			(Instance::fastDynamicCast<PartInstance>(root) ||
			 Instance::fastDynamicCast<RBX::Bone>(root)))
			return root;
		for (std::size_t index = 0; index < root->numChildren(); ++index)
			if (Instance* result = findRigTarget(root->getChild(index), name))
				return result;
		return NULL;
	}

	static Vector3 getFileMeshScale(PartInstance* part)
	{
		if (MeshPart* meshPart = Instance::fastDynamicCast<MeshPart>(part))
		{
			const Vector3 initial = meshPart->getInitialSize();
			const Vector3 current = meshPart->getPartSizeXml();
			if (initial.min() > 0.0f)
				return Vector3(current.x / initial.x, current.y / initial.y,
					current.z / initial.z);
		}
		return Vector3::one();
	}

    RBX::PartInstance* getLastPart(const MaterialGroup& mg)
    {
        if (mg.batches.empty())
            return NULL;
            
        const Batch& batch = mg.batches.back();
        
        if (batch.instances.empty())
            return NULL;
            
        return batch.instances.back().part;
    }
    
    const shared_ptr<VertexLayout>& getVertexLayout()
    {
        shared_ptr<VertexLayout>& p = visualEngine->getFastClusterLayout();
        
        if (!p)
        {
            std::vector<VertexLayout::Element> elements;

            elements.push_back(VertexLayout::Element(0, offsetof(GeometryGenerator::Vertex, pos), VertexLayout::Format_Float3, VertexLayout::Semantic_Position));
            elements.push_back(VertexLayout::Element(0, offsetof(GeometryGenerator::Vertex, normal), VertexLayout::Format_Float3, VertexLayout::Semantic_Normal));
            
            elements.push_back(VertexLayout::Element(0, offsetof(GeometryGenerator::Vertex, color), VertexLayout::Format_Color, VertexLayout::Semantic_Color, 0));

            bool useShaders = visualEngine->getRenderCaps()->getSkinningBoneCount() > 0;
                
            if (useShaders)
			{
                elements.push_back(VertexLayout::Element(0, offsetof(GeometryGenerator::Vertex, extra), VertexLayout::Format_UByte4, VertexLayout::Semantic_Color, 1));
			}
			else
			{
                elements.push_back(VertexLayout::Element(0, offsetof(GeometryGenerator::Vertex, extra), VertexLayout::Format_Color, VertexLayout::Semantic_Color, 1));
            }
            
            elements.push_back(VertexLayout::Element(0, offsetof(GeometryGenerator::Vertex, uv), VertexLayout::Format_Float2, VertexLayout::Semantic_Texture, 0));
            elements.push_back(VertexLayout::Element(0, offsetof(GeometryGenerator::Vertex, uvStuds), VertexLayout::Format_Float2, VertexLayout::Semantic_Texture, 1));
            
            elements.push_back(VertexLayout::Element(0, offsetof(GeometryGenerator::Vertex, tangent), VertexLayout::Format_Float3, VertexLayout::Semantic_Texture, 2));
            elements.push_back(VertexLayout::Element(0, offsetof(GeometryGenerator::Vertex, edgeDistances), VertexLayout::Format_Float4, VertexLayout::Semantic_Texture, 3));
			elements.push_back(VertexLayout::Element(0, offsetof(GeometryGenerator::Vertex, skinIndices), VertexLayout::Format_UByte4, VertexLayout::Semantic_Texture, 4));
			elements.push_back(VertexLayout::Element(0, offsetof(GeometryGenerator::Vertex, skinWeights), VertexLayout::Format_Color, VertexLayout::Semantic_Texture, 5));

            p = visualEngine->getDevice()->createVertexLayout(elements);
            RBXASSERT(p);
        }
        
        return p;
    }
    
    bool canUseBuffer(unsigned int currentCount, unsigned int desiredCount)
    {
        // We can use the current buffer if it has enough data and it does not waste >10% of memory
        return
            desiredCount <= currentCount &&
            currentCount - desiredCount < currentCount / 10;
    }

    void setupSharedGeometry(FastClusterSharedGeometry& sharedGeometry, unsigned int vertexCount, unsigned int indexCount, bool isFW)
    {
        // Create vertex buffer
        if (!sharedGeometry.vertexBuffer || !canUseBuffer(sharedGeometry.vertexBuffer->getElementCount(), vertexCount))
        {
            sharedGeometry.vertexBuffer = visualEngine->getDevice()->createVertexBuffer(sizeof(GeometryGenerator::Vertex), vertexCount, GeometryBuffer::Usage_Static);
        }
        
        // Create index buffer
        if (!sharedGeometry.indexBuffer || !canUseBuffer(sharedGeometry.indexBuffer->getElementCount(), indexCount))
        {
            sharedGeometry.indexBuffer = visualEngine->getDevice()->createIndexBuffer(sizeof(unsigned short), indexCount, GeometryBuffer::Usage_Static);
        }
    }

    CoordinateFrame getRelativeTransform(PartInstance* part, PartInstance* root)
    {
        if (part == root)
        {
            static const CoordinateFrame identity;
            
            return identity;
        }
        else if (root)
        {
            const CoordinateFrame& partFrame = part->getCoordinateFrame();
            const CoordinateFrame& rootFrame = root->getCoordinateFrame();
            
            return rootFrame.toObjectSpace(partFrame);
        }
        else
        {
            return part->getCoordinateFrame();
        }
    }
    
    Extents generateBatchGeometry(const MaterialGroup& mg, const Batch& batch, GeometryGenerator::Vertex* vbptr, unsigned short* ibptr, unsigned int vertexOffset, std::vector<unsigned int>& instanceVertexCount, bool isFW)
    {
		RBXPROFILER_SCOPE("Render", "generateBatchGeometry");

        instanceVertexCount.reserve(batch.instances.size());
        
        GeometryGenerator generator(vbptr, ibptr, vertexOffset);
        
        Vector3 boundsMin = Vector3::maxFinite();
        Vector3 boundsMax = Vector3::minFinite();
        
        const HumanoidIdentifier* hi = humanoidIdentifier.humanoid ? &humanoidIdentifier : NULL;

        for (size_t i = 0; i < batch.instances.size(); ++i)
        {
            const BatchInstance& bi = batch.instances[i];
            Bone& bone = bones[batch.bones[bi.localBoneIndex]];

			const CoordinateFrame localTransform = bi.fileSkinning
				? CoordinateFrame()
				: getRelativeTransform(bi.part, bone.root);
            GeometryGenerator::Options options(visualEngine, *bi.part, bi.decal,
				localTransform, mg.materialResultFlags, bi.uvOffsetScale,
				bi.localBoneIndex);
			options.fileMeshSubset = bi.fileMeshSubset;
			options.fileSkinning = bi.fileSkinning;
			options.fileCpuSkinning = bi.fileCpuSkinning;
			options.fileBoneMap = bi.fileBoneMap;
			if (bi.fileCpuSkinning && bi.resources.fileMeshData &&
				bi.fileMeshSubset >= 0)
			{
				const FileMeshData::Subset& subset = bi.resources.fileMeshData->subsets[
					static_cast<std::size_t>(bi.fileMeshSubset)];
				for (std::size_t paletteIndex = 0;
					paletteIndex < subset.boneIndices.size(); ++paletteIndex)
				{
					const unsigned int clusterBone =
						bi.fileClusterBoneMap[paletteIndex];
					if (clusterBone == std::numeric_limits<unsigned int>::max())
						throw std::runtime_error("CPU FileMesh palette is incomplete");
					options.fileSkinTransforms[paletteIndex] =
						getSkinTransform(clusterBone);
				}
			}

            generator.resetBounds();
            
            size_t oldVertexCount = generator.getVertexCount();
            
            generator.addInstance(bi.part, bi.decal, options, bi.resources, hi);
            
            instanceVertexCount.push_back(generator.getVertexCount() - oldVertexCount);

            if (generator.areBoundsValid())
            {
                RBX::AABox partBounds = generator.getBounds();

				if (bi.fileSkinning && !bi.fileCpuSkinning &&
					bi.resources.fileMeshData &&
					bi.fileMeshSubset >= 0)
				{
					const FileMeshData::Subset& subset = bi.resources.fileMeshData->subsets[
						static_cast<std::size_t>(bi.fileMeshSubset)];
					std::set<unsigned int> localBones;
					for (std::uint16_t fileBone : subset.boneIndices)
						localBones.insert(bi.fileBoneMap[fileBone]);
					for (unsigned int localBone : localBones)
					{
						Bone& influenced = bones[batch.bones[localBone]];
						influenced.boundsMin = influenced.boundsMin.min(partBounds.low());
						influenced.boundsMax = influenced.boundsMax.max(partBounds.high());
					}
				}
				else
				{
					bone.boundsMin = bone.boundsMin.min(partBounds.low());
					bone.boundsMax = bone.boundsMax.max(partBounds.high());
				}
                boundsMin = boundsMin.min(partBounds.low());
                boundsMax = boundsMax.max(partBounds.high());
            }
        }

        // Patch stud UVs for FFP
		if (visualEngine->getDevice()->getCaps().supportsFFP && (mg.materialResultFlags & MaterialGenerator::Result_UsesTexture) == 0)
			for (unsigned int i = vertexOffset; i < generator.getVertexCount(); ++i)
				vbptr[i].uv = vbptr[i].uvStuds;
        
        RBXASSERT(generator.getVertexCount() == batch.counter.getVertexCount() + vertexOffset && generator.getIndexCount() == batch.counter.getIndexCount());
        
        return getBounds(boundsMin, boundsMax);
    }
    
    Extents getBounds(const Vector3& min, const Vector3& max)
    {
        if (min.x <= max.x)
            return Extents(min, max);
        else
            return Extents();
    }

    struct BatchMaterialPlasticLODComparator
    {
        bool operator()(const std::pair<MaterialGroup*, Batch*>& lhs, const std::pair<MaterialGroup*, Batch*>& rhs) const
        {
            MaterialGroup* ml = lhs.first;
            MaterialGroup* mr = rhs.first;

            return (ml->materialResultFlags & MaterialGenerator::Result_PlasticLOD) > (mr->materialResultFlags & MaterialGenerator::Result_PlasticLOD);

        }
    };
};

struct PartBindingNullPredicate
{
    template <typename T> bool operator()(const T& part) const
    {
        return part.binding == NULL;
    }
};

struct PartClumpSinglePredicate
{
    template <typename T> bool operator()(const T& part) const
    {
        const Primitive* p = part.instance->getConstPartPrimitive();
        const Primitive* clumpRoot = p->getRoot<Primitive>();
        
        return clumpRoot->numChildren() == 0;
    }
};

struct PartClumpGroupPredicate
{
    template <typename T> bool operator()(const T& lhs, const T& rhs) const
    {
        return lhs.instance->getClump() < rhs.instance->getClump();
    }
};

FastClusterEntity::FastClusterEntity(FastCluster* cluster, const GeometryBatch& geometry, const shared_ptr<Material>& material, const shared_ptr<Material>& decalMaterialOpaque,
     RenderQueue::Id renderQueueId, unsigned char lodMask, const std::vector<unsigned int>& bones, const Extents& localBounds, unsigned int extraFeatures)
    : RenderEntity(cluster, geometry, material, renderQueueId, lodMask)
    , decalMaterialOpaque(decalMaterialOpaque)
    , extraFeatures(extraFeatures)
    , bones(bones)
	, localBounds(localBounds)
    , sortKeyOffset(0)
{
	if (decalMaterialOpaque)
    {
		if (const Technique* technique = decalMaterialOpaque->getBestTechnique(0, RenderQueue::Pass_Default))
            decalTexture = technique->getTexture(MaterialGenerator::getDiffuseMapStage());

        // if decal we want to offset slightly sort key
        sortKeyOffset = -0.01f;
    }
}

FastClusterEntity::~FastClusterEntity()
{
}

unsigned int FastClusterEntity::getWorldTransforms4x3(float* buffer, unsigned int maxTransforms, const void** cacheKey) const
{
    if (useCache(cacheKey, this)) return 0;

    RBXASSERT(bones.size() <= maxTransforms);

    FastCluster* cluster = static_cast<FastCluster*>(node);
    
    for (unsigned int i = 0; i < bones.size(); ++i)
    {
        const CoordinateFrame& cframe = cluster->getTransform(bones[i]);

        memcpy(&buffer[0], cframe.rotation[0], sizeof(float) * 3);
        buffer[3] = cframe.translation.x;

        memcpy(&buffer[4], cframe.rotation[1], sizeof(float) * 3);
        buffer[7] = cframe.translation.y;

        memcpy(&buffer[8], cframe.rotation[2], sizeof(float) * 3);
        buffer[11] = cframe.translation.z;

        buffer += 12;
    }

    return bones.size();
}

void FastClusterEntity::updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, unsigned int lodIndex, RenderQueue::Pass pass)
{
	TextureRef::Status decalStatus = decalTexture.getStatus();
	
	if (decalStatus != TextureRef::Status_Null)
	{
		if (decalStatus != TextureRef::Status_Loaded)
            return;

		if (renderQueueId == RenderQueue::Id_Decals)
		{
			if (!decalTexture.getInfo().alpha)
            {
                material = decalMaterialOpaque;
                renderQueueId = RenderQueue::Id_OpaqueDecals;
            }
            else
            {
                renderQueueId = RenderQueue::Id_TransparentDecals;
            }
		}

        decalTexture = TextureRef();
	}

	queue.setFeature(extraFeatures);

    RenderEntity::updateRenderQueue(queue, camera, lodIndex, pass);
}

float FastClusterEntity::getViewDepth(const RenderCamera& camera) const
{
	FastCluster* cluster = static_cast<FastCluster*>(node);
	Extents worldBounds;
	for (unsigned int bone : bones)
		worldBounds.expandToContain(localBounds.toWorldSpace(cluster->getTransform(bone)));
	const Vector3 center = worldBounds.center();
    
    return RenderEntity::computeViewDepth(camera, center, sortKeyOffset);
}

FastClusterBinding::FastClusterBinding(FastCluster* cluster, const boost::shared_ptr<PartInstance>& part,
	const boost::shared_ptr<Instance>& renderRoot)
    : GfxBinding(part, renderRoot)
    , cluster(cluster)
{
    bindProperties(part);
    
    RBXASSERT(part->getGfxPart() == NULL);
    part->setGfxPart(cluster);
    
    FASTLOG4(FLog::RenderFastCluster, "FastCluster[%p]: bound part %p to binding %p (%d connections)", cluster, part.get(), this, connections.size());

}
    
void FastClusterBinding::invalidateEntity()
{
    FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: part %p with binding %p requests invalidateEntity", cluster, partInstance.get(), this);
    
    cluster->invalidateEntity();
}

void FastClusterBinding::onCoordinateFrameChanged()
{
    if (cluster->isFW())
    {
        FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: part %p with binding %p requests coordinate frame change", cluster, partInstance.get(), this);
    
        cluster->invalidateEntity();
        cluster->queueClusterCheck();
        cluster->markLightingDirty();
    }
}

void FastClusterBinding::onSizeChanged()
{
    FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: part %p with binding %p requests size change", cluster, partInstance.get(), this);

    cluster->invalidateEntity();
    cluster->queueClusterCheck();
    cluster->markLightingDirty();
}

void FastClusterBinding::onTransparencyChanged()
{
    FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: part %p with binding %p requests transparency change", cluster, partInstance.get(), this);

    cluster->invalidateEntity();
    cluster->markLightingDirty();
}

void FastClusterBinding::onSpecialShapeChanged()
{
    FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: part %p with binding %p requests special shape change", cluster, partInstance.get(), this);

    cluster->invalidateEntity();
    cluster->markLightingDirty();
}

void FastClusterBinding::unbind()
{
    FASTLOG4(FLog::RenderFastCluster, "FastCluster[%p]: unbind part %p from binding %p (%d connections)", cluster, partInstance.get(), this, connections.size());
    
    RBXASSERT(partInstance->getGfxPart() == cluster || partInstance->getGfxPart() == NULL);
    GfxBinding::unbind();
}

FastClusterSharedGeometry::FastClusterSharedGeometry()
{
}

void FastClusterSharedGeometry::reset()
{
    vertexBuffer.reset();
    indexBuffer.reset();
}
    
FastCluster::FastCluster(VisualEngine* visualEngine, SceneUpdater* sceneUpdater, Humanoid* humanoid, SuperCluster* owner, bool fw)
    : RenderNode(visualEngine, CullMode_SpatialHash, Flags_ShadowCaster,
		sceneUpdater->getSceneManager(), sceneUpdater->getRenderWorld())
    , humanoid(humanoid)
    , owner(owner)
	, sceneUpdater(sceneUpdater)
    , fw(fw)
    , dirty(false)
	, lightDirty(false)
	, cpuSkinningFallback(!fw && visualEngine->getRenderCaps()->getSkinningBoneCount() == 0)
	, rebuildingGeometry(false)
{
    if (humanoid)
        FASTLOG2(FLog::RenderFastCluster, "FastCluster[%p]: create (humanoid %p)", this, humanoid);
    else
	{
		const SpatialGridIndex& spatialIndex = owner->getSpatialIndex();
        FASTLOG5(FLog::RenderFastCluster, "FastCluster[%p]: create (grid %dx%dx%d, flags %d)", this, spatialIndex.position.x, spatialIndex.position.y, spatialIndex.position.z, spatialIndex.flags);
	}
    
    // register non-FW clusters for updateCoordinateFrame
    if (!fw)
		sceneUpdater->notifyAwake(this);
    
    getStatsBucket().clusters++;
}


FastCluster::~FastCluster()
{
    unbind();
            
    // delete shared geometry
    sharedGeometry.reset();

    getStatsBucket().clusters--;
    
    // notify scene updater about destruction so that the pointer to FastCluster is no longer stored
    sceneUpdater->notifyDestroyed(this);
}

void FastCluster::addPart(const boost::shared_ptr<PartInstance>& part)
{
    FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: addPart %p (part count was %d)", this, part.get(), parts.size());
    FASTLOGS(FLog::RenderFastCluster, "FastCluster part: %s", part->getFullName().c_str());
    
    Part p;
    p.instance = part.get();
    p.binding = new FastClusterBinding(this, part, sceneUpdater->getRenderRoot());
    
    parts.push_back(p);
    
    getStatsBucket().parts++;

    lightDirty = true;
}

void FastCluster::checkCluster()
{
    if (!owner) return;

    const SpatialGridIndex& spIndex = getSpatialIndex();

    FASTLOG5(FLog::RenderFastCluster, "FastCluster[%p]: checking cluster (spatial index %dx%dx%d-%u)", this, spIndex.position.x, spIndex.position.y, spIndex.position.z, spIndex.flags);

    bool selfInvalidate = false;

    for (size_t i = 0; i < parts.size(); ++i)
    {
        Part& part = parts[i];

        if (!part.binding->isBound())
        {
            continue;
        }
        else
        {
            bool isPartFW = SceneUpdater::isPartStatic(part.instance);
            SpatialGridIndex newspIndex;

            newspIndex = owner->getSpatialGrid()->getIndexUnsafe(part.instance, isPartFW ? SpatialGridIndex::fFW : 0);

            static Vector3int16 m(-1,-1,-1),M(1,1,1);

            if ( spIndex.flags != newspIndex.flags || !( spIndex.position - newspIndex.position ).isBetweenInclusive(m,M) )
            {
                if(selfInvalidate == false)
                {
                    bool result = sceneUpdater->checkAddSeenFastClusters(newspIndex);
                    RBXASSERT(result);
                    priorityInvalidateEntity();
                }

                if(!sceneUpdater->checkAddSeenFastClusters(spIndex))
                {
                    RBXASSERT(selfInvalidate == true); // Should never fail on first added cluster
                    queueClusterCheck();
                    break;
                }

                if (spIndex.flags != newspIndex.flags)
                    FASTLOG4(FLog::RenderFastCluster, "FastCluster[%p]: part %p changed FW status from %d to %d", this, part.instance, spIndex.flags, newspIndex.flags);
                else
                    FASTLOG5(FLog::RenderFastCluster, "FastCluster[%p]: part %p changed spatial index to %dx%dx%d", this, part.instance, spIndex.position.x, spIndex.position.y, spIndex.position.z);

                boost::shared_ptr<PartInstance> instance = shared_from(part.instance);

                part.binding->unbind();
                delete part.binding;
                part.binding = NULL;

                sceneUpdater->queuePartToCreate(instance);

                getStatsBucket().parts--;

                selfInvalidate = true;

                lightDirty = true;
            }
        }
    }

    if(selfInvalidate)
    {
        parts.erase(std::remove_if(parts.begin(), parts.end(), PartBindingNullPredicate()), parts.end());
        owner->checkCluster( this );
    }
}

void FastCluster::invalidateEntity()
{
    FASTLOG1(FLog::RenderFastCluster, "FastCluster[%p]: invalidateEntity", this);
        
    if (!dirty)
    {
        dirty = true;
        
        sceneUpdater->queueInvalidateFastCluster(this);
    }
}

void FastCluster::priorityInvalidateEntity()
{
    FASTLOG1(FLog::RenderFastCluster, "FastCluster[%p]: priorityInvalidateEntity", this);

    dirty = true;

    sceneUpdater->queuePriorityInvalidateFastCluster(this);
}


void FastCluster::checkBindings()
{
    for (size_t i = 0; i < parts.size(); ++i)
    {
        Part& part = parts[i];
        
        if (!part.binding->isBound())
        {
            FASTLOG2(FLog::RenderFastCluster, "FastCluster[%p]: part %p is no longer in workspace", this, part.instance);
    
            delete part.binding;
            part.binding = NULL;
        }
        else
        {
            Humanoid* newHumanoid = SceneUpdater::getHumanoid(part.instance);
            
            if (humanoid != newHumanoid)
            {
                FASTLOG4(FLog::RenderFastCluster, "FastCluster[%p]: part %p changed humanoid from %p to %p", this, part.instance, newHumanoid, humanoid);
                
                boost::shared_ptr<PartInstance> instance = shared_from(part.instance);
                
                part.binding->unbind();
                delete part.binding;
                part.binding = NULL;
                
                sceneUpdater->queuePartToCreate(instance);
            }
        }
        
        if (!part.binding)
        {
            getStatsBucket().parts--;
            
            lightDirty = true;
        }
    }
    
    parts.erase(std::remove_if(parts.begin(), parts.end(), PartBindingNullPredicate()), parts.end());
}

void FastCluster::updateEntity(bool assetsUpdated)
{
    if(!assetsUpdated && !dirty)
    {
        FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: skipping updateEntity dirty: %d, assetsUpdated: %d", this, dirty, assetsUpdated);
        return;
    }
    
    RBX::Timer<RBX::Time::Precise> timer;

    // cluster is dirty at this point if updateEntity is a reaction to invalidateEntity, but it may not be dirty if
    // scene updater decides to update the cluster after a requested asset is ready.
    FASTLOG4(FLog::RenderFastCluster, "FastCluster[%p]: updateEntity for %d parts (dirty: %d, light dirty: %d)", this, parts.size(), dirty, lightDirty);
    
    // reset dirty state before updating to check it after updating finished
    dirty = false;
    
    // update all part bindings
    checkBindings();
    
    // invalidate lighting for the old AABB
    if (lightDirty)
        invalidateLighting(getWorldBounds());
    
    // if we don't need this cluster any more, destroy it
    if (parts.empty())
    {
        FASTLOG1(FLog::RenderFastCluster, "FastCluster[%p]: destroy (no more parts)", this);
    
        if (owner)
            owner->destroyFastCluster(this); // this call deletes the object, should be the last call in this function
        else
            sceneUpdater->destroyFastCluster(this); // this call deletes the object, should be the last call in this function
        return;
    }
    
    // update cluster geometry
    AsyncResult asyncResult;
    
    updateClumpGrouping();
    unsigned int totalVertexCount = updateGeometry(&asyncResult);
    
    // subscribe for updateEntity when some pending assets are done loading
    sceneUpdater->notifyWaitingForAssets(this, asyncResult.waitingFor);
        
    // update block count used for FRM-based culling
    setBlockCount(parts.size());

    // this call updates the AABB
	rebuildingGeometry = true;
    updateCoordinateFrame(false);
	rebuildingGeometry = false;
    
    // invalidate lighting for the new AABB
    if (lightDirty)
        invalidateLighting(getWorldBounds());
        
    // reset light dirty; this means that any pending changes except for the changes in bone transforms have resulted in light invalidation
    lightDirty = false;

    // we should not do invalidateEntity from updateEntity - this results in excessive invalidations
    RBXASSERT(!dirty);
    
    FASTLOG5(FLog::RenderFastCluster, "FastCluster[%p]: updated geometry for %d parts in %d usec (%d entities, %d vertices)", this, parts.size(), (int)(timer.delta().msec() * 1000), entities.size(), totalVertexCount);
}

void FastCluster::updateClumpGrouping()
{
    // no need to update grouping for FW
    if (fw) return;
    
    // move all single-clump parts to the beginning (to make sorting cheaper)
    std::vector<Part>::iterator middle = std::partition(parts.begin(), parts.end(), PartClumpSinglePredicate());
    
    // sort remaining parts by clump to use one bone for clump
    std::sort(middle, parts.end(), PartClumpGroupPredicate());
    
    FASTLOG4(FLog::RenderFastCluster, "FastCluster[%p]: %d parts, %d single-clump parts, %d multi-clump parts", this, parts.size(), middle - parts.begin(), parts.end() - middle);
}

unsigned int FastCluster::updateGeometry(AsyncResult* asyncResult)
{
	RBXPROFILER_SCOPE("Render", "updateGeometry");

    FastClusterMeshGenerator generator(getVisualEngine(), humanoid, parts.size(), fw);
    
    const HumanoidIdentifier& hi = generator.getHumanoidIdentifier();

    // for FW cluster all parts use one pseudo-bone
    if (fw)
        generator.addBone(NULL);
    
    // generate geometry for parts
    RBX::Clump* lastClump = NULL;
    
    for (size_t parti = 0; parti < parts.size(); ++parti)
    {
        Part& part = parts[parti];
        
		// Material flags
        bool ignoreDecals = false;
		unsigned int materialFlags = MaterialGenerator::createFlags(!fw, part.instance, &hi, ignoreDecals);
		bool partTransparent = (part.instance->getTransparencyUi() > 0);
  
        // add a new bone if necessary
        Clump* clump = part.instance->getClump();
        
        if (!fw && ( !clump || clump != lastClump ) )
        {
            generator.addBone(part.instance);
            
            lastClump = clump;
        }
        
        unsigned int boneIndex = generator.getBoneCount() - 1;
        
        // add part geometry
        if (part.instance->getTransparencyUi() < 1)
			generator.addInstance(boneIndex, part.instance, NULL, materialFlags, partTransparent ? RenderQueue::Id_Transparent : RenderQueue::Id_Opaque, asyncResult);
        
        // add part decals / textures
        if ((part.instance->getCookie() & PartCookie::HAS_DECALS) && part.instance->getChildren() && !ignoreDecals)
        {
            unsigned int decalMaterialFlags = (materialFlags & MaterialGenerator::Flag_Skinned) | MaterialGenerator::Flag_Transparent;
                    
            const Instances& children = *part.instance->getChildren();
            
            for (size_t i = 0; i < children.size(); ++i)
            {
                if (Decal* decal = children[i]->fastDynamicCast<Decal>())
                {
					float decalTransparency = decal->getTransparencyUi();
					if (decalTransparency < 1)
					{
						RenderQueue::Id renderQueue = partTransparent ? RenderQueue::Id_Transparent : decalTransparency > 0 ? RenderQueue::Id_TransparentDecals : RenderQueue::Id_Decals;

						generator.addInstance(boneIndex, part.instance, decal, decalMaterialFlags, renderQueue, asyncResult);
					}
                }
            }
        }
    }

    // Destroy all existing entities
    for (size_t i = 0; i < entities.size(); ++i)
        delete entities[i];
        
    entities.clear();

    // Generate new entities
    unsigned int vertexCount = generator.finalize(this, sharedGeometry);
    
    // Retrieve bone data
    bones.resize(generator.getBoneCount());
    
    for (size_t i = 0; i < bones.size(); ++i)
    {
        bones[i].root = generator.getBoneRoot(i);
		bones[i].targetPart = generator.getBoneTargetPart(i);
		bones[i].targetBone = generator.getBoneTargetBone(i);
		bones[i].parent = generator.getBoneParent(i);
		bones[i].bindFrame = generator.getBoneBindFrame(i);
		bones[i].skinned = generator.isSkinnedBone(i);
        bones[i].localBounds = generator.getBoneBounds(i);
		bones[i].currentWorld = CoordinateFrame();
        bones[i].transform = bones[i].skinned ? CoordinateFrame()
			: (bones[i].root ? bones[i].root->getCoordinateFrame() : CoordinateFrame());
    }

    return vertexCount;
}

ClusterStats& FastCluster::getStatsBucket()
{
    RenderStats* stats = getVisualEngine()->getRenderStats();
    
    if (humanoid)
        return stats->clusterFastHumanoid;
    else if (fw)
        return stats->clusterFastFW;
    else
        return stats->clusterFast;
}

void FastCluster::updateCoordinateFrame(bool recalcLocalBounds)
{
    Extents oldWorldBB = getWorldBounds();
    Extents newWorldBB;
    
    bool bonesChanged = false;

	RBX::Time now = RBX::Time::nowFast();

    for (size_t i = 0; i < bones.size(); ++i)
    {
        Bone& bone = bones[i];

		if (bone.skinned)
		{
			CoordinateFrame currentWorld;
			if (boost::shared_ptr<PartInstance> targetPart = bone.targetPart.lock())
				currentWorld = targetPart->calcRenderingCoordinateFrame(now);
			else if (boost::shared_ptr<RBX::Bone> targetBone = bone.targetBone.lock())
				currentWorld = targetBone->getTransformedWorldCFrame();
			else if (bone.parent < bones.size())
				currentWorld = bones[bone.parent].currentWorld *
					bones[bone.parent].bindFrame.toObjectSpace(bone.bindFrame);
			else if (bone.root)
				currentWorld = bone.root->calcRenderingCoordinateFrame(now) *
					bone.bindFrame;
			else
				currentWorld = bone.bindFrame;

			const CoordinateFrame frame = currentWorld * bone.bindFrame.inverse();
			if (!Math::fuzzyEq(bone.transform, frame))
				bonesChanged = true;
			bone.currentWorld = currentWorld;
			bone.transform = frame;
		}
        else if (bone.root && !bone.root->getSleeping())
        {
            CoordinateFrame frame = bone.root->calcRenderingCoordinateFrame(now);

            if (!Math::fuzzyEq(bone.transform, frame))
                bonesChanged = true;

            bone.transform = frame;
            
            if (owner && owner->getSpatialGrid()->getIndexUnsafe(
				bone.root, fw ? SpatialGridIndex::fFW : 0) != owner->getSpatialIndex())
            {
                // Bone moved significantly, re-cluster
                queueClusterCheck();
            }
        }

        newWorldBB.expandToContain(bone.localBounds.toWorldSpace(bone.transform));
    }

    updateWorldBounds(newWorldBB);

    if (bonesChanged)
    {
        invalidateLighting(oldWorldBB);
        invalidateLighting(newWorldBB);
		if (cpuSkinningFallback && !rebuildingGeometry)
			invalidateEntity();
    }
}

void FastCluster::invalidateLighting(const Extents& bbox)
{
    if (humanoid) return;

    if (!bbox.isNull())
    {
        sceneUpdater->lightingInvalidateOccupancy(bbox, bbox.center(), fw);
    }
}

void FastCluster::unbind()
{
    FASTLOG3(FLog::RenderFastCluster, "FastCluster[%p]: unbind (%d parts, %d own connections)", this, parts.size(), connections.size());
        
    GfxPart::unbind();
    
    getStatsBucket().parts -= parts.size();
    
    for (size_t i = 0; i < parts.size(); ++i)
    {
        Part& part = parts[i];
        
        part.binding->unbind();
        delete part.binding;
    }
    
    bones.clear();
    parts.clear();
}

void FastCluster::onSleepingChanged(bool sleeping, PartInstance* part)
{
    if (!owner) return;
    
    // FW parts react on wakeup, non-FW parts react on sleep
    if ((fw && !sleeping) || (!fw && sleeping))
    {
        bool isPartFW = SceneUpdater::isPartStatic(part);
        
        if (isPartFW != fw)
        {
            FASTLOG5(FLog::RenderFastCluster, "FastCluster[%p]: part %p, sleeping %d (fw %d -> %d)", this, part, sleeping, fw, isPartFW);
            
            queueClusterCheck();
        }
    }
}

void FastCluster::onClumpChanged(PartInstance* part)
{
    if (!fw)
	{
        FASTLOG2(FLog::RenderFastCluster, "FastCluster[%p]: part %p requests clump change", this, part);
    
        invalidateEntity();
	}
}

void FastCluster::queueClusterCheck()
{
    if (!owner) return;
    sceneUpdater->queueFastClusterCheck(this, isFW());
}

void FastCluster::markLightingDirty()
{
    lightDirty = true;
}

unsigned int FastCluster::getPartCount()
{
    return parts.size();
}

const SpatialGridIndex& FastCluster::getSpatialIndex() const
{
    return owner->getSpatialIndex();
}

void FastCluster::getAllParts(std::vector<boost::shared_ptr<PartInstance>>& retParts) const
{
    retParts.resize(parts.size());
    for( int j=0,e=parts.size(); j<e; ++j )
    {
        retParts[j] = shared_from( parts[j].instance );
    }
}

}
}
