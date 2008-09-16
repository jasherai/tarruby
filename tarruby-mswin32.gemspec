Gem::Specification.new do |spec|
  spec.name              = 'tarruby'
  spec.version           = '0.1.4'
  spec.platform          = 'mswin32'
  spec.summary           = 'Ruby bindings for libtar.'
  spec.require_paths     = %w(lib/i386-mswin32)
  spec.files             = %w(lib/i386-mswin32/tarruby.so README.txt ext/tarruby.c)
  spec.author            = 'winebarrel'
  spec.email             = 'sgwr_dts@yahoo.co.jp'
  spec.homepage          = 'http://tarruby.rubyforge.org'
  spec.has_rdoc          = true
  spec.rdoc_options      << '--title' << 'TAR/Ruby - Ruby bindings for libtar.'
  spec.extra_rdoc_files  = %w(README.txt ext/tarruby.c)
  spec.rubyforge_project = 'tarruby'
end
