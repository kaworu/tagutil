#!/usr/bin/env ruby
# encoding: UTF-8

require 'tmpdir'
require 'fileutils'

module Tagutil
    ProjectRoot = File.join(File.dirname(__FILE__), '..', '..', '..')
    Executable  = File.join(ProjectRoot, 'build', 'tagutil')
    @tmpdir     = nil # temporary test directory
    @blankfiles = []  # music test files

    # build tagutil iff the executable does not exist
    def self.build
        self.force_build unless File.exist?(Executable)
    end

    # build tagutil
    def self.force_build
        system "make -C #{ProjectRoot}"
    end

    # called by a Before hook
    def self.setup
        self.build
        @tmpdir ||= Dir.mktmpdir('tagutil-cucumber-')
        Dir.chdir @tmpdir
        @blankfiles = Dir[File.join(ProjectRoot, 'test', 'files', '*')] if @blankfiles.empty?
    end

    # create a music file by copying the blank file matching the requested
    # extension.
    def self.create_tune filename, ext
        target = @blankfiles.select { |f| f =~ /\.#{ext}$/ }.first
        raise ArgumentError.new "#{ext}: bad file extension" unless target
        FileUtils.cp target, "#{filename}.#{ext}"
    end
end
