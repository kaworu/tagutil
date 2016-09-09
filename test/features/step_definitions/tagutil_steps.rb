#!/usr/bin/env ruby
# encoding: UTF-8

require 'json'
require 'yaml'
require 'rspec/expectations'
require 'expect'


Given(/^There is a music file (\w+)\.(mp3|ogg|flac)?$/) do |filename, ext|
  Tagutil.create_tune(filename, ext)
end


Given(/^There is a music file (\w+)\.(mp3|ogg|flac) tagged with:$/) do |filename, ext, tbl|
  Tagutil.create_tune(filename, ext, tags_from_cuke_table(tbl))
end

Given(/^my favourite editor is ([^\s]+)$/) do |desc|
  name = desc.strip
  editor = Tagutil::Editor.find(name.strip)
  throw Exception.new("couldn't find the `#{desc}' editor") unless editor
  (@env ||= Hash.new)['EDITOR'] = editor
end

When(/^I run tagutil(.*)$/) do |params|
  env  = @env || Hash.new
  argv = params
  @output, @status = Tagutil.run(env: env, argv: argv)
end


Then(/^I expect tagutil to succeed$/) do
  expect(@status.exitstatus).to eq(0)
end


Then(/^I expect tagutil to fail$/) do
  expect(@status.exitstatus).not_to eq(0)
end


Then(/^I should see "(.*?)"$/) do |text|
  expect(@output).to include(text)
end


Then(/^I should see the help about (.+)$/) do |section|
  expect(@output).to match(/^#{section}/m)
end


Then(/^I should see an empty (YAML|JSON) tag list$/) do |fmt|
  expect(Object.const_get(fmt).load(@output)).to be_empty
end


Then(/^I should see the YAML tag list:$/) do |tbl|
  tags = tags_from_cuke_table(tbl)

  # XXX: the ruby YAML parser will convert any value that look like a number,
  # so we need to do the same kind of trick on the cucumber table (otherwise
  # the test will fail).
  tags.each do |h|
    h.update(h) { |k,v| if v =~ /^\d+$/ then v.to_i else v end }
  end

  expect(YAML.load(@output)).to eql(tags)
end


Then(/^I should see the JSON tag list:$/) do |tbl|
  tags = tags_from_cuke_table(tbl)
  expect(JSON.load(@output)).to eql(tags)
end


Then(/^I debug$/) do
  require "pp"
  pp "ENV:", @env, "STATUS:", @status, "OUTPUT:", @output
end


AfterConfiguration do
  Tagutil.build
end

Before do
  Tagutil.setup
end

After do
  Tagutil.teardown
end
