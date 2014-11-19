Feature: Printing the backend used for a file

    Scenario: using the libflac backend
        Given I have a music file track.flac
        When  I run tagutil backend track.flac
        Then  I expect tagutil to succeed
        And   I should see "libFLAC"

    Scenario: using the libvorbis backend
        Given I have a music file track.ogg
        When  I run tagutil backend track.ogg
        Then  I expect tagutil to succeed
        And   I should see "libvorbis"

    Scenario: using the taglib backend
        Given I have a music file track.mp3
        When  I run tagutil backend track.mp3
        Then  I expect tagutil to succeed
        And   I should see "TagLib"
