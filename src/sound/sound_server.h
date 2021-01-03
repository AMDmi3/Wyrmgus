//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
/**@name sound_server.h - The sound server header file. */
//
//      (c) Copyright 1998-2020 by Lutz Sammer, Fabrice Rossi,
//                                 Jimmy Salmon and Andrettin
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//

#pragma once

#include "sound/sound.h"

constexpr int MaxVolume = 255;
constexpr int SOUND_BUFFER_SIZE = 65536;

namespace wyrmgus {

enum class unit_sound_type;

/**
**  RAW samples.
*/
class sample final
{
public:
	static constexpr int channel_count = 2;
	static constexpr int sample_size = 16;
	static constexpr int frequency = 44100;

	explicit sample(const std::filesystem::path &filepath) : filepath(filepath)
	{
		if (!std::filesystem::exists(filepath)) {
			throw std::runtime_error("Sound file \"" + filepath.string() + "\" does not exist.");
		}
	}

	~sample()
	{
		Mix_FreeChunk(this->chunk);
	}

	bool is_loaded() const
	{
		return this->chunk != nullptr;
	}

	void load()
	{
		this->chunk = Mix_LoadWAV(this->filepath.string().c_str());
		if (this->chunk == nullptr) {
			throw std::runtime_error("Failed to decode audio file \"" + this->filepath.string() + "\": " + std::string(SDL_GetError()));
		}
	}

	virtual int Read(void *buf, int len)
	{
		Q_UNUSED(buf)
		Q_UNUSED(len)

		return 0;
	}

	const uint8_t *get_buffer() const
	{
		return this->chunk->abuf;
	}

	int get_length() const
	{
		return static_cast<int>(this->chunk->alen);
	}

	int get_channel_count() const
	{
		return sample::channel_count;
	}

	int get_sample_size() const
	{
		return sample::sample_size;
	}

	int get_frequency() const
	{
		return sample::frequency;
	}

	Mix_Chunk *get_chunk() const
	{
		return this->chunk;
	}

private:
	std::filesystem::path filepath;
	Mix_Chunk *chunk = nullptr; //sample buffer
};

}

/// Set the channel volume
extern int SetChannelVolume(int channel, int volume);
/// Set the channel stereo
extern int SetChannelStereo(int channel, int stereo);
//Wyrmgus start
/// Set the channel voice group
extern void SetChannelVoiceGroup(int channel, const wyrmgus::unit_sound_type unit_sound_type);
//Wyrmgus end
/// Set the channel's callback for when a sound finishes playing
extern void SetChannelFinishedCallback(int channel, void (*callback)(int channel));
/// Stop a channel
extern void StopChannel(int channel);
/// Stop all channels
extern void StopAllChannels();

/// Check if this unit plays some sound
extern bool UnitSoundIsPlaying(Origin *origin);
/// Check, if this sample is already playing
extern bool SampleIsPlaying(const wyrmgus::sample *sample);
/// Load a sample
extern std::unique_ptr<wyrmgus::sample> LoadSample(const std::filesystem::path &filepath);
/// Play a sample
extern int PlaySample(wyrmgus::sample *sample, Origin *origin = nullptr);

/// Set effects volume
extern void SetEffectsVolume(int volume);
/// Get effects volume
extern int GetEffectsVolume();
/// Set effects enabled
extern void SetEffectsEnabled(bool enabled);
/// Check if effects are enabled
extern bool IsEffectsEnabled();

/// Set the music finished callback
void SetMusicFinishedCallback(void (*callback)());
/// Play a music file
extern int PlayMusic(std::unique_ptr<wyrmgus::sample> &&sample);
/// Play a music file
extern int PlayMusic(const std::string &file);
/// Play a music track
extern void PlayMusicName(const std::string &name);
/// Play a music track
extern void PlayMusicByGroupRandom(const std::string &group);
/// Play a music track
extern void PlayMusicByGroupAndSubgroupRandom(const std::string &group, const std::string &subgroup);
/// Play a music track
extern void PlayMusicByGroupAndFactionRandom(const std::string &group, const std::string &civilization_name, const std::string &faction_name);
/// Set a condition for music
extern void SetMusicCondition(int id, int value);
/// Increase tension value for the music
extern void AddMusicTension(int value);
/// Set gain of a music layer
extern void SetMusicLayerGain(const std::string &layer, float gain);
/// Stop music playing
extern void StopMusic();
/// Set music volume
extern void SetMusicVolume(int volume);
/// Get music volume
extern int GetMusicVolume();
/// Set music enabled
extern void SetMusicEnabled(bool enabled);
/// Check if music is enabled
extern bool IsMusicEnabled();
/// Check if music is playing
extern bool IsMusicPlaying();

/// Check if sound is enabled
extern bool SoundEnabled();
/// Initialize the sound card.
extern int InitSound();
///  Cleanup sound.
extern void QuitSound();
