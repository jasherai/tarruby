require 'mkmf'

def make_libtar
  Dir.chdir('libtar')

  begin
    system('sh configure') and system('make')
  ensure
    Dir.chdir('..')
  end
end

if make_libtar and have_header('zlib.h') and have_library('z')
  have_header('bzlib.h')
  have_library('bz2')
  $CPPFLAGS << '-Ilibtar/lib -Ilibtar/listhash'
  $objs = %w(tarruby.o libtar/lib/libtar.a)
  create_makefile('tarruby')
end
