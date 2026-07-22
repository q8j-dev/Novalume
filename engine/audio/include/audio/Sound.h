/* Copyright 2003-2014 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "v8tree/Instance.h"
#include "reflection/Event.h"
#include "audio/AudioEngine.h"

namespace RBX 
{
	namespace Soundscape
	{
		// A wrapper of contentId we expose to lua as a type
		class SoundId : public ContentId
		{
		public:
			SoundId(const ContentId& id):ContentId(id) {}
			SoundId(const char* id):ContentId(id) {}
			SoundId(const std::string& id):ContentId(id) {}
			SoundId() {}
		};

		// Cached decoded clip shared by DataModel Sound instances.
		class Sound : boost::noncopyable
		{
			Audio::Engine* engine;
			Audio::ClipHandle clip;
			int refCount;
			bool isStreaming;

		public:
			SoundId const id;
			bool const is3D;
			Sound(Audio::Engine& engine, SoundId id, bool is3D)
				: engine(&engine), id(id), is3D(is3D), refCount(0), isStreaming(false) {}
			~Sound() { release(); }
			Audio::ClipHandle get() const { return clip; }
			double getLengthSeconds() const
			{
				const std::uint32_t rate = engine->clipSampleRate(clip);
				return rate ? static_cast<double>(engine->clipLengthFrames(clip)) / rate : 0.0;
			}
			Audio::ClipHandle tryLoad(const RBX::Instance* context);
			void detatch() { clip = {}; }
			void release();

			bool isReferenced() const { return refCount > 0; }
			void acquire() { ++refCount; }
			void unacquire() { refCount = std::max(0, refCount - 1); }

			bool getIsStreaming() const { return isStreaming; }
		};
	} // namespace Soundscape
} // namespace RBX
