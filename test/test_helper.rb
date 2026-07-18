require "bundler/setup"
Bundler.require(:default)
require "minitest/autorun"

class Minitest::Test
  def setup
    GC.stress = true if stress?
  end

  def teardown
    GC.stress = false if stress?
  end

  def stress?
    ENV["STRESS"]
  end

  def assert_array(expected, obj)
    assert_kind_of Nanoarrow::Array, obj
    assert_equal expected, obj.to_a
  end

  def polars?
    defined?(Polars)
  end
end
