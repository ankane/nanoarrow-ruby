module Nanoarrow
  class Array
    def initialize(obj, schema = nil)
      if obj.is_a?(CMaterializedArrayStream) && schema.nil?
        @data = obj
        return
      end

      if obj.is_a?(Array) && schema.nil?
        @data = obj.instance_variable_get(:@data)
        return
      end

      if obj.is_a?(CArray) && schema.nil?
        @data = CMaterializedArrayStream.from_c_array(obj)
        return
      end

      stream = Utils.c_array_stream(obj, schema)
      @data = CMaterializedArrayStream.from_c_array_stream(stream)
    end

    def self.from_chunks(obj, schema = nil, validate: true)
      if schema.nil?
        raise Todo
      else
        out_schema = Utils.c_schema(schema)
      end

      data =
        CMaterializedArrayStream.from_c_arrays(
          obj.map { |item| Utils.c_array(item, schema) },
          out_schema,
          validate
        )

      Array.new(data)
    end

    def arrow_c_stream
      @data.arrow_c_stream
    end

    def schema
      Schema.new(@data.schema)
    end

    def n_children
      @data.schema.n_children
    end

    def child(i)
      Array.new(@data.child(i))
    end

    def each_child
      return to_enum(:each_child) unless block_given?

      n_children.times do |i|
        yield child(i)
      end
    end

    def n_chunks
      @data.n_arrays
    end

    def chunk(i)
      Array.new(@data.array(i))
    end

    def each_chunk
      return to_enum(:each_chunk) unless block_given?

      n_chunks.times do |i|
        yield chunk(i)
      end
    end

    def length
      @data.length
    end
    alias_method :size, :length

    def each
      RbIterator.get_iterator(self)
    end

    def to_a
      each.to_a
    end

    def inspect
      "#<#{self.class.name} #{schema.inspect}[#{length.inspect}]>"
    end
  end
end
