#ifndef T_PUID_H
#define T_PUID_H
/*
 * t_puid.h
 *
 * tagutil PUID generator, using libofa.
 */
#include "t_config.h"


char * puid_create(const struct audio_data *ad);
#endif /* not T_PUID_H */
