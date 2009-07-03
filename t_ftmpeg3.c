/*
 * t_ftmpeg3.c
 *
 * MPEG layer 3 format handler with ID3Lib.
 */

/*
 * XXX:
 * HACK: id3/globals.h will define bool, true and false ifndef __cplusplus. !!!
 *          so we include "id3.h" before anything, undef false/true and include
 *          <stdbool.h>
 */

/* ID3Lib headers */
#include "id3.h"
#undef true
#undef false

#include <stdbool.h>
#include <string.h>

#include "t_config.h"
#include "t_toolkit.h"
#include "t_file.h"
#include "t_ftmpeg3.h"
#include "t_ftmpeg3_tokenize.h"


struct t_ftmpeg3_data {
    ID3Tag *tag;
};


static const char *t_ftmpeg3_id3_err_str[] = {
  [ID3E_NoError]            = "No error reported",
  [ID3E_NoMemory]           = "No available memory",
  [ID3E_NoData]             = "No data to parse",
  [ID3E_BadData]            = "Improperly formatted data",
  [ID3E_NoBuffer]           = "No buffer to write to",
  [ID3E_SmallBuffer]        = "Buffer is too small",
  [ID3E_InvalidFrameID]     = "Invalid frame id",
  [ID3E_FieldNotFound]      = "Requested field not found",
  [ID3E_UnknownFieldType]   = "Unknown field type",
  [ID3E_TagAlreadyAttached] = "Tag is already attached to a file",
  [ID3E_InvalidTagVersion]  = "Invalid tag version",
  [ID3E_NoFile]             = "No file to parse",
  [ID3E_ReadOnly]           = "Attempting to write to a read-only file",
  [ID3E_zlibError]          = "Error in compression/uncompression",
};


_t__nonnull(1)
void t_ftmpeg3_destroy(struct t_file *restrict file);

_t__nonnull(1)
bool t_ftmpeg3_save(struct t_file *restrict file);

_t__nonnull(1)
struct t_taglist * t_ftmpeg3_get(struct t_file *restrict file,
        const char *restrict key);

_t__nonnull(1)
bool t_ftmpeg3_clear(struct t_file *restrict file,
        const struct t_taglist *restrict T);

_t__nonnull(1) _t__nonnull(2)
bool t_ftmpeg3_add(struct t_file *restrict file,
        const struct t_taglist *restrict T);


void
t_ftmpeg3_destroy(struct t_file *restrict file)
{
    struct t_ftmpeg3_data *data;

    assert_not_null(file);
    assert_not_null(file->data);

    data = file->data;

    ID3Tag_Delete(data->tag);
    t_error_clear(file);
    freex(file);
}


bool
t_ftmpeg3_save(struct t_file *restrict file)
{
    bool ret = true;
    ID3_Err e;
    struct t_ftmpeg3_data *data;

    assert_not_null(file);
    assert_not_null(file->data);
    t_error_clear(file);

    data = file->data;

    e = ID3Tag_Update(data->tag);
    if (e != 0) {
        t_error_set(file, "t_ftmpeg3_save: %s", t_ftmpeg3_id3_err_str[e]);
        ret = false;
    }

    return (ret);
}


struct t_taglist *
t_ftmpeg3_get(struct t_file *restrict file, const char *restrict key)
{
    struct t_taglist *T;
    struct t_ftmpeg3_data *data;

    assert_not_null(file);
    assert_not_null(file->data);
    t_error_clear(file);

    data = file->data;
    T = t_taglist_new();

    /* TODO */
    t_error_set(file, "t_ftmpeg3_get: still need to be implemented.");
    return (NULL);
}


bool
t_ftmpeg3_clear(struct t_file *restrict file, const struct t_taglist *restrict T)
{
    struct t_ftmpeg3_data *data;

    assert_not_null(file);
    assert_not_null(file->data);
    t_error_clear(file);

    data = file->data;

    /* TODO */
    t_error_set(file, "t_ftmpeg3_clear: still need to be implemented.");
    return (false);
}


bool
t_ftmpeg3_add(struct t_file *restrict file, const struct t_taglist *restrict T)
{
    struct t_ftmpeg3_data *data;

    assert_not_null(file);
    assert_not_null(file->data);
    assert_not_null(T);
    t_error_clear(file);

    data = file->data;

    /* TODO */
    t_error_set(file, "t_ftmpeg3_add: still need to be implemented.");
    return (false);
}


void
t_ftmpeg3_init(void)
{
    return;
}


