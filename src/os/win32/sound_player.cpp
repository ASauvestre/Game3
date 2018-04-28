#include <stdio.h>
#include <assert.h>
#include <dsound.h>
#include <math.h>
#include <malloc.h>

#include "sound_player.h"
#include "macros.h"
#include "array.h"
#include "math_m.h"


struct Sound {
    int num_channels;
    double * data;
    int num_samples;

    double length;
    double samples_per_sec;
    double time_offset;

    bool is_playing;
    float num_loops;
};

static IDirectSound8 * ds_interface;
static IDirectSoundBuffer8 * ds_buffer;

static int buffer_length_in_sec = 1;

static Array<Sound*> sounds;

const int playing_frequency = 22050;
const int bytes_per_output_sample = 2;

void win32_init_sound_player(void * handle) {
    // Create the interface
    HRESULT result = DirectSoundCreate8(NULL, &ds_interface, NULL);
    if(!SUCCEEDED(result)) {
        // @Incomplete, Failed to get an interface, should either disable sound playing or try again.
        assert(false);
    }

    ds_interface->SetCooperativeLevel((HWND) handle, DSSCL_PRIORITY);

    // Create wave format
    WAVEFORMATEX wave_format = {};
    wave_format.wFormatTag      = WAVE_FORMAT_PCM;
    wave_format.nChannels       = 1; // @Temporary only one channel for now.
    wave_format.nSamplesPerSec  = playing_frequency; // 22.05kHz seems reasonable
    wave_format.wBitsPerSample  = bytes_per_output_sample * 8;
    wave_format.nBlockAlign     = wave_format.nChannels * wave_format.wBitsPerSample / 8;
    wave_format.nAvgBytesPerSec = wave_format.nBlockAlign * wave_format.nSamplesPerSec;
    wave_format.cbSize          = 0;

    // Create secondary buffer
    DSBUFFERDESC buffer_desc = {};
    buffer_desc.dwSize          = sizeof(DSBUFFERDESC);
    buffer_desc.dwFlags         = DSBCAPS_GETCURRENTPOSITION2;
    buffer_desc.dwBufferBytes   = buffer_length_in_sec * wave_format.nAvgBytesPerSec;
    buffer_desc.dwReserved      = 0;
    buffer_desc.lpwfxFormat     = &wave_format;
    buffer_desc.guid3DAlgorithm = DS3DALG_DEFAULT;

    IDirectSoundBuffer * buffer; // Apparently we need to go through this.

    result = ds_interface->CreateSoundBuffer(&buffer_desc, &buffer, NULL);
    if(!SUCCEEDED(result)) {
        // @Incomplete, failed to get a buffer, should either disable sound playing or try again.
        assert(false);
    }

    result = buffer->QueryInterface(IID_IDirectSoundBuffer8, (void **) &ds_buffer);
    if(!SUCCEEDED(result)) {
        // @Incomplete, failed to get a buffer, should either disable sound playing or try again.
        assert(false);
    }

    ds_buffer->Play(0, 0, DSBPLAY_LOOPING);

}

