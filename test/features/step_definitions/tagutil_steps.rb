#!/usr/bin/env ruby
# encoding: UTF-8

require 'open3'
require 'json'
require 'yaml'
require 'rspec/expectations'
require 'expect'


Given(/^I have a music file (\w+)\.(mp3|ogg|flac)?$/) do |filename, ext|
    Tagutil.create_tune(filename, ext)
end

Given(/^I have a music file (\w+)\.(mp3|ogg|flac) tagged with:$/) do |filename, ext, tbl|
    Tagutil.create_tune(filename, ext, tags_from_cuke_table(tbl))
end



When(/^I run tagutil(.*)$/) do |params|
    @output, @status = Open3.capture2e("#{Tagutil::Executable} #{params}")
end


Then(/^I expect tagutil to succeed$/) do
    expect(@status).to eq(0)
end

Then(/^I expect tagutil to fail$/) do
    expect(@status).not_to eq(0)
end

Then(/^I should see "(.*?)"$/) do |text|
    expect(@output).to include(text)
end

Then(/^I should see the help about (.+)$/) do |section|
    expect(@output).to match(/^#{section}/m)
end

Then(/^I should see an empty tag list$/) do
    expect(YAML.load(@output)).to be_empty
end

Then(/^I should see:$/) do |tbl|
    expect(YAML.load(@output)).to eql(tags_from_cuke_table(tbl))
end


Before do
    Tagutil.setup
end
