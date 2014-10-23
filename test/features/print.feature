Feature: Reading a file

    Scenario: reading tags of an empty file
        Given I have a music file track.mp3
        When  I run tagutil track.mp3
        Then I expect tagutil to succeed
        And   I should see an empty YAML tag list

    Scenario: reading tags of a tagged file
        Given I have a music file track.flac tagged with:
             | title       | Atom Heart Mother |
             | artist      | Pink Floyd        |
             | album       | Atom Heart Mother |
             | tracknumber | 01                |
             | date        | 1970              |
             | genre       | Progressive Rock  |
        When  I run tagutil track.flac
        Then I expect tagutil to succeed
        And   I should see the YAML tag list:
             | title       | Atom Heart Mother |
             | artist      | Pink Floyd        |
             | album       | Atom Heart Mother |
             | tracknumber | 01                |
             | date        | 1970              |
             | genre       | Progressive Rock  |

    Scenario: reading tags of a tagged file in JSON
        Given I have a music file track.flac tagged with:
             | title       | Atom Heart Mother |
             | artist      | Pink Floyd        |
             | album       | Atom Heart Mother |
             | tracknumber | 01                |
             | date        | 1970              |
             | genre       | Progressive Rock  |
        When  I run tagutil -F json track.flac
        Then I expect tagutil to succeed
        And   I should see the JSON tag list:
             | title       | Atom Heart Mother |
             | artist      | Pink Floyd        |
             | album       | Atom Heart Mother |
             | tracknumber | 01                |
             | date        | 1970              |
             | genre       | Progressive Rock  |
