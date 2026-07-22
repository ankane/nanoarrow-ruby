require_relative "test_helper"

class ArrayTest < Minitest::Test
  def test_bool
    assert_array [true, nil, false], Nanoarrow::Array.new([true, nil, false], Nanoarrow.bool)
  end

  def test_int
    assert_array [1, nil, 3], Nanoarrow::Array.new([1, nil, 3], Nanoarrow.int8)
    assert_array [1, nil, 3], Nanoarrow::Array.new([1, nil, 3], Nanoarrow.uint8)
    assert_array [1, nil, 3], Nanoarrow::Array.new([1, nil, 3], Nanoarrow.int16)
    assert_array [1, nil, 3], Nanoarrow::Array.new([1, nil, 3], Nanoarrow.uint16)
    assert_array [1, nil, 3], Nanoarrow::Array.new([1, nil, 3], Nanoarrow.int32)
    assert_array [1, nil, 3], Nanoarrow::Array.new([1, nil, 3], Nanoarrow.uint32)
    assert_array [1, nil, 3], Nanoarrow::Array.new([1, nil, 3], Nanoarrow.int64)
    assert_array [1, nil, 3], Nanoarrow::Array.new([1, nil, 3], Nanoarrow.uint64)
  end

  def test_float
    assert_array [1.5, nil, 3.5], Nanoarrow::Array.new([1.5, nil, 3.5], Nanoarrow.half_float)
    assert_array [1.5, nil, 3.5], Nanoarrow::Array.new([1.5, nil, 3.5], Nanoarrow.float)
    assert_array [1.5, nil, 3.5], Nanoarrow::Array.new([1.5, nil, 3.5], Nanoarrow.double)
  end

  def test_string
    assert_array ["one", nil, "three"], Nanoarrow::Array.new(["one", nil, "three"], Nanoarrow.string)
    assert_array ["one", nil, "three"], Nanoarrow::Array.new(["one", nil, "three"], Nanoarrow.large_string)
    assert_array ["one", nil, "three"], Nanoarrow::Array.new(["one", nil, "three"], Nanoarrow.string_view)
  end

  def test_binary
    assert_array ["one", nil, "three"], Nanoarrow::Array.new(["one", nil, "three"], Nanoarrow.binary)
    assert_array ["one", nil, "three"], Nanoarrow::Array.new(["one", nil, "three"], Nanoarrow.large_binary)
    assert_array ["one", nil, "three"], Nanoarrow::Array.new(["one", nil, "three"], Nanoarrow.binary_view)
    assert_array ["one!!", nil, "three"], Nanoarrow::Array.new(["one!!", nil, "three"], Nanoarrow.fixed_size_binary(5))

    assert_raises(Nanoarrow::Error) do
      Nanoarrow::Array.new(["one", nil, "three"], Nanoarrow.fixed_size_binary(5))
    end

    assert_raises(Nanoarrow::Error) do
      Nanoarrow::Array.new(["one!!", nil, "three"], Nanoarrow.fixed_size_binary(4))
    end

    assert_raises(Nanoarrow::Error) do
      Nanoarrow::Array.new(["one!!", nil, "three"], Nanoarrow.fixed_size_binary(6))
    end
  end

  def test_date
    today = Date.today
    assert_array [today, nil, today + 2], Nanoarrow::Array.new([today, nil, today + 2], Nanoarrow.date32)
    assert_array [today, nil, today + 2], Nanoarrow::Array.new([today, nil, today + 2], Nanoarrow.date64)
  end

  def test_timestamp
    now = Time.now.round
    assert_array [now, nil, now + 2], Nanoarrow::Array.new([now, nil, now + 2], Nanoarrow.timestamp("s"))

    now = Time.now.round(3)
    assert_array [now, nil, now + 2], Nanoarrow::Array.new([now, nil, now + 2], Nanoarrow.timestamp("ms"))

    now = Time.now.round(6)
    assert_array [now, nil, now + 2], Nanoarrow::Array.new([now, nil, now + 2], Nanoarrow.timestamp("us"))

    now = Time.now
    assert_array [now, nil, now + 2], Nanoarrow::Array.new([now, nil, now + 2], Nanoarrow.timestamp("ns"))
  end

  def test_decimal
    require "bigdecimal"

    assert_array [BigDecimal("1000"), nil, BigDecimal("-1.23456789")], Nanoarrow::Array.new([BigDecimal("1000"), nil, BigDecimal("-1.23456789")], Nanoarrow.decimal128(38, 10))
    assert_array [BigDecimal("1000"), nil, BigDecimal("-1.23456789")], Nanoarrow::Array.new([BigDecimal("1000"), nil, BigDecimal("-1.23456789")], Nanoarrow.decimal256(38, 10))
    assert_array [1000, nil, -1.23456789], Nanoarrow::Array.new([BigDecimal("1000"), nil, BigDecimal("-1.23456789")], Nanoarrow.decimal128(38, 10))
    assert_array [BigDecimal("0.00005")], Nanoarrow::Array.new([BigDecimal("0.00005")], Nanoarrow.decimal128(38, 10))
    assert_array [BigDecimal("1000")], Nanoarrow::Array.new([1000], Nanoarrow.decimal128(38, -3))

    assert_raises(Nanoarrow::Todo) do
      assert_array [BigDecimal("123")], Nanoarrow::Array.new([123], Nanoarrow.decimal128(2, 0))
    end

    assert_raises(Nanoarrow::Todo) do
      Nanoarrow::Array.new([1.5], Nanoarrow.decimal128(38, 0))
    end

    assert_raises(Nanoarrow::Todo) do
      Nanoarrow::Array.new([1000], Nanoarrow.decimal128(38, -4))
    end
  end

  def test_struct
    rows = [{"a" => 1, "b" => "one"}, nil, {"a" => 3, "b" => "three"}]
    assert_array rows, Nanoarrow::Array.new(rows, Nanoarrow.struct({"a" => Nanoarrow.int32, "b" => Nanoarrow.string}))

    assert_raises(KeyError) do
      Nanoarrow::Array.new([{}], Nanoarrow.struct({"a" => Nanoarrow.int32}))
    end

    # TODO raise error for extra rows
    Nanoarrow::Array.new([{"a" => 1, "b" => "one"}], Nanoarrow.struct({"a" => Nanoarrow.int32}))
  end

  def test_schema_missing
    assert_raises(Nanoarrow::Todo) do
      Nanoarrow::Array.new([1, nil, 3])
    end
  end

  def test_from_chunks
    a = Nanoarrow::Array.from_chunks([[1, 2, 3], [4, 5, 6]], Nanoarrow.int32)
    assert_equal 6, a.length
    assert_equal 2, a.n_chunks
    assert_array [1, 2, 3], a.chunk(0)
    assert_array [4, 5, 6], a.chunk(1)
    assert_raises(IndexError) do
      a.chunk(2)
    end
    assert_equal [[1, 2, 3], [4, 5, 6]], a.each_chunk.map(&:to_a)
  end

  def test_int_range
    assert_array [127], Nanoarrow::Array.new([127], Nanoarrow.int8)
    assert_array [-128], Nanoarrow::Array.new([-128], Nanoarrow.int8)

    error = assert_raises(RangeError) do
      Nanoarrow::Array.new([128], Nanoarrow.int8)
    end
    assert_equal "integer out of range", error.message

    error = assert_raises(RangeError) do
      Nanoarrow::Array.new([-129], Nanoarrow.int8)
    end
    assert_equal "integer out of range", error.message

    assert_raises(TypeError) do
      Nanoarrow::Array.new(["one"], Nanoarrow.int8)
    end

    # could raise error instead
    assert_array [1], Nanoarrow::Array.new([1.5], Nanoarrow.int8)
  end

  def test_uint_range
    assert_array [255], Nanoarrow::Array.new([255], Nanoarrow.uint8)

    error = assert_raises(RangeError) do
      Nanoarrow::Array.new([256], Nanoarrow.uint8)
    end
    assert_equal "integer out of range", error.message

    error = assert_raises(RangeError) do
      Nanoarrow::Array.new([-1], Nanoarrow.uint64)
    end
    assert_equal "integer out of range", error.message

    error = assert_raises(RangeError) do
      Nanoarrow::Array.new([-1.0], Nanoarrow.uint64)
    end
    assert_equal "integer out of range", error.message

    error = assert_raises(RangeError) do
      Nanoarrow::Array.new([2**64], Nanoarrow.uint64)
    end
    assert_match "too big to convert", error.message

    assert_raises(TypeError) do
      Nanoarrow::Array.new(["one"], Nanoarrow.uint64)
    end

    # could raise error instead
    assert_array [1], Nanoarrow::Array.new([1.5], Nanoarrow.uint64)
  end

  def test_inspect
    expected = "#<Nanoarrow::Array int64[3]>"
    assert_equal expected, Nanoarrow::Array.new([1, nil, 3], Nanoarrow.int64).inspect
  end

  def test_polars
    skip unless polars?

    require "bigdecimal"

    today = Date.today
    now = Time.now
    df =
      Polars::DataFrame.new({
        "a" => [1, 2, 3],
        "b" => ["one", "two", "three"],
        "c" => [today, nil, today + 2],
        "d" => [now, nil, now + 2],
        "e" => [BigDecimal("1"), BigDecimal("0.001"), BigDecimal("-1.23456789")]
      })
    rows = df.to_a
    a = Nanoarrow::Array.new(df)
    df = nil
    GC.start
    assert_equal "#<Nanoarrow::Array struct<a: int64, b: string_view, c: date32, d: timestamp('ns', ''), e: decimal128(38, 8)>[3]>", a.inspect
    schema = a.schema
    assert_equal "struct", schema.type
    assert_equal ["int64", "string_view", "date32", "timestamp", "decimal128"], schema.fields.map(&:type)
    assert_equal 5, a.n_children
    assert_array [1, 2, 3], a.child(0)
    assert_array ["one", "two", "three"], a.child(1)
    assert_array [today, nil, today + 2], a.child(2)
    assert_array [now, nil, now + 2], a.child(3)
    assert_array [BigDecimal("1"), BigDecimal("0.001"), BigDecimal("-1.23456789")], a.child(4)
    assert_raises(IndexError) do
      a.child(5)
    end
    assert_raises(IndexError) do
      a.child(-1)
    end
    assert_equal rows, a.to_a
  end
end
