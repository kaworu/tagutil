/*
 * t_puid.c
 *
 * tagutil PUID generator, using libofa.
 */
#include <ofa1/ofa.h>

#include "t_config.h"
#include "t_toolkit.h"
#include "t_wav.h"
#include "t_puid.h"

char *
puid_create(const struct audio_data *restrict ad)
{
    const char *fprint;
    char *ret;

    assert_not_null(ad);

    fprint = ofa_create_print(ad->samples, OFA_LITTLE_ENDIAN, ad->size,
            ad->srate, ad->stereo ? 1 : 0);
    if (fprint == NULL) {
        (void)fprintf(stdout, "ZUT!\n");
        fflush(stdout);
        ret = NULL;
    }
    else
        ret = xstrdup(fprint);

    return (ret);
}


# if 1
#include <err.h>
int
main(int argc, char *argv[])
{
    int ret;
    struct audio_data ad;
    char *fprint;

    if (argc < 2) {
        (void)fprintf(stderr, "usage: %s wavefile\n", argv[0]);
        return (1);
    }
    ret = wav_load(argv[1], &ad);
    if (ret != 0)
        err(errno, "%s", argv[1]);

    (void)printf("size: %li\n", ad.size);
    (void)printf("srate: %i\n", ad.srate);
    (void)printf("stereo: %i\n", ad.stereo);

    fprint = puid_create(&ad);
    if (fprint == NULL)
        errx(-1, "libofa PASCONTENT");

    (void)printf("fprint: %s\n", fprint);

    free(ad.samples);
    free(fprint);
}
#endif
