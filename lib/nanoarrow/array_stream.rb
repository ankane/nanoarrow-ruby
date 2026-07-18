module Nanoarrow
  class ArrayStream
    def initialize(obj, schema = nil)
      @c_array_stream = Utils.c_array_stream(obj, schema)
    end

    def schema
      Schema.new(@c_array_stream.get_cached_schema)
    end

    def arrow_c_stream
      @c_array_stream.arrow_c_stream
    end

    def read_all
      Array.new(@c_array_stream)
    end

    def read_next
      c_array = @c_array_stream.get_next
      Array.new(CMaterializedArrayStream.from_c_array(c_array)) if c_array
    end

    def each
      return to_enum(:each) unless block_given?

      while (c_array = @c_array_stream.get_next)
        yield Array.new(CMaterializedArrayStream.from_c_array(c_array))
      end
    end
  end
end
