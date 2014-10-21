#!/usr/bin/env ruby
# encoding: UTF-8

require 'open3'
require 'rspec/expectations'
require 'expect'


When(/^I run tagutil(.*)$/) do |params|
    @output, @status = Open3.capture2e("#{Tagutil::Executable} #{params}")
end

Then(/^I should see "(.*?)"$/) do |text|
    expect(@output).to include(text)
end

Then(/^I expect tagutil to succeed$/) do
    expect(@status).to eq(0)
end

Then(/^I expect tagutil to fail$/) do
    expect(@status).not_to eq(0)
end
