/* Copyright 2003-2014 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "audio/SoundService.h"
#include "audio/AudioGraph.h"

#include "audio/SoundChannel.h"
#include "V8DataModel/ContentProvider.h"

#include "V8DataModel/DataModel.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Camera.h"
#include "V8DataModel/Attachment.h"
#include "V8DataModel/ModelInstance.h"
#include "V8DataModel/VoiceChatService.h"

#include "util/standardout.h"
#include "V8DataModel/GameSettings.h"
#include "V8DataModel/DebugSettings.h"
#include "V8DataModel/PlayerGui.h"
#include "V8DataModel/GameBasicSettings.h"
#include "Network/Players.h"
#include "Network/Player.h"

#include "FastLog.h"
#include "rbx/RbxDbgInfo.h"

#include <algorithm>
#include <array>
#include <span>

using namespace RBX;
using namespace RBX::Soundscape;

LOGVARIABLE(Sound, 0)
DYNAMIC_LOGVARIABLE(SoundTiming, 0)
DYNAMIC_LOGVARIABLE(SoundTrace, 0)

namespace RBX
{
namespace Soundscape
{
const char* const sSoundService = "SoundService";
const char* const sSoundChannel = "Sound";

bool SoundService::soundDisabled = false;
bool SoundService::outputDeviceDisabled = false;

} // namespace Soundscape

namespace Reflection
{
template<>
EnumDesc<ReverbType>::EnumDesc()
	:EnumDescriptor("ReverbType")
{
	addPair(NoReverb, "NoReverb");
	addPair(GenericReverb, "GenericReverb");
	addPair(PaddedCell, "PaddedCell");
	addPair(Room, "Room");
	addPair(Bathroom, "Bathroom");
	addPair(LivingRoom, "LivingRoom");
	addPair(StoneRoom, "StoneRoom");
	addPair(Auditorium, "Auditorium");
	addPair(ConcertHall, "ConcertHall");
	addPair(Cave, "Cave");
	addPair(Arena, "Arena");
	addPair(Hangar, "Hangar");
	addPair(CarpettedHallway, "CarpettedHallway");
	addPair(Hallway, "Hallway");
	addPair(StoneCorridor, "StoneCorridor");
	addPair(Alley, "Alley");
	addPair(Forest, "Forest");
	addPair(City, "City");
	addPair(Mountains, "Mountains");
	addPair(Quarry, "Quarry");
	addPair(Plain, "Plain");
	addPair(ParkingLot, "ParkingLot");
	addPair(SewerPipe, "SewerPipe");
	addPair(UnderWater, "UnderWater");
}
template<>
EnumDesc<ListenerType>::EnumDesc()
	:EnumDescriptor("ListenerType")
{
	addPair(CameraListener, "Camera");
	addPair(CFrame, "CFrame");
	addPair(ObjectPosition, "ObjectPosition");
	addPair(ObjectCFrame, "ObjectCFrame");
}
template<>
EnumDesc<ListenerLocation>::EnumDesc()
	:EnumDescriptor("ListenerLocation")
{
	addPair(DefaultListenerLocation, "Default");
	addPair(NoDefaultListener, "None");
	addPair(CharacterListener, "Character");
	addPair(CameraDefaultListener, "Camera");
}
template<>
RBX::Soundscape::ListenerType& Variant::convert<RBX::Soundscape::ListenerType>(void)
{
	return genericConvert<RBX::Soundscape::ListenerType>();
}
template<>
RBX::Soundscape::ListenerLocation& Variant::convert<RBX::Soundscape::ListenerLocation>(void)
{
	return genericConvert<RBX::Soundscape::ListenerLocation>();
}
} //namespace Reflection
template<>
bool StringConverter<RBX::Soundscape::ListenerType>::convertToValue(const std::string& text, RBX::Soundscape::ListenerType& value)
{
	return Reflection::EnumDesc<RBX::Soundscape::ListenerType>::singleton().convertToValue(text.c_str(),value);
}
template<>
bool StringConverter<RBX::Soundscape::ListenerLocation>::convertToValue(const std::string& text, RBX::Soundscape::ListenerLocation& value)
{
	return Reflection::EnumDesc<RBX::Soundscape::ListenerLocation>::singleton().convertToValue(text.c_str(),value);
}
} //namespace RBX

static const Time::Interval gcTime(5);		// GC the sounds every 5 seconds
static const float decay = 0.7;

static bool toAudioVector(const G3D::Vector3& source, Audio::Vector3& destination)
{
	G3D::Vector3 value = source;
	if (Math::isDenormal(value.x)) value.x = 0;
	if (Math::isDenormal(value.y)) value.y = 0;
	if (Math::isDenormal(value.z)) value.z = 0;
	if (Math::isNanInfDenormVector3(value))
		return false;
	value = value.clamp(-100000, 100000);
	destination = {value.x, value.y, value.z};
	return true;
}

Reflection::BoundProp<float> SoundService::prop_dopplerscale("DopplerScale", category_Data, &SoundService::dopplerscale, &SoundService::on3DSettingChanged);
Reflection::BoundProp<float> SoundService::prop_distancefactor("DistanceFactor", category_Data, &SoundService::distancefactor, &SoundService::on3DSettingChanged);
Reflection::BoundProp<float> SoundService::prop_rolloffscale("RolloffScale", category_Data, &SoundService::rolloffscale, &SoundService::on3DSettingChanged);
Reflection::EnumPropDescriptor<SoundService, ReverbType> SoundService::prop_AmbientReverb("AmbientReverb", category_Data, &SoundService::getAmbientReverb, &SoundService::setAmbientReverb);
static Reflection::EnumPropDescriptor<SoundService, Enums::RolloutState>
	propCharacterSoundsUseNewApi("CharacterSoundsUseNewApi", category_Data,
		&SoundService::getCharacterSoundsUseNewApi,
		&SoundService::setCharacterSoundsUseNewApi);
static Reflection::EnumPropDescriptor<SoundService, ListenerLocation>
	propDefaultListenerLocation("DefaultListenerLocation", category_Data,
		&SoundService::getDefaultListenerLocation,
		&SoundService::setDefaultListenerLocation);
static Reflection::PropDescriptor<SoundService, CoordinateFrame>
	propListenerCFrame("ListenerCFrame", category_Data,
		&SoundService::getListenerCFrame, &SoundService::setListenerCFrame);
static Reflection::RefPropDescriptor<SoundService, Instance>
	propListenerObject("ListenerObject", category_Data,
		&SoundService::getListenerObject, &SoundService::setListenerObject);
static Reflection::EnumPropDescriptor<SoundService, ListenerType>
	propListenerType("ListenerType", category_Data,
		&SoundService::getListenerType, &SoundService::setListenerType);
Reflection::BoundFuncDesc<SoundService, void(SoundType)> func_playSound(&SoundService::playSound, "PlayStockSound", "sound", Security::RobloxScript);
Reflection::BoundFuncDesc<SoundService, void(ListenerType, shared_ptr<const RBX::Reflection::Tuple>)> func_setListener(&SoundService::setListener, "SetListener", "listenerType", "listener", Security::None);
Reflection::BoundFuncDesc<SoundService, shared_ptr<const RBX::Reflection::Tuple>()> func_getListener(&SoundService::getListener, "GetListener", Security::None);
Reflection::BoundFuncDesc<SoundService, double()> func_getMixerTime(
	&SoundService::getMixerTime, "GetMixerTime", Security::None);
Reflection::EventDesc<SoundService, void(shared_ptr<const Reflection::Tuple>)>
	event_deviceListChanged(&SoundService::deviceListChangedSignal,
		"DeviceListChanged", "newDevices", Security::RobloxScript);

SoundService::SoundService()
	:audioEngine(std::make_unique<Audio::Engine>(Audio::EngineConfig{}))
	,dopplerscale(1.0f) 
	,distancefactor(3.33f)
	,rolloffscale(1.0f)
	,ambientReverb(NoReverb)
	,characterSoundsUseNewApi(Enums::ROLLOUT_ENABLED)
	,defaultListenerLocation(DefaultListenerLocation)
	,currentListenerType(CameraListener)
    ,masterChannelFadeTimeMsec(0)
	,masterChannelFadeStatus(FADE_STATUS_NONE)
	,lastOutputDeviceEventSerial(0)
	,initialized(false)
    ,muted(false)
{
	setName(sSoundService);
}

void SoundService::setCharacterSoundsUseNewApi(Enums::RolloutState value)
{
	if (characterSoundsUseNewApi == value)
		return;
	characterSoundsUseNewApi = value;
	raisePropertyChanged(propCharacterSoundsUseNewApi);
}

void SoundService::setDefaultListenerLocation(ListenerLocation value)
{
	if (defaultListenerLocation == value)
		return;
	defaultListenerLocation = value;
	raisePropertyChanged(propDefaultListenerLocation);
	updateDefaultListener();
}

void SoundService::clearDefaultListener()
{
	if (automaticWire)
		automaticWire->setParent(nullptr);
	automaticWire.reset();
	if (automaticListener)
		automaticListener->setParent(nullptr);
	automaticListener.reset();
	if (automaticOutput)
		automaticOutput->setParent(nullptr);
	automaticOutput.reset();
	if (automaticAttachment)
		automaticAttachment->setParent(nullptr);
	automaticAttachment.reset();
}

void SoundService::updateDefaultListener()
{
	Workspace* workspace = ServiceProvider::find<Workspace>(this);
	if (!workspace)
	{
		clearDefaultListener();
		return;
	}

	ListenerLocation resolved = defaultListenerLocation;
	if (resolved == DefaultListenerLocation)
	{
		VoiceChatService* voice =
			ServiceProvider::find<VoiceChatService>(this);
		resolved = voice && voice->getEnableDefaultVoice() &&
			voice->getUseAudioApi() != Enums::AUDIO_API_DISABLED
			? CameraDefaultListener : NoDefaultListener;
	}
	if (resolved == NoDefaultListener)
	{
		clearDefaultListener();
		return;
	}

	Camera* camera = workspace->getCamera();
	Instance* listenerParent = camera;
	PartInstance* characterPart = nullptr;
	if (resolved == CharacterListener)
	{
		Network::Players* players =
			ServiceProvider::find<Network::Players>(this);
		Network::Player* player = players ? players->getLocalPlayer() : nullptr;
		ModelInstance* character = player ? player->getCharacter() : nullptr;
		characterPart = character ? character->getPrimaryPart() : nullptr;
		listenerParent = automaticAttachment.get();
		if (!characterPart || !camera)
		{
			clearDefaultListener();
			return;
		}
		if (!automaticAttachment || automaticAttachment->getParent() != characterPart)
		{
			clearDefaultListener();
			automaticAttachment =
				Creatable<Instance>::create<Attachment>();
			automaticAttachment->setName("DefaultAudioListenerAttachment");
			automaticAttachment->setParent(characterPart);
		}
		listenerParent = automaticAttachment.get();
	}
	else if (!camera)
	{
		clearDefaultListener();
		return;
	}

	if (!automaticListener || automaticListener->getParent() != listenerParent)
	{
		clearDefaultListener();
		if (resolved == CharacterListener)
		{
			automaticAttachment = Creatable<Instance>::create<Attachment>();
			automaticAttachment->setName("DefaultAudioListenerAttachment");
			automaticAttachment->setParent(characterPart);
			listenerParent = automaticAttachment.get();
		}
		automaticListener = Creatable<Instance>::create<AudioListener>();
		automaticListener->setName("DefaultAudioListener");
		automaticListener->setAudioInteractionGroup(std::string());
		automaticListener->setParent(listenerParent);
		automaticOutput = Creatable<Instance>::create<AudioDeviceOutput>();
		automaticOutput->setName("DefaultAudioDeviceOutput");
		automaticOutput->setParent(this);
		automaticWire = Creatable<Instance>::create<Wire>();
		automaticWire->setName("DefaultAudioListenerWire");
		automaticWire->setSourceInstance(automaticListener.get());
		automaticWire->setTargetInstance(automaticOutput.get());
		automaticWire->setParent(automaticListener.get());
	}

	if (resolved == CharacterListener && automaticAttachment && characterPart && camera)
	{
		CoordinateFrame desired = camera->getCameraCoordinateFrame();
		desired.translation = characterPart->getCoordinateFrame().translation;
		automaticAttachment->setFrameInPart(
			characterPart->getCoordinateFrame().toObjectSpace(desired));
	}
}

void SoundService::openAudio()
{
	FASTLOG1(DFLog::SoundTrace, "SoundService::openAudio(%p)", this);
	if (!soundDisabled && RBX::GameSettings::singleton().soundEnabled)
	{
		const char* initializationStage = "game audio settings";
		try
		{
		audioEngine->setMasterVolume(RBX::GameBasicSettings::singleton().getMasterVolume());
		initializationStage = "mute state";
		audioEngine->setMuted(muted);
		initializationStage = "output device";
		if (!outputDeviceDisabled)
		{
			try
			{
				audioEngine->startOutputDevice();
			}
			catch (const std::exception& error)
			{
				StandardOut::singleton()->printf(MESSAGE_WARNING,
					"Audio output device unavailable; mixer remains active: %s", error.what());
			}
		}
		initializationStage = "settings observer";
		gameSettingsChangedConnection = RBX::GameBasicSettings::singleton().propertyChangedSignal.connect(boost::bind(&SoundService::gameSettingsChanged,this,_1));
		initializationStage = "3D settings";
		update3DSettings();
		initializationStage = "reverb settings";
		updateAmbientReverb();
		initialized = true;
		FASTLOG(FLog::Sound, "Audio engine initialization complete.");
		}
		catch (const std::exception& error)
		{
			throw RBX::runtime_error("Audio startup failed during %s: %s",
				initializationStage, error.what());
		}
	}
}

int SoundService::getSampleRate() const
{
	FASTLOG1(DFLog::SoundTrace, "SoundService::getSampleRate(%p)", this);
	return audioEngine ? static_cast<int>(audioEngine->sampleRate()) : -1;
}

double SoundService::getMixerTime()
{
	return audioEngine ? audioEngine->mixerTimeSeconds() : 0.0;
}

SoundService::~SoundService()
{
	RBXASSERT(stockSounds.size()==0);
	RBXASSERT(loadedSounds.size()==0);
	RBXASSERT(loaded3DSounds.size()==0);
	RBXASSERT(!soundJob);
}

static void releaseSound(const std::pair<SoundId, boost::shared_ptr<Sound> >& p)
{
	FASTLOGS(DFLog::SoundTrace, "releaseSound(%s)", p.first.c_str());
	p.second->release();
}

void SoundService::closeAudio()
{
	FASTLOG1(DFLog::SoundTrace, "SoundService::closeAudio(%p)", this);
	FASTLOG(FLog::Sound, "Closing audio engine.");

	gameSettingsChangedConnection.disconnect();

	stockSounds.clear();
	this->removeAllChildren();

	std::for_each(loadedSounds.begin(), loadedSounds.end(), releaseSound);
	std::for_each(loaded3DSounds.begin(), loaded3DSounds.end(), releaseSound);

	loadedSounds.clear();
	loaded3DSounds.clear();
	audioEngine->stopOutputDevice();

	initialized = false;
	FASTLOG(FLog::Sound, "Audio engine closed.");
}

void SoundService::updateAmbientReverb()
{
	FASTLOG1(DFLog::SoundTrace, "SoundService::updateAmbientReverb(%p)", this);
	Audio::ReverbParameters parameters;
	if (ambientReverb != NoReverb)
	{
		parameters.enabled = true;
		parameters.mix = 0.22f;
		parameters.decay = 0.55f;
		parameters.damping = 0.3f;
		parameters.roomSize = 0.5f;
		switch (ambientReverb)
		{
		case PaddedCell: parameters = {true, 0.08f, 0.2f, 0.8f, 0.1f}; break;
		case Bathroom: parameters = {true, 0.32f, 0.62f, 0.18f, 0.3f}; break;
		case LivingRoom: parameters = {true, 0.15f, 0.42f, 0.55f, 0.35f}; break;
		case StoneRoom: parameters = {true, 0.3f, 0.68f, 0.18f, 0.55f}; break;
		case Auditorium: parameters = {true, 0.38f, 0.78f, 0.28f, 0.82f}; break;
		case ConcertHall: parameters = {true, 0.42f, 0.84f, 0.32f, 0.9f}; break;
		case Cave: parameters = {true, 0.5f, 0.92f, 0.12f, 1.0f}; break;
		case Arena: case Hangar: parameters = {true, 0.4f, 0.86f, 0.2f, 0.95f}; break;
		case CarpettedHallway: parameters = {true, 0.13f, 0.35f, 0.72f, 0.45f}; break;
		case Hallway: case StoneCorridor: parameters = {true, 0.3f, 0.7f, 0.22f, 0.65f}; break;
		case Forest: case City: case Mountains: case Quarry: case Plain:
		case ParkingLot: case Alley: parameters = {true, 0.12f, 0.38f, 0.5f, 0.72f}; break;
		case SewerPipe: parameters = {true, 0.46f, 0.8f, 0.25f, 0.72f}; break;
		case UnderWater: parameters = {true, 0.35f, 0.72f, 0.88f, 0.55f}; break;
		default: break;
		}
	}
	audioEngine->setReverb(parameters);
}

void SoundService::update3DSettings()
{
	FASTLOG1(DFLog::SoundTrace, "SoundService::update3DSettings(%p)", this);
	for (SoundChannels::iterator it = soundChannels.begin(); it != soundChannels.end(); ++it)
		(*it)->updateListenState(Time::Interval::zero());
}

void SoundService::setAmbientReverb(const ReverbType& value)
{
	FASTLOG1(DFLog::SoundTrace, "SoundService::setAmbientReverb(%p)", this);
	if (value!=ambientReverb)
	{
		ambientReverb = value;
		raisePropertyChanged(prop_AmbientReverb);
		updateAmbientReverb();
	}
}

void SoundService::setListener(ListenerType listenerType, shared_ptr<const RBX::Reflection::Tuple> value)
{
	if (!((value && value->values.size() > 0) || listenerType == CameraListener))
	{
		throw RBX::runtime_error("SoundService:SetListener called with an incorrect argument.");
		return;
	}

	if (listenerType == CFrame)
	{
		Reflection::ValueArray valueArray = value->values;
		Reflection::ValueArray::iterator tupleIter = valueArray.begin();
		if (tupleIter->isType<CoordinateFrame>())
		{
			currentListenerValues.listenCFrame = tupleIter->cast<CoordinateFrame>();
			raisePropertyChanged(propListenerCFrame);
		}
		else
		{
			throw RBX::runtime_error("SoundService:SetListener value given is not a valid CFrame when given Enum.ListenerType.CFrame");
			return;
		}
	}
	else if (listenerType == ObjectPosition || listenerType == ObjectCFrame)
	{
		Reflection::ValueArray valueArray = value->values;
		Reflection::ValueArray::iterator tupleIter = valueArray.begin();
		if (tupleIter->isType<shared_ptr<Instance> >())
		{
			shared_ptr<Instance> instance = tupleIter->cast<shared_ptr<Instance> >();
			if (shared_ptr<IHasLocation> location = shared_dynamic_cast<IHasLocation>(instance))
			{
				currentListenerValues.listenObject = location;
				raisePropertyChanged(propListenerObject);
			}
			else
			{
				throw RBX::runtime_error("SoundService:SetListener value given does not have a location");
				return;
			}
		}
		else
		{
			throw RBX::runtime_error("SoundService:SetListener value given is not a valid instance");
			return;
		}
	}
	currentListenerType = listenerType;
	raisePropertyChanged(propListenerType);
}

CoordinateFrame SoundService::getListenerCFrame() const
{
	Workspace* workspace = ServiceProvider::find<Workspace>(this);
	Camera* camera = workspace ? workspace->getCamera() : nullptr;
	return camera
		? const_cast<SoundService*>(this)->getListenCFrame(camera)
		: currentListenerValues.listenCFrame;
}

void SoundService::setListenerCFrame(const CoordinateFrame& value)
{
	if (currentListenerValues.listenCFrame == value)
		return;
	currentListenerValues.listenCFrame = value;
	raisePropertyChanged(propListenerCFrame);
}

Instance* SoundService::getListenerObject() const
{
	return shared_dynamic_cast<Instance>(currentListenerValues.listenObject).get();
}

void SoundService::setListenerObject(Instance* value)
{
	shared_ptr<IHasLocation> location = value
		? shared_dynamic_cast<IHasLocation>(shared_from(value))
		: shared_ptr<IHasLocation>();
	if (value && !location)
		throw RBX::runtime_error("SoundService.ListenerObject must have a world location");
	if (currentListenerValues.listenObject == location)
		return;
	currentListenerValues.listenObject = location;
	raisePropertyChanged(propListenerObject);
}

void SoundService::setListenerType(ListenerType value)
{
	if (currentListenerType == value)
		return;
	currentListenerType = value;
	raisePropertyChanged(propListenerType);
}

shared_ptr<const RBX::Reflection::Tuple> SoundService::getListener()
{
	shared_ptr<RBX::Reflection::Tuple> result(new RBX::Reflection::Tuple(2));
	result->values[0] = currentListenerType;
	if (currentListenerType == CFrame)
	{
		result->values[1] = currentListenerValues.listenCFrame;
	}
	else if (currentListenerType == ObjectPosition || currentListenerType == ObjectCFrame)
	{
		result->values[1] = shared_dynamic_cast<Instance>(currentListenerValues.listenObject);
	}
	return result;
}

void SoundService::playSound(SoundType soundType)
{
	FASTLOG1(DFLog::SoundTrace, "SoundService::playSound(%p)", this);
	StockSounds::iterator iter = stockSounds.find(soundType);
	if (iter!=stockSounds.end())
	{
		if (DFLog::SoundTrace)
		{
			std::stringstream ss;
			ss << reinterpret_cast<void*>(iter->second.get()) << ") with soundId = " << iter->second->getSoundId().toString() << ", isLoaded = " << iter->second->isSoundLoaded();
			FASTLOGS(DFLog::SoundTrace, "SoundService::playSound(%s", ss.str().c_str());
		}
		iter->second->play();
	}
}

void SoundService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	FASTLOG3(DFLog::SoundTrace, "SoundService::onServiceProvider(%p, %p, %p)", this, oldProvider, newProvider);
	const char* initializationStage = "previous audio job cleanup";
	try
	{
	TaskScheduler::singleton().removeBlocking(soundJob);
	soundJob.reset();

	if (statsItem) {
		statsItem->setParent(NULL);
		statsItem.reset();
	}

	if (oldProvider)
	{
		clearDefaultListener();
		closeAudio();
	}

	initializationStage = "service attachment";
	Super::onServiceProvider(oldProvider, newProvider);

	if (newProvider)
	{
		if (!soundDisabled)
		{
			initializationStage = "audio output startup";
			openAudio();

			initializationStage = "audio scheduler job";
			soundJob = shared_ptr<SoundJob>(new SoundJob(this));
			TaskScheduler::singleton().add(soundJob);
		}
		updateDefaultListener();
	}

	initializationStage = "audio statistics";
	Stats::StatsService* stats = ServiceProvider::create<Stats::StatsService>(newProvider);
	if (stats) {
		statsItem = SoundServiceStatsItem::create(this);
		statsItem->setParent(stats);
	}
	}
	catch (const std::exception& error)
	{
		throw RBX::runtime_error("SoundService attachment failed during %s: %s",
			initializationStage, error.what());
	}
}


CoordinateFrame SoundService::getListenCFrame(Camera* camera)
{
	CoordinateFrame cf = camera->getCameraCoordinateFrame();
	if (currentListenerType == CFrame)
	{
		cf = currentListenerValues.listenCFrame;
	}
	else if (currentListenerType == ObjectPosition)
	{
		cf.translation = currentListenerValues.listenObject->getLocation().translation;
	}
	else if (currentListenerType == ObjectCFrame)
	{
		cf = currentListenerValues.listenObject->getLocation();
	}
	return cf;
}

void SoundService::step(const Time::Interval& timeSinceLastStep)
{
	if (audioEngine)
	{
		const std::uint64_t serial = audioEngine->outputDeviceEventSerial();
		if (serial != lastOutputDeviceEventSerial)
		{
			lastOutputDeviceEventSerial = serial;
			deviceListChangedSignal(shared_ptr<const Reflection::Tuple>(
				new Reflection::Tuple()));
		}
	}
	if (enabled())
	{
		updateDefaultListener();
		Workspace* workspace = ServiceProvider::find<Workspace>(this);
		if (workspace!=NULL)
		{
			bool graphListenerApplied = false;
			workspace->visitDescendants([&](const shared_ptr<Instance>& value) {
				if (graphListenerApplied) return;
				Wire* wire = Instance::fastDynamicCast<Wire>(value.get());
				AudioListener* listener = wire && wire->getConnected()
					? Instance::fastDynamicCast<AudioListener>(wire->getSourceInstance())
					: NULL;
				if (listener && wire->getSourceName() == "Output" &&
					wire->getTargetName() == "Input" &&
					Instance::fastDynamicCast<AudioDeviceOutput>(wire->getTargetInstance()))
				{
					audioEngine->setGraphListener(listener->listenerState());
					graphListenerApplied = true;
				}
			});
			Camera* camera = workspace->getCamera();
			if (camera!=NULL)
			{
				CoordinateFrame cf = getListenCFrame(camera);
			
				Audio::ListenerState listener;
				const float distanceFactor = std::max(distancefactor, 0.001f);
				if (toAudioVector(cf.translation / distanceFactor, listener.position))
				{
					if (toAudioVector(-cf.lookVector(), listener.direction))
					{
						if (toAudioVector(cf.upVector(), listener.up))
						{
							RBX::PVInstance* pv = dynamic_cast<PVInstance*>(camera->getCameraSubject());
							RBX::PartInstance* part = pv ? pv->getPrimaryPart() : NULL;
							if (part)
								toAudioVector(part->getVelocity().linear / distanceFactor, listener.velocity);
							audioEngine->setListener(listener);
							if (!graphListenerApplied)
								audioEngine->setGraphListener(listener);
						}
					}
				}
			}
		}

		garbageCollectSounds();
	}

	updateMasterChannelGroup(timeSinceLastStep);
	updateSidechainBindings();
	updateSoundGroupBuses();
	updateSoundChannels(timeSinceLastStep);
	updateSoundGroupBuses();
	updateAudioPlayers();
}

void SoundService::updateMasterChannelGroup(const Time::Interval& timeSinceLastStep)
{
	if (!audioEngine || masterChannelFadeStatus == FADE_STATUS_NONE)
	{
		return;
	}

	const bool muting = (masterChannelFadeStatus == FADE_STATUS_OUT);
	const float storedMasterVolume = muting ? getMasterVolume() : RBX::GameBasicSettings::singleton().getMasterVolume();

	float volumeStep = 0.0f;
	if (masterChannelFadeTimeMsec <= 0.0f)
	{
		volumeStep = getMasterVolume() * (muting ? -1.0f : 1.0f);
	}
	else
	{
		volumeStep = ( (masterChannelFadeTimeMsec/timeSinceLastStep.msec() * storedMasterVolume)/masterChannelFadeTimeMsec ) * (muting ? -1.0f : 1.0f);
	}

	if (!muting)
	{
		muteAllChannels(false);
	}

	if ( muting ? (getMasterVolume() > 0.0f) : (getMasterVolume() < storedMasterVolume))
	{
		float newMasterVolume = getMasterVolume() + volumeStep;
		if (fabs(newMasterVolume) <= 0.01f)
			newMasterVolume = 0.0f;
		else if(newMasterVolume >= 0.99f)
			newMasterVolume = 1.0f;

		setMasterVolume(newMasterVolume);
	}

	if (getMasterVolume() <= 0.0f && muting)
	{
		muteAllChannels(true);
		masterChannelFadeStatus = FADE_STATUS_NONE;
	}
	else if (!muting && getMasterVolume() >= storedMasterVolume)
	{
		setMasterVolume(storedMasterVolume);
		masterChannelFadeStatus = FADE_STATUS_NONE;
	}
}

void SoundService::updateSoundChannels(const Time::Interval& timeSinceLastStep)
{
	FASTLOG1(DFLog::SoundTiming, "Number of channels = %d", soundChannels.size());
	for (SoundChannels::iterator it = soundChannels.begin(); soundChannels.end() != it; ++it)
	{
		SoundChannel *soundChannel = *it;
		soundChannel->updateListenState(timeSinceLastStep);
	}
}

Audio::BusHandle SoundService::resolveSoundGroupBus(SoundGroup* group)
{
	if (!group)
		return {};
	SoundGroup* parentGroup = Instance::fastDynamicCast<SoundGroup>(
		group->getParent());
	const Audio::BusHandle parentBus = resolveSoundGroupBus(parentGroup);
	const float volume = std::clamp(group->getVolume(), 0.0f, 10.0f);
	std::array<Audio::VoiceEffect, 32> effects{};
	const std::uint32_t effectCount = collectRuntimeSoundEffects(group, effects);

	for (SoundGroupBus& entry : soundGroupBuses)
	{
		shared_ptr<SoundGroup> current = entry.group.lock();
		if (current.get() != group)
			continue;
		if (!audioEngine->setBusVolume(entry.bus, volume))
			entry.bus = audioEngine->createBus(volume, parentBus);
		else
			audioEngine->setBusParent(entry.bus, parentBus);
		audioEngine->setBusEffects(entry.bus,
			std::span<const Audio::VoiceEffect>(effects.data(), effectCount));
		return entry.bus;
	}

	const Audio::BusHandle bus = audioEngine->createBus(volume, parentBus);
	audioEngine->setBusEffects(bus,
		std::span<const Audio::VoiceEffect>(effects.data(), effectCount));
	soundGroupBuses.push_back({weak_from(group), bus});
	return bus;
}

void SoundService::updateSoundGroupVolume(SoundGroup* group)
{
	if (!group)
		return;
	for (SoundGroupBus& entry : soundGroupBuses)
	{
		shared_ptr<SoundGroup> current = entry.group.lock();
		if (current.get() == group)
		{
			audioEngine->setBusVolume(entry.bus,
				std::clamp(group->getVolume(), 0.0f, 10.0f));
			return;
		}
	}
}

namespace {

bool isSidechainSource(const Instance* instance)
{
	return Instance::fastDynamicCast<const SoundChannel>(instance) ||
		Instance::fastDynamicCast<const SoundGroup>(instance);
}

bool sidechainPathReaches(const Instance* current, const Instance* target,
	boost::unordered_set<const Instance*>& visited)
{
	if (!current || current == target)
		return current == target;
	if (!visited.insert(current).second || !current->getChildren())
		return false;
	const shared_ptr<const Instances> children = current->getChildren().read();
	if (!children)
		return false;
	for (const shared_ptr<Instance>& child : *children)
	{
		const CompressorSoundEffect* compressor =
			Instance::fastDynamicCast<CompressorSoundEffect>(child.get());
		if (!compressor || !compressor->getEnabled())
			continue;
		const Instance* next = compressor->getSideChain();
		if (isSidechainSource(next) &&
			sidechainPathReaches(next, target, visited))
			return true;
	}
	return false;
}

}

void SoundService::updateSidechainBindings()
{
	boost::unordered_set<Instance*> parents;
	for (SoundChannel* channel : soundChannels)
		if (channel)
			parents.insert(channel);
	for (const SoundGroupBus& entry : soundGroupBuses)
		if (shared_ptr<SoundGroup> group = entry.group.lock())
			parents.insert(group.get());

	boost::unordered_set<const Instance*> activeSources;
	for (Instance* parent : parents)
	{
		const shared_ptr<const Instances> children = parent->getChildren().read();
		if (!children)
			continue;
		for (const shared_ptr<Instance>& child : *children)
		{
			CompressorSoundEffect* compressor =
				Instance::fastDynamicCast<CompressorSoundEffect>(child.get());
			if (!compressor)
				continue;
			compressor->setSideChainMeter({});
			Instance* source = compressor->getSideChain();
			if (!compressor->getEnabled() || !isSidechainSource(source) ||
				ServiceProvider::find<SoundService>(source) != this)
				continue;
			boost::unordered_set<const Instance*> visited;
			if (sidechainPathReaches(source, parent, visited))
				continue;
			std::array<Audio::VoiceEffect, 32> sourceEffects{};
			if (collectSoundEffects(source, sourceEffects) >= sourceEffects.size())
				continue;
			std::shared_ptr<Audio::MeterState>& meter = sidechainMeters[source];
			if (!meter)
				meter = std::make_shared<Audio::MeterState>();
			compressor->setSideChainMeter(meter);
			activeSources.insert(source);
		}
	}

	for (SidechainMeters::iterator it = sidechainMeters.begin();
		it != sidechainMeters.end();)
	{
		if (activeSources.find(it->first) == activeSources.end())
			it = sidechainMeters.erase(it);
		else
			++it;
	}
}

std::uint32_t SoundService::collectRuntimeSoundEffects(const Instance* parent,
	std::array<Audio::VoiceEffect, 32>& effects)
{
	std::uint32_t count = collectSoundEffects(parent, effects);
	const SidechainMeters::const_iterator meter = sidechainMeters.find(parent);
	if (meter == sidechainMeters.end() || count >= effects.size())
		return count;
	Audio::VoiceEffect& analyzer = effects[count++];
	analyzer.type = Audio::VoiceEffectType::Analyzer;
	analyzer.parameters[2] = static_cast<float>(
		reinterpret_cast<std::uintptr_t>(parent) & 0x00ffffffu);
	analyzer.meter = meter->second;
	return count;
}

void SoundService::updateSoundGroupBuses()
{
	std::vector<shared_ptr<SoundGroup>> liveGroups;
	liveGroups.reserve(soundGroupBuses.size());
	for (const SoundGroupBus& entry : soundGroupBuses)
		if (shared_ptr<SoundGroup> group = entry.group.lock())
			liveGroups.push_back(group);
	for (const shared_ptr<SoundGroup>& group : liveGroups)
		resolveSoundGroupBus(group.get());

	for (std::vector<SoundGroupBus>::iterator it = soundGroupBuses.begin();
		it != soundGroupBuses.end();)
	{
		if (!it->group.expired())
		{
			++it;
		}
		else if (audioEngine->destroyBus(it->bus))
		{
			it = soundGroupBuses.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void SoundService::updateAudioPlayers()
{
	// Ended/Looped handlers are allowed to destroy or reparent graph nodes.
	// Iterate a snapshot so unregisterAudioPlayer() cannot invalidate the live
	// iterator while an AudioPlayer callback is running.
	const AudioPlayers snapshot = audioPlayers;
	for (AudioPlayer* player : snapshot)
	{
		if (audioPlayers.find(player) != audioPlayers.end())
			player->update();
	}
}


void SoundService::getCpuStats(CpuStats& stats) const
{
	stats.dsp = 0;
	stats.stream = 0;
	stats.geometry = 0;
	stats.update = 0;
	stats.total = 0;
}

void SoundService::getSoundStats(const LoadedSounds& sounds, unsigned int& numSounds, unsigned int& numUnusedSounds)
{
	for (LoadedSounds::const_iterator iter = sounds.begin(); iter!=sounds.end(); ++iter)
	{
		++numSounds;
		if (!iter->second->isReferenced())
			++numUnusedSounds;
	}
}

void SoundService::getSoundStats(unsigned int& numSounds, unsigned int& numUnusedSounds) const
{
	numSounds = numUnusedSounds = 0;
	getSoundStats(loadedSounds, numSounds, numUnusedSounds);
	getSoundStats(loaded3DSounds, numSounds, numUnusedSounds);
}

void SoundService::getChannelsPlaying(int& value) const
{
	FASTLOG1(DFLog::SoundTrace, "SoundService::getChannelsPlaying(%p)", this);
	value = audioEngine ? static_cast<int>(audioEngine->activeVoiceCount()) : 0;
}

bool SoundService::isMuted()
{
	return audioEngine && audioEngine->muted();
}

void SoundService::muteAllChannels(bool mute)
{
	FASTLOG1(DFLog::SoundTrace, "SoundService::muteAllChannels(%p)", this);

	muted = mute;
	if (audioEngine)
		audioEngine->setMuted(mute);
}

void SoundService::gameSettingsChanged(const Reflection::PropertyDescriptor* propertyDescriptor)
{
	if (propertyDescriptor == &RBX::GameBasicSettings::singleton().prop_masterVolume)
	{
		setMasterVolume(RBX::GameBasicSettings::singleton().getMasterVolume());
	}
}

void SoundService::setMasterVolume(float value)
{
	if (audioEngine)
		audioEngine->setMasterVolume(value);
}

float SoundService::getMasterVolume()
{
	return audioEngine ? audioEngine->masterVolume() : -1.0f;
}

void SoundService::setMasterVolumeFadeIn(float timeToFadeMsec)
{
	masterChannelFadeTimeMsec = G3D::clamp(timeToFadeMsec, 0.0f, 100000.0f);
	masterChannelFadeStatus = FADE_STATUS_IN;
	}
void SoundService::setMasterVolumeFadeOut(float timeToFadeMsec)
{
	masterChannelFadeTimeMsec = G3D::clamp(timeToFadeMsec, 0.0f, 100000.0f);
	masterChannelFadeStatus = FADE_STATUS_OUT;
}

void SoundService::gcSounds(LoadedSounds& sounds)
{
	for (LoadedSounds::iterator iter = sounds.begin(); iter!=sounds.end();)
	{
		if (!iter->second->isReferenced() && G3D::uniformRandom()>=decay)
		{
			FASTLOGS(DFLog::SoundTrace, "Garbage collecting soundId = %s", iter->second->id.c_str());
			iter->second->release();
			sounds.erase(iter++);
		}
		else
		{
			++iter;
		}
	}
}

void SoundService::garbageCollectSounds()
{
	if (nextGarbageCollectTime > Time::now<Time::Fast>())
		return;

	// Now flush the loadedSounds database of empty items
	gcSounds(loadedSounds);
	gcSounds(loaded3DSounds);

	nextGarbageCollectTime = Time::now<Time::Fast>() + gcTime;
}

void SoundService::registerSoundChannel(SoundChannel *soundChannel)
{
	if (FLog::Sound)
	{
		std::stringstream ss;
		ss << "soundChannel = " << reinterpret_cast<void*>(soundChannel) << ", soundId = " << soundChannel->getSoundId().c_str();
		FASTLOGS(FLog::Sound, "Registering with SoundService: %s", ss.str().c_str());
	}

	soundChannels.insert(soundChannel);
}

void SoundService::unregisterSoundChannel(SoundChannel *soundChannel)
{
	SoundChannels::iterator iter = soundChannels.find(soundChannel);
	if (soundChannels.end() != iter)
	{
		if (FLog::Sound)
		{
			SoundChannel *soundChannel = *iter;
			std::stringstream ss;
			ss << "soundChannel = " << reinterpret_cast<void*>(soundChannel) << ", soundId = " << soundChannel->getSoundId().c_str();
			FASTLOGS(FLog::Sound, "Unregistering from SoundService: %s", ss.str().c_str());
		}
		soundChannels.erase(iter);
	}
}

void SoundService::registerAudioPlayer(AudioPlayer* audioPlayer)
{
	audioPlayers.insert(audioPlayer);
}

void SoundService::unregisterAudioPlayer(AudioPlayer* audioPlayer)
{
	audioPlayers.erase(audioPlayer);
}

shared_ptr<Sound> SoundService::loadSound(SoundId id, bool is3D)
{
	LoadedSounds& database(is3D ? loaded3DSounds : loadedSounds);

	LoadedSounds::iterator iter = database.find(id);
	// don't return this sound object if we are streaming, each stream needs a new object
	if (iter!=database.end() && !iter->second->getIsStreaming())
	{
		return iter->second;
	}
	shared_ptr<Sound> result(new Sound(*audioEngine, id, is3D));
	database[id] = result;
	return result;
}
