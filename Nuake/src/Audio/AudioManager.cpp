#include "AudioManager.h"
#include "src/Core/Logger.h"
#include "src/Core/FileSystem.h"

#include <soloud.h>
#include "soloud_speech.h"
#include "soloud_thread.h"
#include <dependencies/soloud/include/soloud_wav.h>


namespace Nuake {

	AudioManager::AudioManager() : 
		m_AudioThreadRunning(true)
	{
	}

	AudioManager::~AudioManager()
	{
		Deinitialize();

		m_AudioThreadRunning = false;
		m_AudioThread.join();
	}

	void AudioManager::Initialize()
	{
		m_Soloud = CreateRef<SoLoud::Soloud>();

		// TODO: Sample rate, back end, buffer size, flags.
		m_Soloud->init();

		m_AudioThread = std::thread(&AudioManager::AudioThreadLoop, this);

		Logger::Log("Audio manager initialized", "audio", VERBOSE);
	}

	void AudioManager::Deinitialize()
	{
		m_WavSamples.clear();
		m_ActiveClips.clear();
		m_Soloud->deinit();
	}

	void AudioManager::SetListenerPosition(const Vector3& position, const Vector3& direction, const Vector3& up)
	{
		if (position != m_ListenerPosition || direction != m_ListenerDirection || up != m_ListenerUp)
		{
			m_ListenerPosition = position;
			m_ListenerDirection = direction;
			m_ListenerUp = up;

			m_Soloud->set3dListenerParameters(m_ListenerPosition.x, m_ListenerPosition.y, m_ListenerPosition.z,
					m_ListenerDirection.x, m_ListenerDirection.y, m_ListenerDirection.z,
					m_ListenerUp.x, m_ListenerUp.y, m_ListenerUp.z
			);

			m_Soloud->update3dAudio();
		}
	}

	void AudioManager::PlayTTS(const std::string& text)
	{

	}

	void AudioManager::QueueWavAudio(const AudioRequest& request)
	{
		// Acquire mutex lock and push to queue
		const std::lock_guard<std::mutex> lock(m_AudioQueueMutex);

		// Check if file exists and load
		const bool fileExists = FileSystem::FileExists(request.audioFile, true);
		if (fileExists && !IsWavLoaded(request.audioFile))
		{
			LoadWavAudio(request.audioFile);
		}

		m_AudioQueue.push(request);
	}

	void AudioManager::UpdateVoice(const AudioRequest& request)
	{
		// Acquire mutex lock
		const std::lock_guard<std::mutex> lock(m_AudioQueueMutex);

		if (IsVoiceActive(request.audioFile))
		{
			if (!m_Soloud->isValidVoiceHandle(m_ActiveClips[request.audioFile]))
			{
				m_ActiveClips.erase(request.audioFile);
				return;
			}
		}

		SoLoud::handle& voice = m_ActiveClips[request.audioFile];

		if (request.spatialized)
		{
			m_Soloud->set3dSourcePosition(voice, request.position.x, request.position.y, request.position.z);
			m_Soloud->set3dSourceMinMaxDistance(voice, request.MinDistance, request.MaxDistance);
			m_Soloud->set3dSourceAttenuation(voice, 1, request.AttenuationFactor);
			m_Soloud->update3dAudio();
		}
		else
		{
			m_Soloud->setPan(voice, request.pan);
		}
		m_Soloud->setLooping(voice, request.Loop);
		m_Soloud->setRelativePlaySpeed(voice, request.speed);
	}

	void AudioManager::StopVoice(const std::string& filePath)
	{
		const std::lock_guard<std::mutex> lock(m_AudioQueueMutex);

		if (!IsVoiceActive(filePath))
		{
			// We can't stop a voice that isn't active.
			return;
		}

		SoLoud::handle& voice = m_ActiveClips[filePath];
		m_Soloud->stop(voice);
	}

	void AudioManager::StopAll() const
	{
		m_Soloud->stopAll();
	}

	bool AudioManager::IsWavLoaded(const std::string& filePath) const
	{
		return m_WavSamples.find(filePath) != m_WavSamples.end();
	}

	bool AudioManager::IsVoiceActive(const std::string& voice) const
	{
		return m_ActiveClips.find(voice) != m_ActiveClips.end();
	}

	void AudioManager::LoadWavAudio(const std::string& filePath)
	{
		m_WavSamples[filePath] = SoLoud::Wav();
		m_WavSamples[filePath].load(filePath.c_str());
	}

	void AudioManager::AudioThreadLoop()
	{
		while(m_AudioThreadRunning)
		{
			// Acquire mutex lock
			const std::lock_guard<std::mutex> lock(m_AudioQueueMutex);

			// Check if we have audio queued
			while (!m_AudioQueue.empty())
			{
				AudioRequest& audioRequest = m_AudioQueue.front();
				SoLoud::Wav& audio = m_WavSamples[audioRequest.audioFile];

				SoLoud::handle soloudHandle;
				if (!audioRequest.spatialized)
				{
					soloudHandle = m_Soloud->play(audio);
					m_Soloud->setVolume(soloudHandle, audioRequest.volume);
					m_Soloud->setPan(soloudHandle, audioRequest.pan);
				}
				else
				{
					const Vector3& position = audioRequest.position;
					soloudHandle = m_Soloud->play3d(audio, position.x, position.y, position.z);
					m_Soloud->set3dSourceMinMaxDistance(soloudHandle, audioRequest.MinDistance, audioRequest.MaxDistance);
					m_Soloud->set3dSourceAttenuation(soloudHandle, SoLoud::AudioSource::ATTENUATION_MODELS::EXPONENTIAL_DISTANCE, audioRequest.AttenuationFactor);
				}
				
				m_Soloud->setRelativePlaySpeed(soloudHandle, audioRequest.speed);
				m_Soloud->setLooping(soloudHandle, audioRequest.Loop);

				m_ActiveClips[audioRequest.audioFile] = soloudHandle;

				// Remove item from queue.
				m_AudioQueue.pop();
			}
		}
	}

	void AudioManager::CleanupInactiveVoices()
	{
		if (m_ActiveClips.empty())
		{
			return;
		}

		std::erase_if(m_ActiveClips, [this](const auto& item)
		{
			return !m_Soloud->isValidVoiceHandle(item.second);
		});
	}
}