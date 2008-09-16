Gem::Specification.new do |spec|
  spec.name              = 'tarruby'
  spec.version           = '0.1.4'
  spec.summary           = 'Ruby bindings for libtar.'
  spec.files             = Dir.glob('ext/**/*') + %w(README.txt)
  spec.author            = 'winebarrel'
  spec.email             = 'sgwr_dts@yahoo.co.jp'
  spec.homepage          = 'http://tarruby.rubyforge.org'
  spec.extensions        = 'ext/extconf.rb'
  spec.has_rdoc          = true
  spec.rdoc_options      << '--title' << 'TAR/Ruby - Ruby bindings for libtar.'
  spec.extra_rdoc_files  = %w(README.txt ext/tarruby.c)
  spec.rubyforge_project = 'tarruby'
end
