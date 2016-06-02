Feature: Clearing tags from a file

    Scenario Outline: clearing all tags
        Given I have a music file <music-file> tagged with:
            | title       | Atom Heart Mother |
            | artist      | Pink Floyd        |
            | year        | 1970              |
            | album       | Atom Heart Mother |
            | track       | 01                |
            | genre       | Progressive Rock  |
        When  I run tagutil clear: <music-file>
        And   I run tagutil print <music-file>
        Then  I expect tagutil to succeed
        And   I should see an empty YAML tag list
    Examples:
            | music-file |
            | track.flac |
            | track.ogg  |
            | track.mp3  |

    Scenario Outline: clearing a single tag
        Given I have a music file <music-file> tagged with:
            | title  | Echoes         |
            | artist | Pink Floyd     |
        When  I run tagutil clear:title <music-file>
        And   I run tagutil print <music-file>
        Then  I expect tagutil to succeed
        And   I should see the YAML tag list:
            | artist | Pink Floyd     |
    Examples:
            | music-file |
            | track.flac |
            | track.ogg  |
            | track.mp3  |

    Scenario Outline: clearing multiples tags with the same key
        Given I have a music file <music-file> tagged with:
            | title  | Echoes         |
            | singer | Richard Wright |
            | artist | Pink Floyd     |
            | singer | David Gilmour  |
        When  I run tagutil clear:singer <music-file>
        And   I run tagutil print <music-file>
        Then  I expect tagutil to succeed
        And   I should see the YAML tag list:
            | title  | Echoes         |
            | artist | Pink Floyd     |
    Examples:
            | music-file |
            | track.flac |
            | track.ogg  |
