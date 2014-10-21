#!/usr/bin/env ruby
# encoding: UTF-8

module Tagutil
    ProjectRoot = File.join(File.dirname(__FILE__), '..', '..', '..')
    Executable  = File.join(ProjectRoot, 'build', 'tagutil')

    def self.build
        system "make -C #{ProjectRoot}"
        File.join(ProjectRoot, 'build', 'tagutil')
    end
end

Tagutil.build
