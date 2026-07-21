module Nanoarrow
  class ArrayBuilder
    def initialize(schema)
      @schema = Utils.c_schema(schema)
      @schema_view = Utils.c_schema_view(@schema)
      @c_builder = CArrayBuilder.allocate
      @c_builder.init_from_schema(@schema)
    end

    def build_c_array(obj)
      start_building
      append(obj)
      finish_building
    end
  end

  class ArrayFromIterableBuilder < ArrayBuilder
    def initialize(schema)
      super(schema)

      type_id = @schema_view.type_id
      if !ARRAY_BUILDER_FROM_ITERABLE_METHOD.include?(type_id)
        raise ArgumentError, "Can't build array of type #{@schema_view.type} from iterable"
      end

      method_name = ARRAY_BUILDER_FROM_ITERABLE_METHOD.fetch(type_id)
      @append_impl = method(method_name)
    end

    def start_building
      @c_builder.start_appending
    end

    def append(obj)
      @append_impl.(obj)
    end

    def finish_building
      @c_builder.finish
    end

    private

    def append_bools(obj)
      @c_builder.append_bools(obj)
    end

    def append_ints(obj)
      @c_builder.append_ints(obj)
    end

    def append_uints(obj)
      @c_builder.append_uints(obj)
    end

    def append_doubles(obj)
      @c_builder.append_doubles(obj)
    end

    def append_strings(obj)
      @c_builder.append_strings(obj)
    end

    def append_bytes(obj)
      @c_builder.append_bytes(obj)
    end

    def append_dates(obj)
      @c_builder.append_ints(obj.map { |v| v.nil? ? v : v - EPOCH })
    end

    def append_timestamps(obj)
      obj =
        case @schema_view.time_unit
        when "s"
          obj.map { |v| v.nil? ? v : v.to_i }
        when "ms"
          # truncates like usec
          obj.map { |v| v.nil? ? v : v.to_i * 1_000 + v.usec / 1_000 }
        when "us"
          obj.map { |v| v.nil? ? v : v.to_i * 1_000_000 + v.usec }
        when "ns"
          obj.map { |v| v.nil? ? v : v.to_i * 1_000_000_000 + v.nsec }
        else
          raise Todo
        end
      @c_builder.append_ints(obj)
    end

    def append_decimals(obj)
      obj = obj.map { |v| (v.nil? || v.is_a?(BigDecimal)) ? v : BigDecimal(v.to_s) }
      bitwidth = @schema_view.type_id == Type::DECIMAL128 ? 128 : 256
      @c_builder.append_decimals(obj, bitwidth, @schema_view.decimal_precision, @schema_view.decimal_scale)
    end

    ARRAY_BUILDER_FROM_ITERABLE_METHOD = {
      Type::BOOL => :append_bools,
      Type::INT8 => :append_ints,
      Type::UINT8 => :append_uints,
      Type::INT16 => :append_ints,
      Type::UINT16 => :append_uints,
      Type::INT32 => :append_ints,
      Type::UINT32 => :append_uints,
      Type::INT64 => :append_ints,
      Type::UINT64 => :append_uints,
      Type::HALF_FLOAT => :append_doubles,
      Type::FLOAT => :append_doubles,
      Type::DOUBLE => :append_doubles,
      Type::STRING => :append_strings,
      Type::LARGE_STRING => :append_strings,
      Type::STRING_VIEW => :append_strings,
      Type::BINARY => :append_bytes,
      Type::LARGE_BINARY => :append_bytes,
      Type::BINARY_VIEW => :append_bytes,
      Type::FIXED_SIZE_BINARY => :append_bytes,
      Type::DATE32 => :append_dates,
      Type::DATE64 => :append_dates,
      Type::TIMESTAMP => :append_timestamps,
      Type::DECIMAL128 => :append_decimals,
      Type::DECIMAL256 => :append_decimals
    }
  end
end
