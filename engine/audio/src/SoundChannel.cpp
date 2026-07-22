/* Copyright 2003-2014 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "audio/SoundChannel.h"
#include "v8datamodel/ContentProvider.h"
#include "audio/SoundService.h"

#include "v8datamodel/Workspace.h"
#include "v8datamodel/PartInstance.h"

#include "v8datamodel/GameSettings.h"
#include "v8datamodel/PlayerGui.h"
#include "network/Players.h"

#include "FastLog.h"
#include "rbx/RbxDbgInfo.h"

#include <cmath>

LOGGROUP(Sound)
DYNAMIC_LOGGROUP(SoundTiming)
DYNAMIC_LOGGROUP(SoundTrace)

FASTINTVARIABLE(MinMsecBetweenTimePosEventReplication, 100)
FASTINTVARIABLE(MinSecondLengthForLongSoundChannel, 5)
FASTSTRINGVARIABLE(AssetTypeHeaderForSounds, "")
DYNAMIC_FASTFLAGVARIABLE(SoundFailedToLoadContext, false)
DYNAMIC_FASTFLAGVARIABLE(MinMaxDistanceEnabled, false)
DYNAMIC_FASTFLAGVARIABLE(RollOffModeEnabled, false)

namespace RBX
{

namespace Reflection
{
template<>
EnumDesc<RBX::Soundscape::RollOffMode>::EnumDesc()
	:EnumDescriptor("RollOffMode")
{
	addPair(RBX::Soundscape::Inverse, "Inverse");
	addPair(RBX::Soundscape::Linear, "Linear");
}
template<>
RBX::Soundscape::RollOffMode& Variant::convert<RBX::Soundscape::RollOffMode>(void)
{
	return genericConvert<RBX::Soundscape::RollOffMode>();
}
} //namespace Reflection
template<>
bool StringConverter<RBX::Soundscape::RollOffMode>::convertToValue(const std::string& text, RBX::Soundscape::RollOffMode& value)
{
	return Reflection::EnumDesc<RBX::Soundscape::RollOffMode>::singleton().convertToValue(text.c_str(),value);
}
}

namespace RBX
{

void registerSound()
{
    Soundscape::SoundChannel::classDescriptor();
}

namespace Soundscape
{
REFLECTION_BEGIN();
static Reflection::PropDescriptor<SoundChannel, SoundId> sound_desc_SoundId("SoundId", category_Data, &SoundChannel::getSoundId, &SoundChannel::setSoundId);
static Reflection::PropDescriptor<SoundChannel, float> sound_desc_Volume("Volume", category_Data, &SoundChannel::getVolume, &SoundChannel::setVolume);
static Reflection::RefPropDescriptor<SoundChannel, SoundGroup> sound_desc_SoundGroup(
	"SoundGroup", category_Data, &SoundChannel::getSoundGroup,
	&SoundChannel::setSoundGroup);
static Reflection::PropDescriptor<SoundChannel, float> sound_desc_Pitch("Pitch", category_Data, &SoundChannel::getPitch, &SoundChannel::setPitch);
static Reflection::PropDescriptor<SoundChannel, float> sound_desc_PlaybackSpeed("PlaybackSpeed", category_Data, &SoundChannel::getPlaybackSpeed, &SoundChannel::setPlaybackSpeed);
static Reflection::PropDescriptor<SoundChannel, float> sound_desc_MinDistance("MinDistance", category_Data, &SoundChannel::getMinDistance, &SoundChannel::setMinDistance);
static Reflection::PropDescriptor<SoundChannel, float> sound_desc_MaxDistance("MaxDistance", category_Data, &SoundChannel::getMaxDistance, &SoundChannel::setMaxDistance);
static Reflection::PropDescriptor<SoundChannel, float> sound_desc_RollOffMinDistance("RollOffMinDistance", category_Data, &SoundChannel::getMinDistance, &SoundChannel::setMinDistance);
static Reflection::PropDescriptor<SoundChannel, float> sound_desc_RollOffMaxDistance("RollOffMaxDistance", category_Data, &SoundChannel::getMaxDistance, &SoundChannel::setMaxDistance);
static Reflection::EnumPropDescriptor<SoundChannel, RollOffMode> sound_desc_RollOffMode("RollOffMode", category_Data, &SoundChannel::getRollOffMode, &SoundChannel::setRollOffMode);

static Reflection::PropDescriptor<SoundChannel, double> sound_desc_SoundLength("TimeLength", category_Data, &SoundChannel::getSoundLength, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::PropDescriptor<SoundChannel, double> sound_desc_SoundPosition("TimePosition", category_Data, &SoundChannel::getSoundPosition, &SoundChannel::setSoundPositionLua, Reflection::PropertyDescriptor::UI);

static Reflection::PropDescriptor<SoundChannel, bool> sound_desc_Looped("Looped", category_Behavior, &SoundChannel::getLooped, &SoundChannel::setLooped);
static Reflection::PropDescriptor<SoundChannel, bool> sound_desc_AcousticSimulationEnabled("AcousticSimulationEnabled", category_Behavior, &SoundChannel::getAcousticSimulationEnabled, &SoundChannel::setAcousticSimulationEnabled);
static Reflection::PropDescriptor<SoundChannel, bool> sound_desc_PlaybackRegionsEnabled("PlaybackRegionsEnabled", category_Behavior, &SoundChannel::getPlaybackRegionsEnabled, &SoundChannel::setPlaybackRegionsEnabled);
static Reflection::PropDescriptor<SoundChannel, NumberRange> sound_desc_LoopRegion("LoopRegion", category_Data, &SoundChannel::getLoopRegion, &SoundChannel::setLoopRegion);
static Reflection::PropDescriptor<SoundChannel, NumberRange> sound_desc_PlaybackRegion("PlaybackRegion", category_Data, &SoundChannel::getPlaybackRegion, &SoundChannel::setPlaybackRegion);
Reflection::BoundProp< bool> SoundChannel::sound_desc_playOnRemove("PlayOnRemove", category_Behavior, &SoundChannel::playOnRemove);

static Reflection::BoundFuncDesc<SoundChannel, void()> sound_playFunction(&SoundChannel::play, "Play", Security::None);
static Reflection::BoundFuncDesc<SoundChannel, void()> sound_resumeFunction(&SoundChannel::resume, "Resume", Security::None);
static Reflection::BoundFuncDesc<SoundChannel, void()> sound_pauseFunction(&SoundChannel::pause, "Pause", Security::None);
static Reflection::BoundFuncDesc<SoundChannel, void()> sound_stopFunction(&SoundChannel::stop, "Stop", Security::None);

static Reflection::PropDescriptor<SoundChannel, bool> sound_desc_IsPlaying("IsPlaying", category_Data, &SoundChannel::isPlaying, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::PropDescriptor<SoundChannel, bool> sound_desc_Playing("Playing", category_Data, &SoundChannel::isPlaying, &SoundChannel::setPlaying);
static Reflection::PropDescriptor<SoundChannel, bool> sound_desc_isPaused("IsPaused", category_Data, &SoundChannel::isPaused, NULL, Reflection::PropertyDescriptor::UI);

static Reflection::EventDesc<SoundChannel, void(std::string, int)> sound_loopedSignal(&SoundChannel::soundLoopedSignal, "DidLoop", "soundId", "numOfTimesLooped");
static Reflection::EventDesc<SoundChannel, void(std::string)> sound_pausedSignal(&SoundChannel::soundPausedSignal, "Paused", "soundId");
static Reflection::EventDesc<SoundChannel, void(std::string)> sound_playedSignal(&SoundChannel::soundPlayedSignal, "Played", "soundId");
static Reflection::EventDesc<SoundChannel, void(std::string)> sound_stoppedSignal(&SoundChannel::soundStoppedSignal, "Stopped", "soundId");
static Reflection::EventDesc<SoundChannel, void(std::string)> sound_endedSignal(&SoundChannel::soundEndedSignal, "Ended", "soundId");


//////////////////////////////////////////////////////
// Backend Events/Properties
/////////////////////////////////////////////////
static Reflection::PropDescriptor<SoundChannel, int> sound_desc_PlayCount("PlayCount", category_Data, &SoundChannel::getPlayCount, &SoundChannel::setPlayCount, Reflection::PropertyDescriptor::REPLICATE_ONLY);
static Reflection::RemoteEventDesc<SoundChannel, void(int)> event_timePositionUpdatedFromServer(&SoundChannel::timePositionUpdatedFromServerSignal, "TimePositionUpdated", "newPositionSeconds", Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<SoundChannel, void(int)> event_timePositionUpdatedFromServerScript(&SoundChannel::timePositionUpdatedFromServerScriptSignal, "TimePositionUpdatedFromScript", "newPositionSeconds", Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<SoundChannel, void(int)> event_soundResumedFromServer(&SoundChannel::soundResumedFromServerSignal, "SoundResumedFromServer", "currentTimePosition", Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);


//////////////////////////////////////////////////////
// DEPRECATED LUA FUNCTIONS/PROPS
/////////////////////////////////////////////////
static Reflection::BoundFuncDesc<SoundChannel, void()> sound_stopFunctionDep(&SoundChannel::stop, "stop", Security::None, Reflection::Descriptor::Attributes::deprecated(sound_stopFunction));
static Reflection::BoundFuncDesc<SoundChannel, void()> sound_playFunctionDeprecated(&SoundChannel::play, "play", Security::None, Reflection::Descriptor::Attributes::deprecated(sound_playFunction));
static Reflection::BoundFuncDesc<SoundChannel, void()> sound_dep_pauseFunction(&SoundChannel::pause, "pause", Security::None, Reflection::Descriptor::Attributes::deprecated(sound_pauseFunction));
static Reflection::PropDescriptor<SoundChannel, bool> sound_desc_dep_IsPlaying("isPlaying", category_Data, &SoundChannel::isPlaying, NULL, Reflection::PropertyDescriptor::Attributes::deprecated(sound_desc_IsPlaying));
REFLECTION_END();

SoundChannel::SoundChannel()
	:looped(false)
	,is3D(false)
	,playOnRemove(false)
	,audioEngine(NULL)
	,voice()
	,part(NULL)
	,volume(0.5)
	,pitch(1)
	,minDistance(10)
	,maxDistance(100000)
	,rollOff(Inverse)
	,acousticSimulationEnabled(true)
	,playbackRegionsEnabled(false)
	,loopRegion(0.0f, 60000.0f)
	,playbackRegion(0.0f, 60000.0f)
	,soundDisabled(false)
	,playCount(-1)
	,reqPlayCount(-1)
	,soundPositionSeconds(0)
	,invalidChannel(true)
	,numOfTimesLooped(0)
	,lastSoundPositionMsec(0)
{
	this->setName("Sound");
}

void SoundChannel::setAcousticSimulationEnabled(bool value)
{
	if (acousticSimulationEnabled != value)
	{
		acousticSimulationEnabled = value;
		raisePropertyChanged(sound_desc_AcousticSimulationEnabled);
	}
}

void SoundChannel::setPlaybackRegionsEnabled(bool value)
{
	if (playbackRegionsEnabled != value)
	{
		playbackRegionsEnabled = value;
		raisePropertyChanged(sound_desc_PlaybackRegionsEnabled);
	}
}

void SoundChannel::setLoopRegion(NumberRange value)
{
	if (loopRegion != value)
	{
		loopRegion = value;
		raisePropertyChanged(sound_desc_LoopRegion);
	}
}

void SoundChannel::setPlaybackRegion(NumberRange value)
{
	if (playbackRegion != value)
	{
		playbackRegion = value;
		raisePropertyChanged(sound_desc_PlaybackRegion);
	}
}

SoundChannel::~SoundChannel()
{
	FASTLOG2(DFLog::SoundTrace, "SoundChannel::~SoundChannel(%p, %p)", this, sound.get());
    releaseChannel();
	RBXASSERT(!voice);
	RBXASSERT(!sound);
}


void SoundChannel::releaseChannel()
{
	FASTLOG1(DFLog::SoundTrace, "SoundChannel::releaseChannel(%p)", this);
	if (voice && audioEngine)
	{
		audioEngine->stop(voice);
		audioEngine->destroyVoice(voice);
		voice = {};
		voiceBus = {};
		if (sound)
			sound->unacquire();
	}
	audioEngine = NULL;
	if (sound)
	{
        sound.reset();
	}
}

bool SoundChannel::askSetParent(const Instance* instance) const
{
	// Sounds can be a child of anything
	return true;
}


void SoundChannel::updateListenState(const Time::Interval& timeSinceLastStep)
{
	if (voice && audioEngine)
	{
		synchronizeSoundGroupBus();
		synchronizeSoundEffects();
		update3D();
		if (!getLooped() && audioEngine->isFinished(voice))
			onChannelEnd();
	}

	if (controlledByAndIsServer() && (timeSinceLastStep.msec() > 0))
	{
		// we aren't actually playing sounds, but should keep the scrub in a good location
		// for client replication (say we start playing a sound in workspace when it is actually half over)

		double position = soundPositionSeconds;
		if (isPlaying() && !isPaused())
		{
			position += (timeSinceLastStep.seconds() * getPitch());
			double soundLength = getSoundLength();
			
			if (!getLooped() && (position > soundLength) && sound)
			{
				position = 0;
				DataModel::scoped_write_request request(RBX::DataModel::get(this));
				stop();
			}
			else
			{
				if ( getLooped() && isPlaying() && (position > soundLength) )
				{
					numOfTimesLooped++;

					DataModel::scoped_write_request request(RBX::DataModel::get(this));
					soundLoopedSignal(getSoundId().toString(), numOfTimesLooped);
				}

				position = fmod(position, (soundLength ? soundLength : 1));
				DataModel::scoped_write_request request(RBX::DataModel::get(this));
				setSoundPosition(position);
			}
		}
	}
	else if (!controlledByAndIsServer() && voice && getLooped() && isPlaying())
	{
		const unsigned currentPosMs = static_cast<unsigned>(getSoundPosition() * 1000.0);

		if (currentPosMs < lastSoundPositionMsec)
		{
			numOfTimesLooped++;

			DataModel::scoped_write_request request(RBX::DataModel::get(this));
			soundLoopedSignal(getSoundId().toString(), numOfTimesLooped);
		}

		lastSoundPositionMsec = currentPosMs;
	}
}

void SoundChannel::onAncestorChanged(const AncestorChanged& event)
{
	FASTLOG3(DFLog::SoundTrace, "SoundChannel::onAncestorChanged(%p, %p), event.child = %p", this, event.newParent, event.child);
	Super::onAncestorChanged(event);

	if (event.child==this)
	{
		part = Instance::fastDynamicCast<PartInstance>(event.newParent);
	}

	if (playOnRemove && !getLooped())
	{
		// If this Sound or one of its parents is being removed from the Workspace, then play the sound!
		const Instance* oldWorkspace = ServiceProvider::find<Workspace>(event.oldParent);
		const bool wasInWorkspace = oldWorkspace && (event.oldParent==oldWorkspace || event.oldParent->isDescendantOf(oldWorkspace));

		if (wasInWorkspace)
		{
			const Instance* newWorkspace = ServiceProvider::find<Workspace>(event.newParent);
			const bool isInWorkspace = newWorkspace && (event.newParent==newWorkspace || event.newParent->isDescendantOf(newWorkspace));
			if (!isInWorkspace)
			{
				FASTLOG1(DFLog::SoundTrace, "Play on remove with SoundChannel %p", this);
				RBXASSERT(oldWorkspace);
                loadSound(oldWorkspace, true);
			}
		}
	}
}

void SoundChannel::serverUpdatedTimePositionFromScript(unsigned int timePosition)
{
	// a server script updated time position, make sure we update too
	setSoundPosition(timePosition);
}

void SoundChannel::serverUpdatedTimePosition(unsigned int timePosition)
{
	// only get initial position from server, then don't worry about this type of update
	serverUpdatedTimeConnection.disconnect();
	setSoundPosition(timePosition);
}

void SoundChannel::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	FASTLOG5(DFLog::SoundTrace, "SoundChannel::onServiceProvider(%p, %p, %p, %d, %d)", this, oldProvider, newProvider, playCount, reqPlayCount);
	if (oldProvider)
	{
		// Looped sounds should be turned off when we leave the world (like a rocket engine)
		// Non-looped sounds can finish playing (like an explosion)
		if (getLooped())
		{
			FASTLOG1(DFLog::SoundTrace, "SoundChannel::onServiceProvider(%p) calling stop on looped sound.", this);
			stop();
		}
		
		releaseChannel();
        SoundService* soundService = ServiceProvider::create<SoundService>(oldProvider);
        if (soundService)
        {
            soundService->unregisterSoundChannel(this);
        }

		serverUpdatedTimeConnection.disconnect();
		serverScriptUpdatedTimeConnection.disconnect();
		serverResumedSoundConnection.disconnect();
	}

	Super::onServiceProvider(oldProvider,newProvider);

	if (newProvider)
	{
		SoundService* soundService = ServiceProvider::create<SoundService>(newProvider);
        soundService->registerSoundChannel(this);
        part = Instance::fastDynamicCast<PartInstance>(this->getParent());
        FASTLOG2(DFLog::SoundTrace, "SoundChannel::onServiceProvider(%p) setting part to %p", this, part);
		if (soundService && soundService->enabled())
		{
			soundDisabled = false;	// Worth trying again to see if we have a working sound manager
			FASTLOG2(DFLog::SoundTrace, "SoundChannel::onServiceProvider(%p), playCount < reqPlayCount = %d", this, playCount < reqPlayCount);
            loadSound(this, playCount < reqPlayCount);
            playCount = reqPlayCount;
		}

		lastTimePosReplication.reset();

		if (RBX::Network::Players::clientIsPresent(newProvider))
		{
			// only get initial position update from server if sound is replicated to each client
			if (isHeardGlobally())
			{
				serverUpdatedTimeConnection = timePositionUpdatedFromServerSignal.connect(boost::bind(&SoundChannel::serverUpdatedTimePosition, this, _1));
			}

			serverScriptUpdatedTimeConnection = timePositionUpdatedFromServerScriptSignal.connect(boost::bind(&SoundChannel::serverUpdatedTimePositionFromScript, this, _1));
			serverResumedSoundConnection = soundResumedFromServerSignal.connect(boost::bind(&SoundChannel::resume, this));
		}
	}
}

namespace {
	void onSoundLoadedForSoundChannel(shared_ptr<SoundChannel> soundChannel, const SoundId &soundId, const Instance *context, bool shouldPlayOnLoad, AsyncHttpQueue::RequestResult result, shared_ptr<const std::string> item)
	{
		switch (result)
		{
		case AsyncHttpQueue::Failed:
			FASTLOGS(FLog::Sound, "onSoundLoaded Failed to load %s", soundId.c_str());
			if (DFFlag::SoundFailedToLoadContext && context && !soundId.isNull())
			{
				RBX::StandardOut::singleton()->printf(MESSAGE_ERROR, "Sound failed to load %s : %s", context->getFullName().c_str(), soundId.c_str());
			}
			break;
		case AsyncHttpQueue::Waiting:
			FASTLOGS(FLog::Sound, "onSoundLoaded Waiting for %s", soundId.c_str());
			break;
		case AsyncHttpQueue::Succeeded:
			FASTLOGS(FLog::Sound, "onSoundLoaded Succeeded in loading %s", soundId.c_str());

			// Make sure we only trigger an onSoundLoaded event if our callback contained the data we want
			// and we don't currently have the sound loaded.
			if (soundChannel->getSoundId() == soundId)
			{
				soundChannel->onSoundLoaded(context, shouldPlayOnLoad);
			}
			break;
		}
	}
} // namespace

void SoundChannel::onSoundLoaded(const Instance *context, bool shouldPlayOnLoad)
{
	FASTLOG3(DFLog::SoundTrace, "SoundChannel::onSoundLoaded(%p, %p, %d)", this, context, shouldPlayOnLoad);

	is3D = part != NULL;

	SoundService* soundService = ServiceProvider::create<SoundService>(context);
	if (soundService)
	{
		sound = soundService->loadSound(getSoundId(), is3D);

		raisePropertyChanged(sound_desc_SoundLength);

		// Only play our sound at the appropriate time.
		if (shouldPlayOnLoad)
		{
			playLocal(context);
		}
	}
}

bool SoundChannel::isPaused() const
{
    FASTLOG1(DFLog::SoundTrace, "SoundChannel::isPaused(%p)", this);

	if (!voice || !audioEngine)
	{
        return playCount <= 0;
	}
	return audioEngine->isPaused(voice);
}

bool SoundChannel::isPlaying() const
{
	FASTLOG1(DFLog::SoundTrace, "SoundChannel::isPlaying(%p)", this);
	if (!voice || !audioEngine)
	{
		if (RBX::Network::Players::clientIsPresent(this, false))
		{
			return false;
		}

		return playCount > 0;
	}

	return audioEngine->isPlaying(voice);
}

bool SoundChannel::isSoundLoaded() const
{
	return sound != NULL && sound->id == getSoundId();
}

void SoundChannel::loadSound(const Instance *context, bool shouldPlayOnLoad)
{
	FASTLOG3(DFLog::SoundTrace, "SoundChannel::loadSound(%p, %p, %d)", this, context, shouldPlayOnLoad);
	if (!SoundService::soundDisabled && !soundDisabled && RBX::GameSettings::singleton().soundEnabled)
	{
		ContentProvider* contentProvider = ServiceProvider::create<ContentProvider>(context);
		FASTLOG2(DFLog::SoundTrace, "SoundChannel::loadSound(%p), contentProvider = %p", this, contentProvider);
		if (contentProvider)
		{
			if (!isSoundLoaded())
			{
				if (contentProvider->hasContent(getSoundId()))
				{
					FASTLOGS(FLog::Sound, "Loading cached soundId = %s", getSoundId().c_str());
					onSoundLoaded(context, shouldPlayOnLoad);
				}
				else
				{
					FASTLOGS(FLog::Sound, "Fetching soundId = %s", getSoundId().c_str());
					contentProvider->preloadContentWithCallback(soundId, ContentProvider::PRIORITY_SOUND, 
						boost::bind(&onSoundLoadedForSoundChannel, shared_from(this), soundId, context, shouldPlayOnLoad, _1, shared_ptr<const std::string>()),
						AsyncHttpQueue::AsyncWrite,
						FString::AssetTypeHeaderForSounds
					);
				}
			}
			else
			{
				FASTLOGS(FLog::Sound, "Already loaded soundId = %s", getSoundId().c_str());
				onSoundLoaded(context, shouldPlayOnLoad);
			}
		}
	}
}

void SoundChannel::setSoundId(SoundId value)
{
	if (DFLog::SoundTrace)
	{
		std::stringstream ss;
		ss << reinterpret_cast<void*>(this) << ", " << value.toString() << ")";
		FASTLOGS(DFLog::SoundTrace, "SoundChannel::setSoundId(%s", ss.str().c_str());
	}

	if (value != soundId)
	{
		soundId = value;
        releaseChannel();
        loadSound(this, false);
		raiseChanged(sound_desc_SoundId);
	}
}

const SoundId &SoundChannel::getSoundId() const
{
	return soundId;
}

float SoundChannel::getVolume() const
{
	return volume;
}

double SoundChannel::getSoundLength() const
{
	FASTLOG1(DFLog::SoundTrace, "SoundChannel::getSoundLength(%p)", this);
	if (!sound)
		return 0.0;
	sound->tryLoad(this);
	return sound->getLengthSeconds();
}

double SoundChannel::getSoundPosition() const
{
	FASTLOG1(DFLog::SoundTrace, "SoundChannel::getSoundPosition(%p)", this);
	double position = 0.0;

	// The server emulates sound position and has no playback voices.
	if (controlledByAndIsServer())
	{
		position = soundPositionSeconds;
	}
	else
	{
		if (voice && audioEngine && sound)
		{
			const std::uint32_t rate = audioEngine->clipSampleRate(sound->get());
			position = rate ? static_cast<double>(audioEngine->positionFrames(voice)) / rate : 0.0;
		}
	}

	double soundLength = getSoundLength();
	position = fmod(position, (soundLength ? soundLength : 1));

	if (DFLog::SoundTiming)
	{
		std::stringstream ss;
		ss << reinterpret_cast<const void*>(this) << ", " << getSoundId().toString() << ") = " << position;
		FASTLOGS(DFLog::SoundTiming, "Returning SoundChannel::getSoundPosition(%s", ss.str().c_str());
	}

	return position;
}


void SoundChannel::setSoundPositionLua(double value)
{
	if (isHeardGlobally() && !RBX::Network::Players::serverIsPresent(this))
	{
		if (RBX::Network::Players::clientIsPresent(this))
		{	
			throw std::runtime_error("Sound.TimePosition was set from local script while either in Workspace or SoundService. Only use a server script to set TimePosition when a sound is in these locations.");
			return;
		}
	}

	setSoundPosition(value, true);
}

// Set our sound position, including a requested refresh at the current value.
// position without replicating the value back to everyone else.  See SoundChannel::playSound for usage of this
// latter feature.
void SoundChannel::setSoundPosition(double value, bool setFromLua)
{
	FASTLOG2(DFLog::SoundTrace, "SoundChannel::setSoundPosition(%p, %d)", this, (int)value);
	if (voice && audioEngine && sound)
	{
		double oldPosition = getSoundPosition();
		double soundLength = getSoundLength();
		double newPosition = fmod(value, (soundLength ? soundLength : 1.0));
		if (oldPosition != newPosition)
		{
			FASTLOG1F(FLog::Sound, "Setting sound position on channel to %f", newPosition);
			const std::uint32_t rate = audioEngine->clipSampleRate(sound->get());
			if (rate)
				audioEngine->seekFrames(voice, static_cast<std::uint64_t>(newPosition * rate));
		}
	}

	
	// Only send a property update if our property really changed.  This is a bit of trickery to allow
	// clients to set their positions at "join time" without re-replicating the value
	// back out to others.
	if (value != soundPositionSeconds)
	{
		soundPositionSeconds = value;
		raiseChanged(sound_desc_SoundPosition);

		if (controlledByAndIsServer() && (getSoundLength() >= FInt::MinSecondLengthForLongSoundChannel))
		{
			if (setFromLua)
			{
				event_timePositionUpdatedFromServerScript.fireAndReplicateEvent(this, soundPositionSeconds);
			}
			else if ( lastTimePosReplication.delta().msec() > FInt::MinMsecBetweenTimePosEventReplication )
			{
				// todo: just always fire this event, and only replicated it to clients that are actually listening
				lastTimePosReplication.reset();
				event_timePositionUpdatedFromServer.fireAndReplicateEvent(this, soundPositionSeconds);
			}
		}
	}
}

void SoundChannel::setVolume(float value)
{
	if (!std::isfinite(value))
		throw runtime_error("Sound.Volume must be finite");
	value = G3D::clamp(value, 0.0f, 10.0f);
	if (DFLog::SoundTrace)
	{
		std::stringstream ss;
		ss << reinterpret_cast<void*>(this) << ", " << value << ")";
		FASTLOGS(DFLog::SoundTrace, "SoundChannel::setVolume(%s", ss.str().c_str());
	}

	if (volume != value)
	{
		volume = value;

		if (voice && audioEngine)
		{
			audioEngine->setVoiceVolume(voice, value);
		}

		raiseChanged(sound_desc_Volume);
	}
}

void SoundChannel::setSoundGroup(SoundGroup* value)
{
	if (getSoundGroup() == value)
		return;
	soundGroup = value ? weak_from(value) : weak_ptr<SoundGroup>();
	synchronizeSoundGroupBus();
	raisePropertyChanged(sound_desc_SoundGroup);
}

void SoundChannel::synchronizeSoundGroupBus()
{
	if (!voice || !audioEngine)
	{
		voiceBus = {};
		return;
	}

	SoundService* service = ServiceProvider::find<SoundService>(this);
	const Audio::BusHandle desired = service
		? service->resolveSoundGroupBus(getSoundGroup())
		: Audio::BusHandle{};
	if (voiceBus.index == desired.index &&
		voiceBus.generation == desired.generation)
		return;

	if (audioEngine->setVoiceBus(voice, desired))
		voiceBus = desired;
}

void SoundChannel::synchronizeSoundEffects()
{
	if (!voice || !audioEngine)
		return;
	std::array<Audio::VoiceEffect, 32> effects{};
	SoundService* service = ServiceProvider::find<SoundService>(this);
	const std::uint32_t count = service
		? service->collectRuntimeSoundEffects(this, effects)
		: collectSoundEffects(this, effects);
	audioEngine->setVoiceEffects(voice,
		std::span<const Audio::VoiceEffect>(effects.data(), count));
}

float SoundChannel::getPitch() const
{
	return pitch;
}

void SoundChannel::setPitch(float value)
{
	if (value < 0.0f)
	{
		value = 0.0f;
	}

	if (pitch != value)
	{
		pitch = value;

		if (voice && audioEngine)
		{
			audioEngine->setVoicePitch(voice, pitch);
		}

		raiseChanged(sound_desc_Pitch);
		raiseChanged(sound_desc_PlaybackSpeed);
	}
}

float SoundChannel::getMinDistance() const
{
	return minDistance;
}

void SoundChannel::setMinDistance(float value)
{
	if (value < 0.0f)
	{
		value = 0.0f;
	}

	if (minDistance != value)
	{
		minDistance = value;
		if (voice && audioEngine)
			audioEngine->setVoiceSpatialModel(voice, minDistance, maxDistance, 1.0f, 1.0f,
				rollOff == Linear ? Audio::AttenuationModel::Linear : Audio::AttenuationModel::Inverse);
		raiseChanged(sound_desc_MinDistance);
		raiseChanged(sound_desc_RollOffMinDistance);
	}
}

float SoundChannel::getMaxDistance() const
{
	return maxDistance;
}

void SoundChannel::setMaxDistance(float value)
{
	if (value < 0.0f)
	{
		value = 0.0f;
	}

	if (maxDistance != value)
	{
		maxDistance = value;
		if (voice && audioEngine)
			audioEngine->setVoiceSpatialModel(voice, minDistance, maxDistance, 1.0f, 1.0f,
				rollOff == Linear ? Audio::AttenuationModel::Linear : Audio::AttenuationModel::Inverse);
		raiseChanged(sound_desc_MaxDistance);
		raiseChanged(sound_desc_RollOffMaxDistance);
	}
}

void SoundChannel::setRollOffMode(Soundscape::RollOffMode value)
{
	if (rollOff != value) {
		rollOff = value;
		if (voice && audioEngine)
			audioEngine->setVoiceSpatialModel(voice, minDistance, maxDistance, 1.0f, 1.0f,
				rollOff == Linear ? Audio::AttenuationModel::Linear : Audio::AttenuationModel::Inverse);
		raiseChanged(sound_desc_RollOffMode);
	}
}

bool SoundChannel::getLooped() const
{
	return looped;
}

void SoundChannel::updateLooped()
{
	FASTLOG1(DFLog::SoundTrace, "SoundChannel::updateLooped(%p)", this);
	if (voice && audioEngine)
	{
		audioEngine->setVoiceLooping(voice, getLooped());
	}
}

void SoundChannel::setLooped(bool value)
{
	FASTLOG2(DFLog::SoundTrace, "SoundChannel::setLooped(%p, %d)", this, value);
	if (looped!=value)
	{
		looped = value;
		updateLooped();

		raiseChanged(sound_desc_Looped);
	}
}

void SoundChannel::setPlayCount(int value)
{
	FASTLOG2(DFLog::SoundTrace, "SoundChannel::setPlayCount(%p, %d)", this, value);
	// TODO: This isn't very satisfying.  This won't replicate if you call "play" multiple times
	switch (value)
	{
	case -1:
		if (isHeardGlobally() && RBX::Network::Players::clientIsPresent(this))
		{
			soundPositionSeconds = 0;
		}
		stop();
		break;
	case 0:
		if (isHeardGlobally() && RBX::Network::Players::clientIsPresent(this))
		{
			soundPositionSeconds = getSoundPosition();
		}
		pause();
		break;
	default:
		if (value > reqPlayCount)
		{
			reqPlayCount = playCount = value;
			this->raiseChanged(sound_desc_PlayCount);

			SoundService *soundService = ServiceProvider::create<SoundService>(this);
			if (!soundService)
			{
				--playCount; // In onServiceProvider, we will have another chance to play.
            }
            else
            {
                FASTLOG3(DFLog::SoundTrace, "SoundChannel(%p) soundService = %p, enabled = %d", this, soundService, soundService->enabled());
            }

			loadSound(this, true);
		}

		break;
	}
}


void SoundChannel::update3D()
{
	if (is3D && part && voice && audioEngine)
	{
		SoundService* service = ServiceProvider::find<SoundService>(this);
		const float distanceFactor = service ? std::max(service->getDistanceFactor(), 0.001f) : 1.0f;
		const G3D::Vector3 position = part->getCoordinateFrame().translation;
		const G3D::Vector3 velocity = part->getLinearVelocity();
		audioEngine->setVoiceTransform(voice,
			{position.x / distanceFactor, position.y / distanceFactor, position.z / distanceFactor},
			{velocity.x / distanceFactor, velocity.y / distanceFactor, velocity.z / distanceFactor});
		audioEngine->setVoiceSpatialModel(voice,
			minDistance / distanceFactor, maxDistance / distanceFactor,
			service ? service->getRolloffScale() : 1.0f,
			service ? service->getDopplerScale() : 1.0f,
			rollOff == Linear ? Audio::AttenuationModel::Linear : Audio::AttenuationModel::Inverse);
	}
}

void SoundChannel::soundEnded(weak_ptr<SoundChannel> channelWeak, std::string soundId)
{
	if (shared_ptr<SoundChannel> channel = channelWeak.lock())
	{
		channel->soundEndedSignal(soundId);
	}
}

void SoundChannel::onChannelEnd()
{
	FASTLOG1(DFLog::SoundTrace, "SoundChannel::onChannelEnd(%p)", this);
	if (voice && audioEngine)
	{
		FASTLOG1(FLog::Sound, "Sound ended for channel = %p", this);
		FASTLOGS(FLog::Sound, "Sound ended for soundId = %s", getSoundId().c_str());

		audioEngine->destroyVoice(voice);
		voice = {};
		voiceBus = {};
		audioEngine = NULL;

		if (sound)
		{
			sound->unacquire();
				if (DataModel* dm = RBX::DataModel::get(this)) 
				{
					dm->submitTask(boost::bind(&SoundChannel::soundEnded, shared_from(this), getSoundId().toString()), RBX::DataModelJob::Write);
				}
			}
	}
}

void SoundChannel::playSound(bool isResuming)
{
	FASTLOG1(DFLog::SoundTrace, "SoundChannel::playSound(%p)", this);
	playSound(this, isResuming);
}

bool SoundChannel::isHeardGlobally() const
{
	FASTLOG1(DFLog::SoundTrace, "SoundChannel::isHeardGlobally(%p)", this);

	if (Workspace* workspace = RBX::ServiceProvider::find<Workspace>(this))
	{
		return isDescendantOf(workspace);
	}

	return false;
}

bool SoundChannel::isHeardLocally(const Instance* context) const
{
	FASTLOG2(DFLog::SoundTrace, "SoundChannel::isHeardLocally(%p, %p)", this, context);
	const Network::Player* player = Network::Players::findConstLocalPlayer(context);

	if (context == this)
	{
		context = context->getParent();
	}

	const Instance* parent = context;
	while (parent)
	{
		// Local stock sounds play
		if (Instance::fastDynamicCast<const SoundService>(parent))
			return true;

		// Sounds inside the Workspace may always play
		if (Instance::fastDynamicCast<const Workspace>(parent))
			return true;
		
		// Sounds inside the CoreGui may always play
		if (Instance::fastDynamicCast<const CoreGuiService>(parent))
			return true;
		
		// Sounds inside the local Player may play
		if (parent == player)
			return true;

		parent = parent->getParent();
	}

	
	return false;
}

void SoundChannel::playSound(const Instance* context, bool isResuming)
{
	FASTLOG3(DFLog::SoundTrace, "SoundChannel::playSound(%p, %p, %d)", this, context, static_cast<int>(soundDisabled));
	if (!soundDisabled)
	{
		try
		{
			SoundService* soundService = ServiceProvider::create<SoundService>(context);

			if (!isHeardLocally(context))
			{
				FASTLOG2(DFLog::SoundTrace, "Context %p is not heard locally for %p.", context, this);
				return;
			}

			if (voice && getLooped() && isPlaying())
			{
				update3D();

				FASTLOG1(FLog::Sound, "Playing a looped sound on %p.", this);
				if (isPaused())
					audioEngine->resume(voice);
			}
			else
			{
				// If the service is not reachable yet, then put this off until later
				if (!soundService)
				{
					FASTLOG1(DFLog::SoundTrace, "SoundChannel::playSound(%p) soundService not found", this);
                    return;
				}
				else
				{
					if (!soundService->enabled())
					{
						FASTLOG(FLog::Sound, "Sound service is not enabled.");
						soundDisabled = true;
                        return;
					}
					else
					{
						RBXASSERT(sound);
						if (!sound) // just in case we missed some case during testing
						{
							return;
						}

						if (voice && audioEngine)
						{
							update3D();
							if (isResuming && audioEngine->resume(voice))
							{
								return;
							}
							if (!isResuming)
							{
								audioEngine->stop(voice);
								audioEngine->destroyVoice(voice);
								voice = {};
								voiceBus = {};
								if (sound)
									sound->unacquire();
							}
						}
							
						sound = soundService->loadSound(getSoundId(), is3D);
						const Audio::ClipHandle clip = sound->tryLoad(context);
						if (!clip)
						{
                            FASTLOGS(FLog::Sound, "SoundId %s return no data.", getSoundId().c_str());
							return;
						}

						if (FLog::Sound)
						{
							std::stringstream ss;
							ss << reinterpret_cast<void*>(this) << " playing soundId " << getSoundId().toString() << ", is3D = " << is3D;
							FASTLOGS(FLog::Sound, "SoundChannel %s", ss.str().c_str());
						}

						audioEngine = &soundService->getAudioEngine();
						const float distanceFactor = std::max(soundService->getDistanceFactor(), 0.001f);
						Audio::VoiceParameters parameters;
						parameters.volume = volume;
						parameters.pitch = pitch;
						parameters.looping = getLooped();
						parameters.spatial = is3D;
						parameters.minDistance = minDistance / distanceFactor;
						parameters.maxDistance = maxDistance / distanceFactor;
						parameters.rolloff = soundService->getRolloffScale();
						parameters.dopplerFactor = soundService->getDopplerScale();
						parameters.attenuation = rollOff == Linear
							? Audio::AttenuationModel::Linear
							: Audio::AttenuationModel::Inverse;
						parameters.bus = soundService->resolveSoundGroupBus(
							getSoundGroup());
						parameters.effectCount =
							soundService->collectRuntimeSoundEffects(this,
								parameters.effects);
						if (playbackRegionsEnabled)
						{
							const std::uint32_t rate = audioEngine->clipSampleRate(clip);
							const std::uint64_t frames = audioEngine->clipLengthFrames(clip);
							const auto secondsToFrame = [rate, frames](float seconds) {
								const double nonnegative = std::max(0.0, static_cast<double>(seconds));
								return std::min(frames, static_cast<std::uint64_t>(nonnegative * rate));
							};
							parameters.rangeBeginFrame = secondsToFrame(playbackRegion.min);
							parameters.rangeEndFrame = secondsToFrame(playbackRegion.max);
							parameters.loopBeginFrame = secondsToFrame(loopRegion.min);
							parameters.loopEndFrame = secondsToFrame(loopRegion.max);
						}
						parameters.priority = getSoundLength() >= FInt::MinSecondLengthForLongSoundChannel ? 1 : 0;
						voice = audioEngine->play(clip, parameters);
						if (!voice)
						{
							FASTLOGS(FLog::Sound, "Failed to play soundId = %s", getSoundId().c_str());
							return;
						}
						sound->acquire();
						voiceBus = parameters.bus;
						FASTLOG2(DFLog::SoundTrace, "SoundChannel %p has voice %u", this, voice.index);
						invalidChannel = false;

						updateLooped();

						// make sure our channel is in sync with our sound position data
						setSoundPosition(soundPositionSeconds);

						// make sure sound is in right position for volume
						update3D();
					}
				}
			}
		}
		catch (const std::exception& e)
		{
			// What should we do in case of an error?  Throwing this exception further might be a problem
			StandardOut::singleton()->print(MESSAGE_ERROR, e);
		}
	}
	else
	{
		FASTLOG(DFLog::SoundTrace, "Sound service is currently disabled.");
	}
}

void SoundChannel::playLocal(const Instance *context)
{
	FASTLOG2(DFLog::SoundTrace, "SoundChannel::playLocal(%p, %p)", this, context);
	playSound(context);
}

void SoundChannel::playLocal()
{
	playLocal(this);
}

void SoundChannel::resume()
{
	if (isPaused())
	{
		playSound(true);
		playCount = reqPlayCount = std::max(reqPlayCount + 1, 1);
		if (controlledByAndIsServer())
		{
			event_soundResumedFromServer.fireAndReplicateEvent(this, getSoundPosition());
		}
	}
}

void SoundChannel::play()
{
	FASTLOG1(DFLog::SoundTrace, "SoundChannel::play(%p)", this);
    loadSound(this, true);
	playCount = reqPlayCount = std::max(reqPlayCount + 1, 1);
	this->raiseChanged(sound_desc_PlayCount);
	soundPlayedSignal(getSoundId().toString());
}

void SoundChannel::pauseLocal()
{
	FASTLOG1(DFLog::SoundTrace, "SoundChannel::pauseLocal(%p)", this);
	if (voice && audioEngine)
	{
		if (!audioEngine->isPaused(voice))
		{
			soundPausedSignal(getSoundId().toString());
		}
		audioEngine->pause(voice);
	}
}

void SoundChannel::pause()
{
	FASTLOG1(DFLog::SoundTrace, "SoundChannel::pause(%p)", this);
	pauseLocal();

	if (0 != reqPlayCount)
	{
		playCount = reqPlayCount = 0;
		this->raiseChanged(sound_desc_PlayCount);
	}
}

bool SoundChannel::controlledByAndIsServer() const
{
	return SoundService::soundDisabled && RBX::Network::Players::serverIsPresent(this) && isHeardGlobally();
}

void SoundChannel::stop()
{
	FASTLOG1(DFLog::SoundTrace, "SoundChannel::stop(%p)", this);
	if (voice && audioEngine)
	{
		if (!audioEngine->isPaused(voice))
		{
			soundStoppedSignal(getSoundId().toString());
		}
		audioEngine->stop(voice);
		audioEngine->destroyVoice(voice);
		voice = {};
		voiceBus = {};
		audioEngine = NULL;
		if (sound)
			sound->unacquire();

		numOfTimesLooped = 0;
	}
	
	// make sure sound position gets set to the right spot
	setSoundPosition(0);

	if (-1 != reqPlayCount)
	{
		playCount = reqPlayCount = -1;

		this->raiseChanged(sound_desc_PlayCount);
	}
}

} // namespace Soundscape
} // namespace RBX


// Randomized Locations for hackflags
namespace RBX 
{ 
    namespace Security
    {
        unsigned int hackFlag2 = 0;
    };
};
