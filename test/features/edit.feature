Feature: Editing a file tags

    Scenario Outline: replacing an existing tag
        Given There is a music file <music-file> tagged with:
            | title       | Feeling Good |
            | artist      | Nina Simone  |
        And my favourite editor is evil-edit
        When  I run tagutil edit <music-file>
        And   I run tagutil print <music-file>
        Then  I expect tagutil to succeed
        And   I should see the YAML tag list:
            | title       | Feeling Bad |
            | artist      | Nina Simone |
    Examples:
            | music-file |
            | track.flac |
            | track.ogg  |
            | track.mp3  |
