/*
 * t_wav.c
 *
 * tagutil WAV handler, based on libofa's example
 */
#include "t_config.h"

#include <errno.h>
#include <fcntl.h> /* open(2) */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "t_toolkit.h"
#include "t_wav.h"


/* write size bytes to buf readed from fd */
_t__nonnull(2)
static inline int readBytes(int fd, unsigned char *restrict buf,
        unsigned int size);


int
wav_load(const char *path, struct audio_data *ad)
{
    int fd;
    unsigned char hdr[36], b[8], *samples;
    long extra_bytes, bytesInNSecs, bytes;
    int compression, channels, srate, bits;

    assert_not_null(path);

    fd = open(path, O_RDONLY);
    if (fd == -1)
        return (-1);

    if (lseek(fd, 0, SEEK_SET) == -1) {
        close(fd);
        return (-1);
    }

    if (readBytes(fd, hdr, 36) != 36) {
        close(fd);
        return (-1);
    }

    if (strncmp((const char *)hdr, "RIFF", 4) != 0 ||
            /* Note: bytes 4 thru 7 contain the file size - 8 bytes */
            strncmp((const char *)&hdr[8], "WAVE", 4) != 0 ||
            strncmp((const char *)&hdr[12], "fmt ", 4) != 0) {
        warnx("bad wave file: %s", path);
        errno = EINVAL;
        close(fd);
        return (-1);
    }

    extra_bytes = hdr[16] + (hdr[17] << 8) + (hdr[18] << 16) + (hdr[19] << 24) - 16;
    compression = hdr[20] + (hdr[21] << 8);
    /* Type 1 is PCM/Uncompressed */
    if (compression != 1) {
        warnx("unsuported compression value: %d", compression);
        errno = EINVAL;
        close(fd);
        return (-1);
    }

    channels = hdr[22] + (hdr[23] << 8);
    /* Only mono or stereo PCM is supported in this example */
    if (channels < 1 || channels > 2) {
        warnx("unsuported number of channels: %d", channels);
        errno = EINVAL;
        close(fd);
        return (-1);
    }

    /* Samples per second, independent of number of channels */
    srate = hdr[24] + (hdr[25] << 8) + (hdr[26] << 16) + (hdr[27] << 24);
    /* Bytes 28-31 contain the "average bytes per second", unneeded here */
    /* Bytes 32-33 contain the number of bytes per sample (includes channels) */
    /* Bytes 34-35 contain the number of bits per single sample */
    bits = hdr[34] + (hdr[35] << 8);
    /* Supporting other sample depths will require conversion */
    if (bits != 16) {
        warnx("unsuported depths : %d", bits);
        errno = EINVAL;
        close(fd);
        return (-1);
    }

    /* Skip past extra bytes, if any */
    if (lseek(fd, 36 + extra_bytes, SEEK_SET) == -1) {
        close(fd);
        return (-1);
    }

    /* Start reading the next frame.  Only supported frame is the data block */
    if (readBytes(fd, b, 8) != 8) {
        close(fd);
        return (-1);
    }

    /* Now look for the data block */
    if (strncmp((const char*)b, "data", 4) != 0) {
        warnx("can't find data in wave file: %s", path);
        errno = EINVAL;
        close(fd);
        return (-1);
    }
    bytes = b[4] + (b[5] << 8) + (b[6] << 16) + (b[7] << 24);

    /* No need to read the whole file, just the first 135 seconds */
    bytesInNSecs = 135 * srate * 2 * channels;
    if (bytes > bytesInNSecs)
        bytes = bytesInNSecs;

    samples = xmalloc(bytes);
    if (readBytes(fd, samples, bytes) != bytes) {
        freex(samples);
        close(fd);
        return (-1);
    }
    close(fd);

    ad->samples = samples;
    ad->size = bytes / 2;
    ad->srate = srate;
    ad->stereo = (channels == 2);

    return (0);
}


/* ~ copy/paste from libofa's example/wavefile.cpp */
static inline int
readBytes(int fd, unsigned char *restrict buf, unsigned int size)
{
    unsigned int ct, x;
    int n;
    unsigned char tmp[BUFSIZ];

    ct = 0;
    while (ct < size) {
        x = size - ct;
        if (x > BUFSIZ)
            x = BUFSIZ;

        n = read(fd, tmp, x);
        if (n <= 0)
            return (ct);

        memcpy(&buf[ct], tmp, n);
        ct += n;
    }

    return (ct);
}
