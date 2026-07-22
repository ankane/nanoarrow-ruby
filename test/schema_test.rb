require_relative "test_helper"

class SchemaTest < Minitest::Test
  def test_null
    assert_type "na", Nanoarrow.null
  end

  def test_bool
    assert_type "bool", Nanoarrow.bool
  end

  def test_int
    assert_type "int8", Nanoarrow.int8
    assert_type "uint8", Nanoarrow.uint8
    assert_type "int16", Nanoarrow.int16
    assert_type "uint16", Nanoarrow.uint16
    assert_type "int32", Nanoarrow.int32
    assert_type "uint32", Nanoarrow.uint32
    assert_type "int64", Nanoarrow.int64
    assert_type "uint64", Nanoarrow.uint64
  end

  def test_float
    assert_type "half_float", Nanoarrow.half_float
    assert_type "float", Nanoarrow.float
    assert_type "double", Nanoarrow.double
  end

  def test_string
    assert_type "string", Nanoarrow.string
    assert_type "large_string", Nanoarrow.large_string
    assert_type "string_view", Nanoarrow.string_view
  end

  def test_binary
    assert_type "binary", Nanoarrow.binary
    assert_type "large_binary", Nanoarrow.large_binary
    assert_type "binary_view", Nanoarrow.binary_view
    assert_type "fixed_size_binary(8)", Nanoarrow.fixed_size_binary(8)
    assert_equal 8, Nanoarrow.fixed_size_binary(8).byte_width
  end

  def test_date
    assert_type "date32", Nanoarrow.date32
    assert_type "date64", Nanoarrow.date64
  end

  def test_time
    assert_type "time32('s')", Nanoarrow.time32("s")
    assert_type "time32('ms')", Nanoarrow.time32("ms")
    assert_type "time64('us')", Nanoarrow.time64("us")
    assert_type "time64('ns')", Nanoarrow.time64("ns")
    assert_equal "s", Nanoarrow.time32("s").unit
    assert_equal "ms", Nanoarrow.time32("ms").unit
    assert_equal "us", Nanoarrow.time64("us").unit
    assert_equal "ns", Nanoarrow.time64("ns").unit
  end

  def test_timestamp
    assert_type "timestamp('s', '')", Nanoarrow.timestamp("s")
    assert_type "timestamp('s', 'America/New_York')", Nanoarrow.timestamp("s", timezone: "America/New_York")
    assert_type "timestamp('ms', '')", Nanoarrow.timestamp("ms")
    assert_type "timestamp('ms', 'America/New_York')", Nanoarrow.timestamp("ms", timezone: "America/New_York")
    assert_type "timestamp('us', '')", Nanoarrow.timestamp("us")
    assert_type "timestamp('us', 'America/New_York')", Nanoarrow.timestamp("us", timezone: "America/New_York")
    assert_type "timestamp('ns', '')", Nanoarrow.timestamp("ns")
    assert_type "timestamp('ns', 'America/New_York')", Nanoarrow.timestamp("ns", timezone: "America/New_York")
    assert_equal "ns", Nanoarrow.timestamp("ns").unit
    assert_equal "America/New_York", Nanoarrow.timestamp("ns", timezone: "America/New_York").timezone
  end

  def test_duration
    assert_type "duration('s')", Nanoarrow.duration("s")
    assert_type "duration('ms')", Nanoarrow.duration("ms")
    assert_type "duration('us')", Nanoarrow.duration("us")
    assert_type "duration('ns')", Nanoarrow.duration("ns")
  end

  def test_interval
    assert_type "interval_months", Nanoarrow.interval_months
    assert_type "interval_day_time", Nanoarrow.interval_day_time
    assert_type "interval_month_day_nano", Nanoarrow.interval_month_day_nano
  end

  def test_decimal
    assert_type "decimal128(2, 1)", Nanoarrow.decimal128(2, 1)
    assert_type "decimal256(2, 1)", Nanoarrow.decimal256(2, 1)
    assert_equal 2, Nanoarrow.decimal128(2, 1).precision
    assert_equal 1, Nanoarrow.decimal128(2, 1).scale
  end

  def test_struct
    assert_type "struct<: int64, : string>", Nanoarrow.struct([Nanoarrow.int64, Nanoarrow.string])
    assert_type "struct<a: int64, b: string>", Nanoarrow.struct({"a" => Nanoarrow.int64, "b" => Nanoarrow.string})
    assert_type "struct<: int64>", Nanoarrow.struct([Nanoarrow.int64])
    assert_type "struct<a: int64>", Nanoarrow.struct({"a" => Nanoarrow.int64})
  end

  def test_list
    assert_type "list<item: int32>", Nanoarrow.list(Nanoarrow.int32)
    assert_type "large_list<item: int32>", Nanoarrow.large_list(Nanoarrow.int32)
    assert_type "fixed_size_list(3)<item: int32>", Nanoarrow.fixed_size_list(Nanoarrow.int32, 3)
  end

  def test_map
    assert_type "map<entries: struct<key: int32, value: string>>", Nanoarrow.map(Nanoarrow.int32, Nanoarrow.string)
  end

  def test_dictionary
    assert_type "dictionary(int32)<string>", Nanoarrow.dictionary(Nanoarrow.int32, Nanoarrow.string)
    assert_type "dictionary(int32)<string>", Nanoarrow.dictionary(Nanoarrow.int32, Nanoarrow.string, dictionary_ordered: true)
  end

  def test_union
    assert_type "sparse_union([0,1])<: int32, : string>", Nanoarrow.sparse_union([Nanoarrow.int32, Nanoarrow.string])
    assert_type "dense_union([0,1])<: int32, : string>", Nanoarrow.dense_union([Nanoarrow.int32, Nanoarrow.string])
  end

  def test_name
    assert_equal "", Nanoarrow.int32.name

    schema = Nanoarrow::Schema.new(Nanoarrow.int32, name: "hello")
    assert_equal "hello", schema.name
  end

  def test_nullable
    assert_equal true, Nanoarrow.int32.nullable
    assert_equal false, Nanoarrow.int32(nullable: false).nullable
    assert_equal true, Nanoarrow::Schema.new(Nanoarrow.int32).nullable
    assert_equal false, Nanoarrow::Schema.new(Nanoarrow.int32(nullable: false)).nullable
    assert_equal true, Nanoarrow::Schema.new(Nanoarrow.int32, name: "a").nullable
    assert_equal false, Nanoarrow::Schema.new(Nanoarrow.int32(nullable: false), name: "a").nullable
  end

  def test_metadata
    assert_equal ({}), Nanoarrow::Schema.new(Nanoarrow.int32).metadata

    metadata = {"hello" => "world"}
    schema = Nanoarrow::Schema.new(Nanoarrow.int32, metadata: metadata)
    assert_equal metadata, schema.metadata
  end

  def test_from_polars
    skip unless polars?

    df = Polars::DataFrame.new({"a" => [1, 2, 3], "b" => ["one", "two", "three"]})
    schema = Nanoarrow::Schema.new(df.schema)
    df = nil
    GC.start
    assert_equal 2, schema.n_fields
    assert_equal "int64", schema.field(0).type
    assert_equal "string_view", schema.field(1).type
    assert_raises(IndexError) do
      schema.field(3)
    end
    assert_raises(IndexError) do
      schema.field(-1)
    end
    assert_equal ["int64", "string_view"], schema.fields.map(&:type)
  end

  def test_to_polars
    skip unless polars?

    schema = Nanoarrow.struct({"a" => Nanoarrow.int32, "b" => Nanoarrow.string})
    assert_equal ({"a" => Polars::Int32, "b" => Polars::String}), Polars::Schema.new(schema).to_h
  end

  private

  def assert_type(expected, schema)
    assert_equal expected, schema.inspect
  end
end
