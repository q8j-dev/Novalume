/* Copyright 2003-2014 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Service.h"
#include "V8datamodel/DataModel.h"
#include "V8DataModel/Stats.h"
#include "V8DataModel/InteractionEnums.h"
#include "audio/SoundWorld.h"
#include "audio/SoundChannel.h"
#include "Reflection/Event.h"
#include "Util/IHasLocation.h"

#include <array>
#include <cstdint>
#include <vector>

namespace RBX 
{
	class Attachment;
	typedef enum 
	{
		FADE_STATUS_NONE = 0,
		FADE_STATUS_IN,
		FADE_STATUS_OUT
	} FadeStatus;

	namespace Soundscape
	{
		class SoundJob;
		class SoundChannel;
		class AudioPlayer;
		class AudioListener;
		class AudioDeviceOutput;
		class Wire;
		class Sound;
		class SoundId;

		enum ReverbType
		{
			NoReverb = 0,
			GenericReverb,
			PaddedCell,
			Room,
			Bathroom,
			LivingRoom,
			StoneRoom,
			Auditorium,
			ConcertHall,
			Cave,
			Arena,
			Hangar,
			CarpettedHallway,
			Hallway,
			StoneCorridor,
			Alley,
			Forest,
			City,
			Mountains,
			Quarry,
			Plain,
			ParkingLot,
			SewerPipe,
			UnderWater
		};

		enum ListenerType
		{
			CameraListener = 0,
			CFrame,
			ObjectPosition,
			ObjectCFrame
		};

		enum ListenerLocation
		{
			DefaultListenerLocation = 0,
			NoDefaultListener = 1,
			CharacterListener = 2,
			CameraDefaultListener = 3
		};

		struct listenerValues
		{
			CoordinateFrame listenCFrame;
			shared_ptr<IHasLocation> listenObject;
		};

		extern const char* const sSoundService;
	
		class SoundService 
			: public DescribedCreatable<SoundService, Instance, sSoundService, Reflection::ClassDescriptor::PERSISTENT_HIDDEN>
			, public Service
		{
		private:
			typedef DescribedCreatable<SoundService, Instance, sSoundService, Reflection::ClassDescriptor::PERSISTENT_HIDDEN> Super;
			typedef boost::unordered_set<SoundChannel*> SoundChannels;
			typedef boost::unordered_set<AudioPlayer*> AudioPlayers;
			friend class SoundChannel;
			friend class SoundGroup;
			std::unique_ptr<Audio::Engine> audioEngine;
			typedef boost::unordered_map<SoundType, shared_ptr<SoundChannel> > StockSounds;
			StockSounds stockSounds;
			float  dopplerscale;
			float  distancefactor;
			float  rolloffscale;
			ListenerType currentListenerType;
			listenerValues currentListenerValues;
			shared_ptr<Instance> statsItem;
			ReverbType ambientReverb;
			Enums::RolloutState characterSoundsUseNewApi;
			ListenerLocation defaultListenerLocation;
			shared_ptr<AudioListener> automaticListener;
			shared_ptr<AudioDeviceOutput> automaticOutput;
			shared_ptr<Wire> automaticWire;
			shared_ptr<RBX::Attachment> automaticAttachment;
			SoundChannels soundChannels;
			AudioPlayers audioPlayers;
			shared_ptr<SoundJob> soundJob;

			rbx::signals::scoped_connection gameSettingsChangedConnection;

			Time nextGarbageCollectTime;
			typedef boost::unordered_map<SoundId, shared_ptr<Sound> > LoadedSounds;
			LoadedSounds loadedSounds;
			LoadedSounds loaded3DSounds;

			struct SoundGroupBus
			{
				weak_ptr<SoundGroup> group;
				Audio::BusHandle bus;
			};
			std::vector<SoundGroupBus> soundGroupBuses;
			typedef boost::unordered_map<const Instance*,
				std::shared_ptr<Audio::MeterState> > SidechainMeters;
			SidechainMeters sidechainMeters;

			float masterChannelFadeTimeMsec;
			FadeStatus masterChannelFadeStatus;
			std::uint64_t lastOutputDeviceEventSerial;

			bool initialized;
			bool muted;

			void openAudio();
			void closeAudio();
			void garbageCollectSounds();
			static void gcSounds(LoadedSounds& sounds);
			static void getSoundStats(const LoadedSounds& sounds, unsigned int& numSounds, unsigned int& numUnusedSounds);
			void updateSoundChannels(const Time::Interval& timeSinceLastStep);
			void updateAudioPlayers();
			void updateMasterChannelGroup(const Time::Interval& timeSinceLastStep);
			Audio::BusHandle resolveSoundGroupBus(SoundGroup* group);
			void updateSoundGroupVolume(SoundGroup* group);
			void updateSoundGroupBuses();
			void updateSidechainBindings();

			void update3DSettings();
			void on3DSettingChanged(const Reflection::PropertyDescriptor&) { update3DSettings(); }
			void updateAmbientReverb();
			void updateDefaultListener();
			void clearDefaultListener();

		protected:
			///////////////////////////////////////////////////////////////////////////////////////////////
			// Instance Overrides
			//////////////////////////////////////////////////////////////////////////////
			/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		public:
			static bool soundDisabled;		// Suppresses audio for builds that do not play sound (such as web services).
			static bool outputDeviceDisabled; // Keeps the mixer active for headless verification.
			rbx::signal<void(shared_ptr<const Reflection::Tuple>)> deviceListChangedSignal;

			shared_ptr<Sound> loadSound(SoundId id, bool is3D);

			bool enabled() const { return initialized; }
			Audio::Engine& getAudioEngine() { return *audioEngine; }
			std::uint32_t collectRuntimeSoundEffects(const Instance* parent,
				std::array<Audio::VoiceEffect, 32>& effects);
			SoundService();
			~SoundService();
			void playSound(SoundType sound);

			int getSampleRate() const;
			double getMixerTime();

			void getSoundStats(unsigned int& numSounds, unsigned int& numUnusedSounds) const;
			void getChannelsPlaying(int& value) const;
			void muteAllChannels(bool mute);
			bool isMuted();

			void setListener(ListenerType listenerType, shared_ptr<const RBX::Reflection::Tuple> value);
			shared_ptr<const RBX::Reflection::Tuple> getListener();
			CoordinateFrame getListenCFrame(Camera* camera);
			CoordinateFrame getListenerCFrame() const;
			void setListenerCFrame(const CoordinateFrame& value);
			Instance* getListenerObject() const;
			void setListenerObject(Instance* value);
			ListenerType getListenerType() const { return currentListenerType; }
			void setListenerType(ListenerType value);
			void setMasterVolume(float value);
			float getMasterVolume();

			void setMasterVolumeFadeOut(float timeToFadeMsec);
			void setMasterVolumeFadeIn(float timeToFadeMsec);

			void gameSettingsChanged(const Reflection::PropertyDescriptor* propertyDescriptor);

			struct CpuStats
			{
				float total;
				float dsp;
				float stream;
				float geometry;
				float update;
			};
			void getCpuStats(CpuStats& stats) const;

			ReverbType getAmbientReverb() const { return ambientReverb; }
			void setAmbientReverb(const ReverbType& value);
			Enums::RolloutState getCharacterSoundsUseNewApi() const { return characterSoundsUseNewApi; }
			void setCharacterSoundsUseNewApi(Enums::RolloutState value);
			ListenerLocation getDefaultListenerLocation() const { return defaultListenerLocation; }
			void setDefaultListenerLocation(ListenerLocation value);
			float getDopplerScale() const { return dopplerscale; }
			float getDistanceFactor() const { return distancefactor; }
			float getRolloffScale() const { return rolloffscale; }

			static Reflection::BoundProp<float> prop_dopplerscale;
			static Reflection::BoundProp<float> prop_distancefactor;
			static Reflection::BoundProp<float> prop_rolloffscale;
			static Reflection::EnumPropDescriptor<SoundService, ReverbType> prop_AmbientReverb;

			void registerSoundChannel(SoundChannel *soundChannel);
			void unregisterSoundChannel(SoundChannel *soundChannel);
			void registerAudioPlayer(AudioPlayer* audioPlayer);
			void unregisterAudioPlayer(AudioPlayer* audioPlayer);

            void step(const Time::Interval& timeSinceLastStep);
		};


		// Responsible for updating all sound logic
		class SoundJob : public DataModelJob
		{
		private:
			SoundService* const soundService;
			const double fps;
		public:
			SoundJob(SoundService* soundService)
				:DataModelJob("Sound", DataModelJob::Write, false,
				shared_from_dynamic_cast<DataModel>(DataModel::get(soundService)), Time::Interval(0.003))
				,fps(30)
				,soundService(soundService)
			{
				cyclicExecutive = true;
			}

			Time::Interval sleepTime(const Stats& stats)
			{
				return computeStandardSleepTime(stats, fps);
			}

			virtual Job::Error error(const Stats& stats)
			{
				return computeStandardErrorCyclicExecutiveSleeping(stats, fps);
			}

			TaskScheduler::StepResult stepDataModelJob(const Stats& stats)
			{
				soundService->step(stats.timespanSinceLastStep);

				return TaskScheduler::Stepped;
			}
		};

		class SoundServiceStatsItem : public Stats::Item
		{
			const SoundService* service;
			size_t currentalloced;
			size_t maxalloced;
			unsigned int numSounds;
			unsigned int numUnusedSounds;
			int channelsPlaying;
			SoundService::CpuStats cpuStats;
		public:
			SoundServiceStatsItem(const SoundService* service)
				:service(service),currentalloced(0),maxalloced(0)
			{
				setName("Sound");
			}

			static shared_ptr<SoundServiceStatsItem> create(const SoundService* service)
			{
				shared_ptr<SoundServiceStatsItem> result = Creatable<Instance>::create<SoundServiceStatsItem>(service);
				Stats::Item* cpu = result->createBoundPercentChildItem("CPU", result->cpuStats.total);
				cpu->createBoundPercentChildItem("Dsp", result->cpuStats.dsp);
				cpu->createBoundPercentChildItem("Stream", result->cpuStats.stream);
				cpu->createBoundPercentChildItem("Geometry", result->cpuStats.geometry);
				cpu->createBoundPercentChildItem("Update", result->cpuStats.update);
				result->createBoundChildItem("ChannelsPlaying", result->channelsPlaying);
				result->createBoundMemChildItem("Current", result->currentalloced);
				result->createBoundMemChildItem("Max", result->maxalloced);
				result->createBoundChildItem("# Sounds", result->numSounds);
				result->createBoundChildItem("# Unused", result->numUnusedSounds);
				return result;
			}
            
			/*override*/ void update()
            {
				if (service->enabled())
				{
					this->formatValue(service->getSampleRate(), "miniaudio %d Hz", service->getSampleRate());
					currentalloced = 0;
					maxalloced = 0;
					service->getSoundStats(numSounds, numUnusedSounds);
					service->getChannelsPlaying(channelsPlaying);
					service->getCpuStats(cpuStats);
				}
				else
				{
					this->setValue(0, "-disabled-");
				}
			}
		};


	} // namespace Soundscape
} // namespace RBX
