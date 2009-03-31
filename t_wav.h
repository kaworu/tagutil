#ifndef T_WAV_H
#define T_WAV_H
/*
 * t_wav.h
 *
 * tagutil WAV handler, based on libofa's example
 */
#include "t_config.h"
#include <stdbool.h>

struct audio_data {
	unsigned char *samples;
	long size;
	int srate, ms;
	bool stereo;
};

__t__nonnull(1) __t__nonnull(2)
int wav_load(const char *path, struct audio_data *ad);

#endif /* not T_WAV_H */
