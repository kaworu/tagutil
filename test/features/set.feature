Feature: Setting tags to a file

    Scenario: Setting tags to an empty file
        Given I have a music file track.mp3
        When  I run tagutil set:title=Atom\ Heart\ Mother track.mp3
        And   I run tagutil print track.mp3
        Then  I expect tagutil to succeed
        And   I should see the YAML tag list:
            | title | Atom Heart Mother |

    Scenario: chaining multiple set
        Given I have a music file track.flac
        When  I run tagutil "set:title=Atom Heart Mother" "set:artist=Pink Floyd" track.flac
        And   I run tagutil print track.flac
        Then  I expect tagutil to succeed
        And   I should see the YAML tag list:
            | title  | Atom Heart Mother |
            | artist | Pink Floyd        |

    Scenario: replacing an existing tag
        Given I have a music file track.flac tagged with:
            | title       | Atom Heart Mother |
            | artist      | Pink Floyd        |
        When  I run tagutil "set:title=Fearless" track.flac
        And   I run tagutil print track.flac
        Then  I expect tagutil to succeed
        And   I should see the YAML tag list:
            | title  | Fearless          |
            | artist | Pink Floyd        |

    Scenario: replacing many tags by one set
        Given I have a music file track.ogg tagged with:
            | title  | Echoes         |
            | singer | Richard Wright |
            | artist | Pink Floyd     |
            | singer | David Gilmour  |
        When  I run tagutil set:singer="Richard Wright & David Gilmour" track.ogg
        And   I run tagutil print track.ogg
        Then  I expect tagutil to succeed
        And   I should see the YAML tag list:
            | title  | Echoes                         |
            | singer | Richard Wright & David Gilmour |
            | artist | Pink Floyd                     |
