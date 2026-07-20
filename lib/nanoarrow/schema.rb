module Nanoarrow
  class Schema
    def initialize(
      obj,
      name: nil,
      nullable: nil,
      metadata: nil,
      fields: nil,
      **params
    )
      if obj.is_a?(Integer)
        @c_schema = Utils.c_schema_from_type_and_params(obj, params)
      else
        if params.any?
          raise ArgumentError, "params are only supported for obj of class Type"
        end
        @c_schema = Utils.c_schema(obj)
      end

      if !name.nil? || !nullable.nil? || !metadata.nil? || !fields.nil?
        @c_schema = @c_schema.modify(name:, nullable:, metadata:, children: clean_fields(fields))
      end

      @c_schema_view = CSchemaView.new(@c_schema)
    end

    def type
      @c_schema_view.type
    end

    def name
      @c_schema.name
    end

    def nullable
      @c_schema_view.nullable
    end

    def metadata
      @c_schema.metadata
    end

    def byte_width
      if @c_schema_view.type_id == Type::FIXED_SIZE_BINARY
        @c_schema_view.fixed_size
      end
    end

    def unit
      @c_schema_view.time_unit
    end

    def timezone
      @c_schema_view.timezone
    end

    def precision
      @c_schema_view.decimal_precision
    end

    def scale
      @c_schema_view.decimal_scale
    end

    def n_fields
      @c_schema.n_children
    end

    def field(i)
      Schema.new(@c_schema.child(i).deep_dup)
    end

    def fields
      n_fields.times.map { |i| field(i) }
    end

    def inspect
      @c_schema.to_s
    end

    def arrow_c_schema
      @c_schema.arrow_c_schema
    end

    private

    def clean_fields(fields)
      if fields.nil?
        fields
      elsif fields.is_a?(Hash)
        fields.transform_values { |v| Utils.c_schema(v) }
      else
        fields.map { |v| Utils.c_schema(v) }
      end
    end
  end
end
