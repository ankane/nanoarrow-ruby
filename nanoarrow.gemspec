require_relative "lib/nanoarrow/version"

Gem::Specification.new do |spec|
  spec.name          = "nanoarrow"
  spec.version       = Nanoarrow::VERSION
  spec.summary       = "Zero-dependency Arrow library for Ruby"
  spec.homepage      = "https://github.com/ankane/nanoarrow-ruby"
  spec.license       = "Apache-2.0"

  spec.author        = "Andrew Kane"
  spec.email         = "andrew@ankane.org"

  spec.files         = Dir["*.{md,txt}", "{ext,lib}/**/*"]
  spec.require_path  = "lib"
  spec.extensions    = ["ext/nanoarrow/extconf.rb"]

  spec.required_ruby_version = ">= 3.3"
end
