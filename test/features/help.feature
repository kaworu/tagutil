Feature: Displaying help

    Scenario: running tagutil without any arguments
        When I run tagutil
        Then I expect tagutil to fail
        Then I should see "missing file argument"
        And  I should see "Try `tagutil -h' for help"
