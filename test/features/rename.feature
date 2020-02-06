Feature: Renaming a file

    Scenario: renaming a file using tags
        Given there is a music file track.flac tagged with:
            | title       | Atom Heart Mother |
            | artist      | Pink Floyd        |
        When  I run tagutil -Y rename:"%artist - %{title}" track.flac
        Then  I expect tagutil to succeed
        And   I expect the file "track.flac" not to exist
        And   I expect the file "Pink Floyd - Atom Heart Mother.flac" to exist

    Scenario: failing to rename a file in a new directory without -p
        Given there is a music file track.flac tagged with:
            | title       | Atom Heart Mother |
            | artist      | Pink Floyd        |
        When  I run tagutil -Y rename:"%artist/%{title}" track.flac
        Then  I expect tagutil to fail
        And   I expect the file "track.flac" to exist
        And   I expect the file "Pink Floyd/Atom Heart Mother.flac" not to exist

    Scenario: successfully renaming a file in a new directory with -p
        Given there is a music file track.flac tagged with:
            | title       | Atom Heart Mother |
            | artist      | Pink Floyd        |
        When  I run tagutil -Y -p rename:"%artist/%{title}" track.flac
        Then  I expect tagutil to succeed
        And   I expect the file "track.flac" not to exist
        And   I expect the file "Pink Floyd/Atom Heart Mother.flac" to exist
