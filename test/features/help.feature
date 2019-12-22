Feature: Displaying help

    Scenario: running tagutil without any arguments
        When I run tagutil
        Then I expect tagutil to fail
        And  I should see "missing file argument"
        And  I should see "Try `tagutil -h' for help"

    Scenario: reading the help
        When I run tagutil -h
        Then I expect tagutil to succeed
        And  I should see "usage: tagutil [OPTION]... [ACTION:ARG]... [FILE]..."
        And  I should see the help about Options
        And  I should see the help about Actions
        And  I should see the help about Formats
        And  I should see the help about Backends
