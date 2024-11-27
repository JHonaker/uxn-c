#include "audio.h"
#include <stdlib.h>

/**
 * The Audio/vector* is evaluated when a note ends. All samples used by the
 * audio devices are unsigned 8-bit mono.
 *
 * When a byte is written to the Audio/pitch port, any sample that is currently
 * playing will be replaced with the sample defined by all the values set in the
 * device. It starts playing an audio sample from memory located at Audio/addr*
 * with length of Audio/length*. It loops until it reaches the end of the
 * envelope defined by Audio/adsr*.
 *
 * Each of the ADSR components is measured in 15ths of a second, so writing
 * #ffff to Audio/adsr* will play a note that lasts for exactly four seconds,
 * with each section of the envelope lasting one second. If Audio/adsr* is
 * #0000, no envelope will be applied. This is most useful for longer samples
 * that are set to play once by setting the most significant bit of Audio/pitch
 * to 1.
 *
 * The evelope varies the amplitude as follows: starting at 0%, rising to 100%
 * over the Attack section, falling to 50% over the Decay section, remaining at
 * 50% throughout the Sustain section, and finally, falling to 0% over the
 * Release section. The envelope is linearly interpolated throughout each
 * section.
 *
 * The two Audio/volume components balance how loudly the next sample will play
 * in each ear. #ff sets maximum volume for both speakers. While the sample is
 * playing, the Audio/output byte can read to find the loudness of the envelope
 * at that moment.
 */

typedef struct Envelope {
  Byte attack;
  Byte decay;
  Byte sustain;
  Byte release;
  bool disabled;
} Envelope;

typedef struct Pitch {
  bool loop;
  Short note;
} Pitch;

typedef struct Volume {
  Byte left;
  Byte right;
} Volume;

typedef struct Sample {
  Short length;
  Byte buffer[]; ///< A buffer of the specified length
} Sample;

// Uxn information extractors

Envelope read_adsr(Uxn *uxn, AudioDevice dev) {
  Short adsr = uxn_dev_read_short(uxn, dev | AUDIO_ADSR_PORT);

  Envelope env = {.attack = adsr >> 12,
                  .decay = (adsr >> 8) & 0xf,
                  .sustain = (adsr >> 4) & 0xf,
                  .release = adsr & 0xf,
                  .disabled = true};

  if (env.attack || env.decay || env.sustain || env.release)
    env.disabled = false;

  return env;
}

Pitch read_pitch(Uxn *uxn, AudioDevice dev) {
  Byte pitch_byte = uxn_dev_read(uxn, dev | AUDIO_PITCH_PORT);

  Pitch pitch = {
      .loop = pitch_byte >> 7,
      .note = pitch_byte & 0x7f,
  };

  return pitch;
}

Volume read_volume(Uxn *uxn, AudioDevice dev) {
  Short volume_byte = uxn_dev_read_short(uxn, dev | AUDIO_VOLUME_PORT);

  Volume vol = {
      .left = volume_byte >> 4,
      .right = volume_byte & 0xf,
  };

  return vol;
}

Sample *read_sample(Uxn *uxn, AudioDevice dev) {
  Short len = uxn_dev_read_short(uxn, dev | AUDIO_LENGTH_PORT);
  Byte addr = uxn_dev_read_short(uxn, dev | AUDIO_ADDR_PORT);

  Sample *s = malloc(sizeof(Sample) + len * sizeof(Byte));

  s->length = len;
  for (size_t i = 0; i < len; i++) {
    s->buffer[i] = uxn_dev_read(uxn, addr + i);
  }

  return s;
}

// Envelope

static inline double lerp(double from, double to, double t) {
  return (1 - t) * from + t * to;
}

double adsr(Envelope env, Short t) {
  if (env.disabled)
    return 1.0;

  if (t <= env.attack) {
    const double r = t / (double)env.attack;
    return lerp(0.0, 1.0, r);
  } else if (t <= env.decay) {
    const double r = (t - env.attack) / (double)env.decay;
    return lerp(1.0, 0.5, r);
  } else if (t <= env.sustain) {
    return 0.5;
  } else if (t <= env.release) {
    const double r =
        (t - (env.attack + env.decay + env.sustain)) / (double)env.release;
    return lerp(0.5, 0.0, r);
  } else {
    return 0.0;
  }
}
