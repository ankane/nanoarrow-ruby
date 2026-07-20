module Nanoarrow
  class ArrayViewBaseIterator
    def initialize(schema)
      @schema = Utils.c_schema(schema)
      @schema_view = Utils.c_schema_view(schema)
      @array_view = CArrayView.from_schema(@schema)
    end

    def set_array(array)
      @array_view.set_array(array)
    end
  end

  class RbIterator < ArrayViewBaseIterator
    def self.get_iterator(obj, schema = nil)
      stream = Utils.c_array_stream(obj, schema)
      iterator = new(stream.get_cached_schema)
      Enumerator.new do |yielder|
        while (array = stream.get_next)
          iterator.set_array(array)
          iterator.each(&yielder)
        end
      end
    end

    def initialize(schema)
      super(schema)
    end

    def each(&block)
      type_id = @schema_view.type_id
      if !ITEMS_ITER_LOOKUP.include?(type_id)
        raise KeyError, "Can't resolve iterator for type #{@schema_view.type.inspect}"
      end

      factory = method(ITEMS_ITER_LOOKUP.fetch(type_id))
      factory.(&block)
    end

    private

    def each_bool(&block)
      @array_view.each_bool(&block)
    end

    def each_int(&block)
      @array_view.each_int(&block)
    end

    def each_uint(&block)
      @array_view.each_uint(&block)
    end

    def each_double(&block)
      @array_view.each_double(&block)
    end

    def each_string(&block)
      @array_view.each_string(&block)
    end

    def each_binary(&block)
      @array_view.each_binary(&block)
    end

    def each_date(&block)
      wrap = ->(v) { block.call(v.nil? ? v : EPOCH + v) }
      @array_view.each_int(&wrap)
    end

    def each_timestamp(&block)
      wrap =
        case @schema_view.time_unit
        when "s"
          ->(v) { block.call(v.nil? ? v : Time.at(v)) }
        when "ms"
          ->(v) { block.call(v.nil? ? v : Time.at(v / 1_000, v % 1_000, :millisecond)) }
        when "us"
          ->(v) { block.call(v.nil? ? v : Time.at(v / 1_000_000, v % 1_000_000, :microsecond)) }
        when "ns"
          ->(v) { block.call(v.nil? ? v : Time.at(v / 1_000_000_000, v % 1_000_000_000, :nanosecond)) }
        else
          raise Todo
        end
      @array_view.each_int(&wrap)
    end

    def each_decimal(&block)
      bitwidth = @schema_view.type_id == Type::DECIMAL128 ? 128 : 256
      wrap = ->(v) { block.call(v.nil? ? v : BigDecimal(v)) }
      @array_view.each_decimal(bitwidth, @schema_view.decimal_precision, @schema_view.decimal_scale, &wrap)
    end

    ITEMS_ITER_LOOKUP = {
      Type::BOOL => :each_bool,
      Type::INT8 => :each_int,
      Type::UINT8 => :each_uint,
      Type::INT16 => :each_int,
      Type::UINT16 => :each_uint,
      Type::INT32 => :each_int,
      Type::UINT32 => :each_uint,
      Type::INT64 => :each_int,
      Type::UINT64 => :each_uint,
      Type::HALF_FLOAT => :each_double,
      Type::FLOAT => :each_double,
      Type::DOUBLE => :each_double,
      Type::STRING => :each_string,
      Type::LARGE_STRING => :each_string,
      Type::STRING_VIEW => :each_string,
      Type::BINARY => :each_binary,
      Type::LARGE_BINARY => :each_binary,
      Type::BINARY_VIEW => :each_binary,
      Type::FIXED_SIZE_BINARY => :each_binary,
      Type::DATE32 => :each_date,
      Type::DATE64 => :each_date,
      Type::TIMESTAMP => :each_timestamp,
      Type::DECIMAL128 => :each_decimal,
      Type::DECIMAL256 => :each_decimal
    }
  end
end
