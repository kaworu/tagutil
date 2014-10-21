#!/usr/bin/env ruby
# encoding: UTF-8

module Tagutil
    ProjectRoot = File.join(File.dirname(__FILE__), '..', '..', '..')
    Executable  = File.join(ProjectRoot, 'build', 'tagutil')

    def self.build
        self.force_build unless File.exist?(Executable)
    end

    def self.force_build
        system "make -C #{ProjectRoot}"
    end
end
