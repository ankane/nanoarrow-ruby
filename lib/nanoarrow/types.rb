module Nanoarrow
  def self.null(nullable: true)
    Schema.new(Type::NULL, nullable:)
  end

  def self.bool(nullable: true)
    Schema.new(Type::BOOL, nullable:)
  end

  def self.int8(nullable: true)
    Schema.new(Type::INT8, nullable:)
  end

  def self.uint8(nullable: true)
    Schema.new(Type::UINT8, nullable:)
  end

  def self.int16(nullable: true)
    Schema.new(Type::INT16, nullable:)
  end

  def self.uint16(nullable: true)
    Schema.new(Type::UINT16, nullable:)
  end

  def self.int32(nullable: true)
    Schema.new(Type::INT32, nullable:)
  end

  def self.uint32(nullable: true)
    Schema.new(Type::UINT32, nullable:)
  end

  def self.int64(nullable: true)
    Schema.new(Type::INT64, nullable:)
  end

  def self.uint64(nullable: true)
    Schema.new(Type::UINT64, nullable:)
  end

  def self.half_float(nullable: true)
    Schema.new(Type::HALF_FLOAT, nullable:)
  end

  def self.float(nullable: true)
    Schema.new(Type::FLOAT, nullable:)
  end

  def self.double(nullable: true)
    Schema.new(Type::DOUBLE, nullable:)
  end

  def self.string(nullable: true)
    Schema.new(Type::STRING, nullable:)
  end

  def self.large_string(nullable: true)
    Schema.new(Type::LARGE_STRING, nullable:)
  end

  def self.string_view(nullable: true)
    Schema.new(Type::STRING_VIEW, nullable:)
  end

  def self.binary(nullable: true)
    Schema.new(Type::BINARY, nullable:)
  end

  def self.large_binary(nullable: true)
    Schema.new(Type::LARGE_BINARY, nullable:)
  end

  def self.binary_view(nullable: true)
    Schema.new(Type::BINARY_VIEW, nullable:)
  end

  def self.fixed_size_binary(byte_width, nullable: true)
    Schema.new(Type::FIXED_SIZE_BINARY, byte_width:, nullable:)
  end

  def self.date32(nullable: true)
    Schema.new(Type::DATE32, nullable:)
  end

  def self.date64(nullable: true)
    Schema.new(Type::DATE64, nullable:)
  end

  def self.time32(unit, nullable: true)
    Schema.new(Type::TIME32, unit:, nullable:)
  end

  def self.time64(unit, nullable: true)
    Schema.new(Type::TIME64, unit:, nullable:)
  end

  def self.timestamp(unit, timezone: nil, nullable: true)
    Schema.new(Type::TIMESTAMP, unit:, timezone:, nullable:)
  end

  def self.duration(unit, nullable: true)
    Schema.new(Type::DURATION, unit:, nullable:)
  end

  def self.interval_months(nullable: true)
    Schema.new(Type::INTERVAL_MONTHS, nullable:)
  end

  def self.interval_day_time(nullable: true)
    Schema.new(Type::INTERVAL_DAY_TIME, nullable:)
  end

  def self.interval_month_day_nano(nullable: true)
    Schema.new(Type::INTERVAL_MONTH_DAY_NANO, nullable:)
  end

  def self.decimal128(precision, scale, nullable: true)
    Schema.new(Type::DECIMAL128, precision:, scale:, nullable:)
  end

  def self.decimal256(precision, scale, nullable: true)
    Schema.new(Type::DECIMAL256, precision:, scale:, nullable:)
  end

  def self.struct(fields, nullable: true)
    Schema.new(Type::STRUCT, fields:, nullable:)
  end

  def self.list(value_type, nullable: true)
    Schema.new(Type::LIST, value_type:, nullable:)
  end

  def self.large_list(value_type, nullable: true)
    Schema.new(Type::LARGE_LIST, value_type:, nullable:)
  end

  def self.fixed_size_list(value_type, list_size, nullable: true)
    Schema.new(Type::FIXED_SIZE_LIST, value_type:, list_size:, nullable:)
  end

  def self.map(key_type, value_type, keys_sorted: false, nullable: true)
    Schema.new(Type::MAP, key_type:, value_type:, keys_sorted:, nullable:)
  end

  def self.dictionary(index_type, value_type, dictionary_ordered: false)
    Schema.new(Type::DICTIONARY, index_type:, value_type:, dictionary_ordered:)
  end

  def self.sparse_union(fields, nullable: true)
    Schema.new(Type::SPARSE_UNION, fields:, nullable:)
  end

  def self.dense_union(fields, nullable: true)
    Schema.new(Type::DENSE_UNION, fields:, nullable:)
  end
end
