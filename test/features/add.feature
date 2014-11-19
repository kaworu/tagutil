Feature: Adding tags to a file

    Scenario: Adding tags to an empty file
        Given I have a music file track.flac
        When  I run tagutil add:title=Atom\ Heart\ Mother track.flac
        And   I run tagutil print track.flac
        Then  I expect tagutil to succeed
        And   I should see the YAML tag list:
            | title | Atom Heart Mother |

    Scenario: chaining multiple add
        Given I have a music file track.flac
        When  I run tagutil "add:title=Atom Heart Mother" "add:artist=Pink Floyd" track.flac
        And   I run tagutil print track.flac
        Then  I expect tagutil to succeed
        And   I should see the YAML tag list:
            | title  | Atom Heart Mother |
            | artist | Pink Floyd        |

    Scenario: multiples tags with the same key using libflac
        Given I have a music file track.flac
        When  I run tagutil add:singer="Richard Wright" add:"singer=David Gilmour" track.flac
        And   I run tagutil print track.flac
        Then  I expect tagutil to succeed
        And   I should see the YAML tag list:
            | singer | Richard Wright |
            | singer | David Gilmour  |

    Scenario: multiples tags with the same key using libvorbis
        Given I have a music file track.ogg
        When  I run tagutil add:singer="Richard Wright" add:"singer=David Gilmour" track.ogg
        And   I run tagutil print track.ogg
        Then  I expect tagutil to succeed
        And   I should see the YAML tag list:
            | singer | Richard Wright |
            | singer | David Gilmour  |

    Scenario: multiples tags with the same key using TagLib
        Given I have a music file track.mp3
        When  I run tagutil add:artist="Richard Wright" add:"artist=David Gilmour" track.mp3
        And   I run tagutil print track.mp3
        Then  I expect tagutil to succeed
        And   I should see the YAML tag list:
            | artist | David Gilmour  |
