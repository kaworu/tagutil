Feature: Loading a file tags

    Scenario Outline: loading tags from a YAML file
        Given there is a music file <music-file>
        And there is a text file named tags.yaml containing:
        """
---
- title: Le missile suit sa lancée
- artist: Keny Arkana
- year: 2006
- album: Entre ciment et belle étoile
- track: 02

        """
        When  I run tagutil load:tags.yaml <music-file>
        And   I run tagutil print <music-file>
        Then  I expect tagutil to succeed
        And   I should see the YAML tag list:
            | title  | Le missile suit sa lancée    |
            | artist | Keny Arkana                  |
            | year   | 2006                         |
            | album  | Entre ciment et belle étoile |
            | track  | 02                           |
    Examples:
            | music-file |
            | track.flac |
            | track.ogg  |
            | track.mp3  |

    Scenario Outline: loading tags from a JSON file
        Given there is a music file <music-file>
        And there is a text file named tags.json containing:
        """
        [
            {"title":  "Le missile suit sa lancée"    },
            {"artist": "Keny Arkana"                  },
            {"year":   "2006"                         },
            {"album":  "Entre ciment et belle étoile" },
            {"track":  "02"                           }
        ]
        """
        When  I run tagutil -F json load:tags.json <music-file>
        And   I run tagutil print <music-file>
        Then  I expect tagutil to succeed
        And   I should see the YAML tag list:
            | title  | Le missile suit sa lancée    |
            | artist | Keny Arkana                  |
            | year   | 2006                         |
            | album  | Entre ciment et belle étoile |
            | track  | 02                           |
    Examples:
            | music-file |
            | track.flac |
            | track.ogg  |
            | track.mp3  |
