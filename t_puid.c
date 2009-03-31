/*
 * t_puid.c
 *
 * tagutil PUID generator, using libofa.
 */
#include "t_config.h"

#include <ofa.h>

#include "t_toolkit.h"
#include "t_wav.h"
#include "t_puid.h"

char *
puid_create(const struct audio_data *restrict ad, )
{

    assert_not_null(ad);

	const char *print = ofa_create_print(ad->samples,
            byteOrder, size, sRate, stereo);
