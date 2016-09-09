Feature: Adding tags to a file

    Scenario Outline: Adding tags to an empty file
        Given There is a music file <music-file>
        When  I run tagutil add:title=Atom\ Heart\ Mother <music-file>
        And   I run tagutil print <music-file>
        Then  I expect tagutil to succeed
        And   I should see the YAML tag list:
            | title | Atom Heart Mother |
    Examples:
            | music-file |
            | track.flac |
            | track.ogg  |
            | track.mp3  |

    Scenario Outline: chaining multiple add
        Given There is a music file <music-file>
        When  I run tagutil "add:title=Atom Heart Mother" "add:artist=Pink Floyd" <music-file>
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

    Scenario Outline: multiples tags with the same key
        Given There is a music file <music-file>
        When  I run tagutil add:singer="Richard Wright" add:"singer=David Gilmour" <music-file>
        And   I run tagutil print <music-file>
        Then  I expect tagutil to succeed
        And   I should see the YAML tag list:
            | singer | Richard Wright |
            | singer | David Gilmour  |
    Examples:
            | music-file |
            | track.flac |
            | track.ogg  |

    Scenario Outline: multiples tags with the same key (using TagLib)
        Given There is a music file <music-file>
        When  I run tagutil add:artist="Richard Wright" add:"artist=David Gilmour" <music-file>
        And   I run tagutil print <music-file>
        Then  I expect tagutil to succeed
        And   I should see the YAML tag list:
            | artist | David Gilmour  |
    Examples:
            | music-file |
            | track.mp3  |
