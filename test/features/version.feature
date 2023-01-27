Feature: Displaying version

    Scenario: reading the version
        When I run tagutil -v
        Then I expect tagutil to succeed
        And  I should see "3.1"
