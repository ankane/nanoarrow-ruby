module Nanoarrow
  module Utils
    def self.c_array(obj, schema)
      if !schema.nil?
        schema = c_schema(schema)
      end

      if obj.is_a?(CArray) && schema.nil?
        return obj
      end

      if obj.respond_to?(:arrow_c_array)
        raise Todo if schema
        return CArray.import_from_c_capsule(obj.arrow_c_array)
      end

      if obj.is_a?(Capsule) && obj.name == "arrow_array"
        raise Todo if schema
        return CArray.import_from_c_capsule(obj)
      end

      if schema.nil?
        raise Todo
      end
      builder = ArrayFromIterableBuilder.new(schema)
      builder.build_c_array(obj)
    end

    def self.c_array_stream(obj, schema)
      if !schema.nil?
        schema = c_schema(schema)
      end

      if obj.is_a?(CArrayStream) && schema.nil?
        return obj
      end

      if obj.respond_to?(:arrow_c_stream)
        raise Todo if schema
        return CArrayStream.import_from_c_capsule(obj.arrow_c_stream)
      end

      if obj.is_a?(Capsule) && obj.name == "arrow_array_stream"
        raise Todo if schema
        return CArrayStream.import_from_c_capsule(obj)
      end

      array = c_array(obj, schema)
      CArrayStream.from_c_arrays([array], array.schema, false)
    end

    def self.c_schema(obj)
      if obj.is_a?(CSchema)
        return obj
      end

      if obj.respond_to?(:arrow_c_schema)
        return CSchema.import_from_c_capsule(obj.arrow_c_schema)
      end

      if obj.is_a?(Capsule) && obj.name == "arrow_schema"
        return CSchema.import_from_c_capsule(obj)
      end

      raise TypeError, "Can't convert object of type #{obj.class.name} to CSchema"
    end

    def self.c_schema_view(obj)
      if obj.is_a?(CSchemaView)
        return obj
      end

      CSchemaView.new(c_schema(obj))
    end

    def self.c_schema_from_type_and_params(type, params)
      factory = CSchemaBuilder.new

      if [Type::DECIMAL128, Type::DECIMAL256].include?(type)
        precision = Integer(params.delete(:precision))
        scale = Integer(params.delete(:scale))
        factory.set_type_decimal(type, precision, scale)

      elsif [Type::TIME32, Type::TIME64, Type::TIMESTAMP, Type::DURATION].include?(type)
        time_unit = params.delete(:unit)
        timezone = params.delete(:timezone)
        factory.set_type_date_time(type, TIME_UNITS.fetch(time_unit), timezone)

      elsif type == Type::FIXED_SIZE_BINARY
        factory.set_type_fixed_size(type, Integer(params.delete(:byte_width)))

      elsif type == Type::LIST
        factory.set_format("+l")
        factory.allocate_children(1)
        factory.set_child(0, "item", c_schema(params.delete(:value_type)))

      elsif type == Type::LARGE_LIST
        factory.set_format("+L")
        factory.allocate_children(1)
        factory.set_child(0, "item", c_schema(params.delete(:value_type)))

      elsif type == Type::FIXED_SIZE_LIST
        fixed_size = Integer(params.delete(:list_size))
        factory.set_format("+w:%d" % fixed_size)
        factory.allocate_children(1)
        factory.set_child(0, "item", c_schema(params.delete(:value_type)))

      elsif type == Type::MAP
        key_schema = c_schema(params.delete(:key_type))
        value_schema = c_schema(params.delete(:value_type))

        entries = CSchemaBuilder.allocate
        entries.set_format("+s")
        entries.set_nullable(false)
        entries.allocate_children(2)
        entries.set_child(0, "key", key_schema.modify(nullable: false))
        entries.set_child(1, "value", value_schema)

        factory.set_format("+m")
        factory.allocate_children(1)
        factory.set_child(0, "entries", entries.finish)
        factory.set_nullable(false)

        if params.include?(:keys_sorted)
          factory.set_map_keys_sorted(params.delete(:keys_sorted))
        end

      elsif type == Type::DICTIONARY
        raise Todo

      elsif type == Type::SPARSE_UNION
        type_codes = params.delete(:type_codes)
        type_codes_str = type_codes.map { |code| Integer(code) }.join(",")
        factory.set_format("+us:#{type_codes_str}")

      elsif type == Type::DENSE_UNION
        type_codes = params.delete(:type_codes)
        type_codes_str = type_codes.map { |code| Integer(code) }.join(",")
        factory.set_format("+ud:#{type_codes_str}")

      else
        factory.set_type(type)
      end

      if params.any?
        unused = params.keys.map(&:inspect).join(", ")
        raise ArgumentError, "Unused parameters whilst constructing Schema: #{unused}"
      end

      factory.set_name("")

      factory.finish
    end
  end
end
