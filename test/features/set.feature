Feature: Setting tags to a file

    Scenario Outline: Setting tags to an empty file
        Given I have a music file <music-file>
        When  I run tagutil set:title=Atom\ Heart\ Mother <music-file>
        And   I run tagutil print <music-file>
        Then  I expect tagutil to succeed
        And   I should see the YAML tag list:
            | title | Atom Heart Mother |
    Examples:
            | music-file |
            | track.flac |
            | track.ogg  |
            | track.mp3  |

    Scenario Outline: chaining multiple set
        Given I have a music file <music-file>
        When  I run tagutil "set:title=Atom Heart Mother" "set:artist=Pink Floyd" <music-file>
        And   I run tagutil print <music-file>
        Then  I expect tagutil to succeed
        And   I should see the YAML tag list:
            | title  | Atom Heart Mother |
            | artist | Pink Floyd        |
    Examples:
            | music-file |
            | track.flac |
            | track.ogg  |
            | track.mp3  |

    Scenario Outline: replacing an existing tag
        Given I have a music file <music-file> tagged with:
            | title       | Atom Heart Mother |
            | artist      | Pink Floyd        |
        When  I run tagutil "set:title=Fearless" <music-file>
        And   I run tagutil print <music-file>
        Then  I expect tagutil to succeed
        And   I should see the YAML tag list:
            | title  | Fearless          |
            | artist | Pink Floyd        |
    Examples:
            | music-file |
            | track.flac |
            | track.ogg  |
            | track.mp3  |

    Scenario Outline: replacing many tags by one set
        Given I have a music file <music-file> tagged with:
            | title  | Echoes         |
            | singer | Richard Wright |
            | artist | Pink Floyd     |
            | singer | David Gilmour  |
        When  I run tagutil set:singer="Richard Wright & David Gilmour" <music-file>
        And   I run tagutil print <music-file>
        Then  I expect tagutil to succeed
        And   I should see the YAML tag list:
            | title  | Echoes                         |
            | singer | Richard Wright & David Gilmour |
            | artist | Pink Floyd                     |
    Examples:
            | music-file |
            | track.flac |
            | track.ogg  |
