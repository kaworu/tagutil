Feature: Reading a file

    Scenario Outline: reading tags of an empty file
        Given there is a music file <music-file>
        When  I run tagutil <music-file>
        Then  I expect tagutil to succeed
        And   I should see an empty YAML tag list
    Examples:
            | music-file |
            | track.flac |
            | track.ogg  |
            | track.mp3  |

    Scenario Outline: reading tags of a tagged file in YAML
        Given there is a music file <music-file> tagged with:
            | title       | Atom Heart Mother |
            | artist      | Pink Floyd        |
            | year        | 1970              |
            | album       | Atom Heart Mother |
            | track       | 01                |
            | genre       | Progressive Rock  |
        When  I run tagutil <music-file>
        Then  I expect tagutil to succeed
        And   I should see the YAML tag list:
            | title       | Atom Heart Mother |
            | artist      | Pink Floyd        |
            | year        | 1970              |
            | album       | Atom Heart Mother |
            | track       | 01                |
            | genre       | Progressive Rock  |
    Examples:
            | music-file |
            | track.flac |
            | track.ogg  |
            | track.mp3  |

    Scenario Outline: reading tags of a tagged file in JSON
        Given there is a music file <music-file> tagged with:
            | title       | Atom Heart Mother |
            | artist      | Pink Floyd        |
            | year        | 1970              |
            | album       | Atom Heart Mother |
            | track       | 01                |
            | genre       | Progressive Rock  |
        When  I run tagutil -F json <music-file>
        Then  I expect tagutil to succeed
        And   I should see the JSON tag list:
            | title       | Atom Heart Mother |
            | artist      | Pink Floyd        |
            | year        | 1970              |
            | album       | Atom Heart Mother |
            | track       | 01                |
            | genre       | Progressive Rock  |
    Examples:
            | music-file |
            | track.flac |
            | track.ogg  |
            | track.mp3  |
