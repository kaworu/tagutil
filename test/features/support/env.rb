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
  @blankfiles = Dir[File.join(ProjectRoot, 'test', 'files', '*')]

  # build tagutil iff the executable does not exist
  def self.build
    self.build! unless File.exist?(Executable)
  end

  # build tagutil
  def self.build!
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
    @tmpdir = Dir.mktmpdir('tagutil-cucumber-')
    Dir.chdir @tmpdir
  end

  def self.teardown
    Dir.chdir '/'
    FileUtils.rm_rf(@tmpdir)
    @tmpdir = nil
  end

  # create a music file by copying the blank file matching the requested
  # extension.
  def self.create_tune(filename, ext, tags=nil)
    tune  = "#{filename}.#{ext}"
    path  = File.join(@tmpdir, tune)
    blank = @blankfiles.select { |f| f =~ /\.#{ext}$/ }.first
    raise ArgumentError.new "#{ext}: bad file extension" unless blank
    FileUtils.cp blank, path
    if tags
      data = "#{tags.to_yaml}\n"
      output, status = Open3.capture2e("#{Executable} load:- #{path}", stdin_data: data)
      raise RuntimeError.new(output) unless status.success? and output.empty?
    end
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

  # fake editors used to test the edit action
  module Editor
    EVIL = File.join(ProjectRoot, 'test', 'scripts', 'evil-editor.rb')

    # helper to find an editor path by keyword
    def self.find key
      case key
      when 'evil-edit'
        EVIL
      end
    end
  end
end


World do
  Tagutil::World.new
end
