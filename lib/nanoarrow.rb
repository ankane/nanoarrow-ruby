# ext
require "nanoarrow/ext"

# stdlib
require "date"

# modules
require_relative "nanoarrow/array"
require_relative "nanoarrow/array_builder"
require_relative "nanoarrow/array_stream"
require_relative "nanoarrow/iterator"
require_relative "nanoarrow/schema"
require_relative "nanoarrow/types"
require_relative "nanoarrow/utils"
require_relative "nanoarrow/version"

module Nanoarrow
  class Error < StandardError; end

  class Todo < Error
    def initialize(message = "not implemented yet")
      super(message)
    end
  end

  EPOCH = Date.new(1970, 1, 1)
  TIME_UNITS = {"s" => 0, "ms" => 1, "us" => 2, "ns" => 3}
end
