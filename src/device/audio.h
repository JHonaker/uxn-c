#ifndef audio_h
#define audio_h

#include "../common.h"
#include "../uxn.h"

typedef enum {
  AUDIO_DEV_1 = 0x30,
  AUDIO_DEV_2 = 0x40,
  AUDIO_DEV_3 = 0x50,
  AUDIO_DEV_4 = 0x60,
} AudioDevice;

#define AUDIO_VECTOR_PORT 0x0
#define AUDIO_POSITION_PORT 0x2
#define AUDIO_OUTPUT_PORT 0x4
#define AUDIO_ADSR_PORT 0x8
#define AUDIO_LENGTH_PORT 0xa
#define AUDIO_ADDR_PORT 0xc
#define AUDIO_VOLUME_PORT 0xe
#define AUDIO_PITCH_PORT 0xf

#endif // audio_h