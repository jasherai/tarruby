require 'mkmf'

def make_libtar
  Dir.chdir('libtar')

  begin
    return false unless system('sh configure')

    if have_func('sigaction')
      open('config.h', 'a') do |fout|
        fout << "\n#define HAVE_SIGACTION 1\n"
      end
    end

    system('make')
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
