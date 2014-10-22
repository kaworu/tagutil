Feature: Reading a file

    Scenario: reading tags of an empty file
        Given I have a music file track.mp3
        When  I run tagutil track.mp3
        Then  I should see an empty tag list
