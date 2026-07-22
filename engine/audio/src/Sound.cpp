/* Copyright 2003-2014 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "audio/Sound.h"
#include "audio/SoundService.h"

#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/GameSettings.h"

#include "StringConv.h"

#include "rbx/Profiler.h"

#include <filesystem>

// This is equivalent to 500 kB by default
DYNAMIC_FASTINTVARIABLE(MinSoundStreamSizeBytes, 512000)

LOGGROUP(Sound)
DYNAMIC_LOGGROUP(SoundTrace)

namespace RBX 
{
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// SoundId Reflection Implementation
	//
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<>
	std::string StringConverter<Soundscape::SoundId>::convertToString(const Soundscape::SoundId& value)
	{
		return value.toString();
	}

	template<>
	bool StringConverter<Soundscape::SoundId>::convertToValue(const std::string& text, Soundscape::SoundId& value)
	{
		value = text;
		return true;
	}

	namespace Reflection {
		template<>
		const Type& Type::getSingleton<RBX::Soundscape::SoundId>()
		{
			return Type::singleton<RBX::ContentId>();
		}

		template<>
		RBX::Soundscape::SoundId& Variant::convert<RBX::Soundscape::SoundId>(void)
		{
			if (_type->isType<std::string>())
			{
				value = RBX::Soundscape::SoundId(cast<std::string>());
				_type = &Type::singleton<RBX::Soundscape::SoundId>();
			}
			return genericConvert<RBX::Soundscape::SoundId>();
		}


		template<>
		void TypedPropertyDescriptor<RBX::Soundscape::SoundId>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
		{
			if (!element->isXsiNil())
			{
				ContentId value;
				if (element->getValue(value))
					setValue(instance, value);
			}
		}

		template<>
		void TypedPropertyDescriptor<RBX::Soundscape::SoundId>::writeValue(const DescribedBase* instance, XmlElement* element) const
		{
			ContentId id(getValue(instance));
			element->setValue(id);
		}

		template<>
		int TypedPropertyDescriptor<RBX::Soundscape::SoundId>::getDataSize(const DescribedBase* instance) const 
		{
			return sizeof(RBX::Soundscape::SoundId) + getValue(instance).toString().size();
		}

		template<>
		bool TypedPropertyDescriptor<RBX::Soundscape::SoundId>::hasStringValue() const {
			return true;
		}

		template<>
		std::string TypedPropertyDescriptor<RBX::Soundscape::SoundId>::getStringValue(const DescribedBase* instance) const{
			return StringConverter<RBX::Soundscape::SoundId>::convertToString(getValue(instance));
		}

		template<>
		bool TypedPropertyDescriptor<RBX::Soundscape::SoundId>::setStringValue(DescribedBase* instance, const std::string& text) const {
			RBX::Soundscape::SoundId value;
			if (StringConverter<RBX::Soundscape::SoundId>::convertToValue(text, value))
			{
				setValue(instance, value);
				return true;
			}
			else
				return false;
		}
	}// namespace Reflection

	namespace Soundscape
    {
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Sound Object Implementation
		//
		///////////////////////////////////////////////////////////////////////////////////////////////////////////

		std::ifstream::pos_type getFilesize(const char* filename)
		{
			std::ifstream is(utf8_decode(filename).c_str(), std::ios_base::in | std::ios_base::binary);
			is.seekg (0, std::ios::end);
			return is.tellg();
		}

		// both ogg and mp3 containers allow mostly arbitrary data before a sync word.
		// People were uploading audio as a model/plugin.  This requires "<roblox" 
		static bool isFileModel(const char* filename)
		{
			std::ifstream is(utf8_decode(filename).c_str(), std::ios_base::in | std::ios_base::binary);
			char buf[7];
			is.read(buf,7);
			return (0 == strncmp(buf, "<roblox", 7));
		}

		static bool isMemModel(const std::string* data)
		{
			if (data->size() > 7)
			{
				return (0 == strncmp(data->c_str(), "<roblox", 7));
			}
			return false;
		}

		Audio::ClipHandle Sound::tryLoad(const RBX::Instance* context)
		{
			FASTLOG1(DFLog::SoundTrace, "Sound::get(%p)", this);

			if (clip)
				return clip;
            
            try
			{
                RBXPROFILER_SCOPE("Sound", "Sound::get");

				std::string fileName;
                
                {
                    RBXPROFILER_SCOPE("Sound", "getFile");

                    fileName = ServiceProvider::create<ContentProvider>(context)->getFile(id);
                }
                
				int fileSize = getFilesize(fileName.c_str());

                RBXPROFILER_LABELF("Sound", "%s (%d bytes)", id.c_str(), fileSize);

				if (!fileName.empty())
				{
					if (isFileModel(fileName.c_str()))
					{
						throw std::runtime_error("sound format invalid");
					}
					if (fileSize <= 0)
						throw std::runtime_error("sound file could not be opened");
					if (fileSize >= DFInt::MinSoundStreamSizeBytes)
					{
						clip = engine->createStreamingClip(std::filesystem::path(utf8_decode(fileName)));
						isStreaming = true;
					}
					else
					{
						std::ifstream input(utf8_decode(fileName).c_str(), std::ios::binary);
						std::vector<std::byte> encoded(static_cast<std::size_t>(fileSize));
						if (!input.read(reinterpret_cast<char*>(encoded.data()), encoded.size()))
							throw std::runtime_error("sound file could not be read completely");
						clip = engine->createEncodedClip(encoded);
					}
				}
				else
				{
					shared_ptr<const std::string> data = ServiceProvider::create<ContentProvider>(context)->requestContentString(id, ContentProvider::PRIORITY_SOUND);

					if (!data)
					{
						return {};
					}

					if (isMemModel(data.get()))
					{
						throw std::runtime_error("sound format invalid");
					}

					const std::span<const std::byte> encoded(
						reinterpret_cast<const std::byte*>(data->data()), data->size());
					clip = engine->createEncodedClip(encoded);
				}
			}
			catch (const std::exception& e)
            {
                StandardOut::singleton()->printf(MESSAGE_ERROR, "Sound failed to load %s : %s because %s", context ? context->getFullName().c_str() : "", id.c_str(), e.what());
            }

			return clip;
		}

		void Sound::release()
		{
			FASTLOG1(DFLog::SoundTrace, "Sound::release(%p)", this);
			if (clip)
            {
				engine->destroyClip(clip);
				clip = {};
			}
		}
	}// namespace Soundscape
}// namespace RBX
