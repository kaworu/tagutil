#!/usr/bin/env ruby
# small script that simulate an editor for testing the tagutil edit action.
#
# substitute all instance of 'Good' in 'Bad' in the tags values.

require 'yaml'

victim = ARGV.first
data = YAML.load(File.read(victim))

data.each do |hash|
  hash.update(hash) do |key, value|
    value.gsub('Good', 'Bad') # MOUHAHAHHAHAA!
  end
end

File.write(victim, data.to_yaml)