struct t_file *
t_ftmpeg3_new(const char *restrict path)
{
    struct t_file *file;
    struct t_ftmpeg3_data data;
    ID3Tag *tag;

    assert_not_null(path);

    tag = ID3Tag_New();
    if (ID3Tag_Link(tag, path) == 0) {
        ID3Tag_Delete(tag);
        return (NULL);
    }

    data.tag = tag;

    file = t_file_new(path, "ID3Lib", &data, sizeof(data));
    T_FILE_FUNC_INIT(file, mpeg3);

    return (file);
}

#if 0
  /* AENC */ ID3FID_AUDIOCRYPTO,       /**< Audio encryption */
  /* APIC */ ID3FID_PICTURE,           /**< Attached picture */
  /* ASPI */ ID3FID_AUDIOSEEKPOINT,    /**< Audio seek point index */
  /* COMM */ ID3FID_COMMENT,           /**< Comments */
  /* COMR */ ID3FID_COMMERCIAL,        /**< Commercial frame */
  /* ENCR */ ID3FID_CRYPTOREG,         /**< Encryption method registration */
  /* EQU2 */ ID3FID_EQUALIZATION2,     /**< Equalisation (2) */
  /* EQUA */ ID3FID_EQUALIZATION,      /**< Equalization */
  /* ETCO */ ID3FID_EVENTTIMING,       /**< Event timing codes */
  /* GEOB */ ID3FID_GENERALOBJECT,     /**< General encapsulated object */
  /* GRID */ ID3FID_GROUPINGREG,       /**< Group identification registration */
  /* IPLS */ ID3FID_INVOLVEDPEOPLE,    /**< Involved people list */
  /* LINK */ ID3FID_LINKEDINFO,        /**< Linked information */
  /* MCDI */ ID3FID_CDID,              /**< Music CD identifier */
  /* MLLT */ ID3FID_MPEGLOOKUP,        /**< MPEG location lookup table */
  /* OWNE */ ID3FID_OWNERSHIP,         /**< Ownership frame */
  /* PRIV */ ID3FID_PRIVATE,           /**< Private frame */
  /* PCNT */ ID3FID_PLAYCOUNTER,       /**< Play counter */
  /* POPM */ ID3FID_POPULARIMETER,     /**< Popularimeter */
  /* POSS */ ID3FID_POSITIONSYNC,      /**< Position synchronisation frame */
  /* RBUF */ ID3FID_BUFFERSIZE,        /**< Recommended buffer size */
  /* RVA2 */ ID3FID_VOLUMEADJ2,        /**< Relative volume adjustment (2) */
  /* RVAD */ ID3FID_VOLUMEADJ,         /**< Relative volume adjustment */
  /* RVRB */ ID3FID_REVERB,            /**< Reverb */
  /* SEEK */ ID3FID_SEEKFRAME,         /**< Seek frame */
  /* SIGN */ ID3FID_SIGNATURE,         /**< Signature frame */
  /* SYLT */ ID3FID_SYNCEDLYRICS,      /**< Synchronized lyric/text */
  /* SYTC */ ID3FID_SYNCEDTEMPO,       /**< Synchronized tempo codes */
  /* TALB */ ID3FID_ALBUM,             /**< Album/Movie/Show title */
  /* TBPM */ ID3FID_BPM,               /**< BPM (beats per minute) */
  /* TCOM */ ID3FID_COMPOSER,          /**< Composer */
  /* TCON */ ID3FID_CONTENTTYPE,       /**< Content type */
  /* TCOP */ ID3FID_COPYRIGHT,         /**< Copyright message */
  /* TDAT */ ID3FID_DATE,              /**< Date */
  /* TDEN */ ID3FID_ENCODINGTIME,      /**< Encoding time */
  /* TDLY */ ID3FID_PLAYLISTDELAY,     /**< Playlist delay */
  /* TDOR */ ID3FID_ORIGRELEASETIME,   /**< Original release time */
  /* TDRC */ ID3FID_RECORDINGTIME,     /**< Recording time */
  /* TDRL */ ID3FID_RELEASETIME,       /**< Release time */
  /* TDTG */ ID3FID_TAGGINGTIME,       /**< Tagging time */
  /* TIPL */ ID3FID_INVOLVEDPEOPLE2,   /**< Involved people list */
  /* TENC */ ID3FID_ENCODEDBY,         /**< Encoded by */
  /* TEXT */ ID3FID_LYRICIST,          /**< Lyricist/Text writer */
  /* TFLT */ ID3FID_FILETYPE,          /**< File type */
  /* TIME */ ID3FID_TIME,              /**< Time */
  /* TIT1 */ ID3FID_CONTENTGROUP,      /**< Content group description */
  /* TIT2 */ ID3FID_TITLE,             /**< Title/songname/content description */
  /* TIT3 */ ID3FID_SUBTITLE,          /**< Subtitle/Description refinement */
  /* TKEY */ ID3FID_INITIALKEY,        /**< Initial key */
  /* TLAN */ ID3FID_LANGUAGE,          /**< Language(s) */
  /* TLEN */ ID3FID_SONGLEN,           /**< Length */
  /* TMCL */ ID3FID_MUSICIANCREDITLIST,/**< Musician credits list */
  /* TMED */ ID3FID_MEDIATYPE,         /**< Media type */
  /* TMOO */ ID3FID_MOOD,              /**< Mood */
  /* TOAL */ ID3FID_ORIGALBUM,         /**< Original album/movie/show title */
  /* TOFN */ ID3FID_ORIGFILENAME,      /**< Original filename */
  /* TOLY */ ID3FID_ORIGLYRICIST,      /**< Original lyricist(s)/text writer(s) */
  /* TOPE */ ID3FID_ORIGARTIST,        /**< Original artist(s)/performer(s) */
  /* TORY */ ID3FID_ORIGYEAR,          /**< Original release year */
  /* TOWN */ ID3FID_FILEOWNER,         /**< File owner/licensee */
  /* TPE1 */ ID3FID_LEADARTIST,        /**< Lead performer(s)/Soloist(s) */
  /* TPE2 */ ID3FID_BAND,              /**< Band/orchestra/accompaniment */
  /* TPE3 */ ID3FID_CONDUCTOR,         /**< Conductor/performer refinement */
  /* TPE4 */ ID3FID_MIXARTIST,         /**< Interpreted, remixed, or otherwise modified by */
  /* TPOS */ ID3FID_PARTINSET,         /**< Part of a set */
  /* TPRO */ ID3FID_PRODUCEDNOTICE,    /**< Produced notice */
  /* TPUB */ ID3FID_PUBLISHER,         /**< Publisher */
  /* TRCK */ ID3FID_TRACKNUM,          /**< Track number/Position in set */
  /* TRDA */ ID3FID_RECORDINGDATES,    /**< Recording dates */
  /* TRSN */ ID3FID_NETRADIOSTATION,   /**< Internet radio station name */
  /* TRSO */ ID3FID_NETRADIOOWNER,     /**< Internet radio station owner */
  /* TSIZ */ ID3FID_SIZE,              /**< Size */
  /* TSOA */ ID3FID_ALBUMSORTORDER,    /**< Album sort order */
  /* TSOP */ ID3FID_PERFORMERSORTORDER,/**< Performer sort order */
  /* TSOT */ ID3FID_TITLESORTORDER,    /**< Title sort order */
  /* TSRC */ ID3FID_ISRC,              /**< ISRC (international standard recording code) */
  /* TSSE */ ID3FID_ENCODERSETTINGS,   /**< Software/Hardware and settings used for encoding */
  /* TSST */ ID3FID_SETSUBTITLE,       /**< Set subtitle */
  /* TXXX */ ID3FID_USERTEXT,          /**< User defined text information */
  /* TYER */ ID3FID_YEAR,              /**< Year */
  /* UFID */ ID3FID_UNIQUEFILEID,      /**< Unique file identifier */
  /* USER */ ID3FID_TERMSOFUSE,        /**< Terms of use */
  /* USLT */ ID3FID_UNSYNCEDLYRICS,    /**< Unsynchronized lyric/text transcription */
  /* WCOM */ ID3FID_WWWCOMMERCIALINFO, /**< Commercial information */
  /* WCOP */ ID3FID_WWWCOPYRIGHT,      /**< Copyright/Legal infromation */
  /* WOAF */ ID3FID_WWWAUDIOFILE,      /**< Official audio file webpage */
  /* WOAR */ ID3FID_WWWARTIST,         /**< Official artist/performer webpage */
  /* WOAS */ ID3FID_WWWAUDIOSOURCE,    /**< Official audio source webpage */
  /* WORS */ ID3FID_WWWRADIOPAGE,      /**< Official internet radio station homepage */
  /* WPAY */ ID3FID_WWWPAYMENT,        /**< Payment */
  /* WPUB */ ID3FID_WWWPUBLISHER,      /**< Official publisher webpage */
  /* WXXX */ ID3FID_WWWUSER,           /**< User defined URL link */
#endif
