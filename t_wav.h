#ifndef T_WAV_H
#define T_WAV_H
/*
 * t_wav.h
 *
 * tagutil WAV handler, based on libofa's example
 */
#include <stdbool.h>

#include "t_config.h"


struct audio_data {
    unsigned char *samples;
    long size;
    int srate;
    bool stereo;
};

int wav_load(const char *path, struct audio_data *ad);

#endif /* not T_WAV_H */
