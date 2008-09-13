= TAR/Ruby

Copyright (c) 2008 SUGAWARA Genki <sgwr_dts@yahoo.co.jp>

== Description

Ruby bindings for libtar.

libtar is a C library for manipulating POSIX tar files.

== Project Page

http://rubyforge.org/projects/tarruby

== Install

gem install tarruby

== Example
=== reading tar archive

    require 'tarruby'
    
    Tar.open('foo.tar', File::RDONLY, 0644, Tar::GNU | Tar::VERBOSE) do |tar|
      while tar.read # or 'tar.each do ...'
        puts tar.pathname
        tar.print_long_ls
        
        if tar.reg? # regular file
          tar.extract_file('bar.txt')
          
          ##if extract buffer
          #puts tar.extract_buffer
        end
      end
      
      ##if extract all files
      #tar.extract_all
    end
    
    ##for gzip archive
    #Tar.gzopen('foo.tar.gz', ...
    
    ##for bzip2 archive
    #Tar.bzopen('foo.tar.bz2', ...

=== creating tar archive

    require 'tarruby'
    
    Tar.open('bar.tar', File::CREAT | File::WRONLY, 0644, Tar::GNU | Tar::VERBOSE) do |tar|
      Dir.glob('**/*.c').each do |filename|
        tar.append_file(filename)
      end
      
      ##if append directory
      #tar.append_tree('dirname')
    end
    
    ##for gzip archive
    #Tar.gzopen('foo.tar.gz', ...
    
    ##for bzip2 archive
    #Tar.bzopen('foo.tar.bz2', ...

== License
    Copyright (c) 2008 SUGAWARA Genki <sgwr_dts@yahoo.co.jp>
    All rights reserved.
    
    Redistribution and use in source and binary forms, with or without modification,
    are permitted provided that the following conditions are met:
    
        * Redistributions of source code must retain the above copyright notice, 
          this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright notice, 
          this list of conditions and the following disclaimer in the documentation 
          and/or other materials provided with the distribution.
        * The names of its contributors may be used to endorse or promote products 
           derived from this software without specific prior written permission.
    
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
    ANY EXPRESS OR IMPLIED WARRANTIES,
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
    FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
    OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
    STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH 
    DAMAGE.

=== libtar
TAR/Ruby contains libtar.

* libtar is a C library for manipulating POSIX tar files.
* http://www.feep.net/libtar/
* patches
  * https://lists.feep.net:8080/pipermail/libtar/2007-July/000240.html