static const int NUM_SAMPLES_PER_WAVE = 1024;
static unsigned long previous_write_position = 0;
unsigned long previous_write_cursor_position = 0;
void win32_play_sounds(double delta_t) {
    float write_ahead_factor = 3.0f;

    unsigned long write_cursor_position = 0;
    unsigned long play_cursor_position = 0;
    ds_buffer->GetCurrentPosition(&play_cursor_position, &write_cursor_position);

    // Here we compute the position of the write cursor
	int predicted_write_position = previous_write_position + (int) (delta_t * playing_frequency * bytes_per_output_sample + 0.5);

	int write_position = write_cursor_position;

	int buffer_length = playing_frequency * buffer_length_in_sec * bytes_per_output_sample;

	if (predicted_write_position > buffer_length) {
		predicted_write_position = predicted_write_position % (buffer_length);

		if (write_cursor_position > buffer_length * 0.5) { // Wrapped but write cursor is still at the end (second half of buffer)
			write_position = predicted_write_position;
		}
		else {
			write_position = max(predicted_write_position, write_cursor_position);
		}
	} else {
		if (write_cursor_position < previous_write_cursor_position) { // Cursor wrapped, but not predicted
			write_position = write_cursor_position;
		}
		else {
			write_position = max(predicted_write_position, write_cursor_position);
		}
	}

    int samples_to_write = min(buffer_length_in_sec, delta_t * write_ahead_factor) * playing_frequency * bytes_per_output_sample;

    log_print("sound_player", "Play cursor at %d, Write cursor at %d, writing %d samples at %d", play_cursor_position, write_cursor_position, samples_to_write, write_position);

    double * temp_buffer = (double *) calloc(samples_to_write, sizeof(double));
    scope_exit(free(temp_buffer));

    for_array(sounds.data, sounds.count) {
		Sound * sound = *it;
        //log_print("sound_player", "CLEAR");
		for (int offset = 0; offset < samples_to_write; offset++) {
            if(sound->is_playing) {

                int sample_index = (int)(sound->time_offset*sound->samples_per_sec + offset * ((double)sound->samples_per_sec/(playing_frequency) ));

                double sample_index_dec = (sound->time_offset*sound->samples_per_sec + offset * ((double)sound->samples_per_sec/playing_frequency)) - sample_index;

                sample_index = sample_index % sound->num_samples;

                double sample = lerp(sound->data[sample_index], sound->data[(sample_index+1)% sound->num_samples], sample_index_dec);

                //log_print("KK", "%d, %f, %d", sample_index, sample_index_dec, (char)(int)(100*sample));

                temp_buffer[offset] += sample; // @Temporary, Handle channels
            }
        }

        sound->time_offset += delta_t;
        if(sound->num_loops >= 0.0) {
            if(sound->time_offset > sound->length * sound->num_loops) {
                // Sound is done playing, should remove it from the list, but can't right now, so just turning it off @Leak.
                sound->is_playing = false;
            }
        } else {
            if(sound->time_offset > sound->length) {
                sound->time_offset = fmod(sound->time_offset, sound->length);
            }
        }
    }

	short * byte_buffer = (short *) calloc(samples_to_write * bytes_per_output_sample, 1);
    scope_exit(free(byte_buffer));
	for (int i = 0; i < samples_to_write; i++) {
		byte_buffer[i] = (short)(100000*temp_buffer[i]);
        //log_print("sound_player", "Sound output at offset %d is %d", write_position + i, byte_buffer[i]);
	}

    char * write_1_pointer, * write_2_pointer;
    unsigned long write_1_length, write_2_length;

    ds_buffer->Lock(write_position, samples_to_write * bytes_per_output_sample, (void**) &write_1_pointer, &write_1_length, (void**) &write_2_pointer, &write_2_length, 0);

    memcpy((void *) write_1_pointer, (void *) byte_buffer, write_1_length);

	if (write_2_pointer != NULL) {
		memcpy((void *)write_2_pointer, (void *)(byte_buffer + write_1_length), write_2_length);
		int x = 4;
	}

    ds_buffer->Unlock((void*) write_1_pointer, write_1_length, (void*) write_2_pointer, write_2_length);

    previous_write_cursor_position = write_cursor_position;
    previous_write_position = write_position;
}

void win32_play_sound_wave(double wave_frequency, float length) { // @Default length = -1.0f
    Sound * wave = (Sound *) malloc(sizeof(Sound));
    wave->num_channels  = 1;
    wave->num_samples   = NUM_SAMPLES_PER_WAVE;
    wave->is_playing    = true;
    wave->length        = 1.0 / wave_frequency;
    wave->num_loops     = length / wave->length;
    wave->samples_per_sec = wave->num_samples * wave_frequency;
    wave->time_offset   = 0.0;

    wave->data = (double *) malloc(wave->num_samples * sizeof(double));

    for(int i = 0; i < wave->num_samples; i++) {
        wave->data[i] = sin((double) i / wave->num_samples * TAU);
    }

    sounds.add(wave);
}
