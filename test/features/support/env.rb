#!/usr/bin/env ruby
# encoding: UTF-8

require 'fileutils'
require 'open3'
require 'tmpdir'
require 'yaml'


module Tagutil
    ProjectRoot = File.join(File.dirname(__FILE__), '..', '..', '..')
    Executable  = File.join(ProjectRoot, 'build', 'tagutil')
    @tmpdir     = nil # temporary test directory
    @blankfiles = []  # music test files
    @tmpfiles   = []  # generated files

    # build tagutil iff the executable does not exist
    def self.build
        self.force_build unless File.exist?(Executable)
    end

    # build tagutil
    def self.force_build
        STDOUT.print "===> running a debug build..."
        STDOUT.flush
        output, status = Open3.capture2e("make -C #{ProjectRoot}")
        raise RuntimeException.new('Could not build tagutil') unless status.success?
        STDOUT.puts "done."
        STDOUT.flush
    end

    # called by a Before hook
    def self.setup
        self.build
        @tmpdir ||= Dir.mktmpdir('tagutil-cucumber-')
        Dir.chdir @tmpdir
        @blankfiles = Dir[File.join(ProjectRoot, 'test', 'files', '*')] if @blankfiles.empty?
    end

    def self.teardown
        while (victim = @tmpfiles.pop) do
            FileUtils.rm(victim, force: true)
        end
    end

    # create a music file by copying the blank file matching the requested
    # extension.
    def self.create_tune(filename, ext, tags=nil)
        tune  = "#{filename}.#{ext}"
        blank = @blankfiles.select { |f| f =~ /\.#{ext}$/ }.first
        raise ArgumentError.new "#{ext}: bad file extension" unless blank
        FileUtils.cp blank, tune
        if tags
            data = "#{tags.to_yaml}\n"
            output, status = Open3.capture2e("#{Executable} load:- #{tune}", stdin_data: data)
            raise RuntimeError.new(output) unless status.success? and output.empty?
        end
        @tmpfiles << tune
    end

    def self.run(env: {}, argv: "")
      cmd = "#{Executable} #{argv}"
      Open3.capture2e(env, cmd)
    end

    class World
        def tags_from_cuke_table tbl
            tbl.raw.map do |x|
                {x.first => x.last}
            end
        end
    end
end


World do
    Tagutil::World.new
end
