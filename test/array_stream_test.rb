require_relative "test_helper"

class ArrayStreamTest < Minitest::Test
  def test_read_all
    chunked = Nanoarrow::Array.from_chunks([[1, 2, 3], [4, 5, 6]], Nanoarrow.int32)
    stream = Nanoarrow::ArrayStream.new(chunked)
    assert_equal [1, 2, 3, 4, 5, 6], stream.read_all.to_a
  end

  def test_read_next
    chunked = Nanoarrow::Array.from_chunks([[1, 2, 3], [4, 5, 6]], Nanoarrow.int32)
    stream = Nanoarrow::ArrayStream.new(chunked)
    assert_equal [1, 2, 3], stream.read_next.to_a
    assert_equal [4, 5, 6], stream.read_next.to_a
    assert_nil stream.read_next
  end

  def test_each
    chunked = Nanoarrow::Array.from_chunks([[1, 2, 3], [4, 5, 6]], Nanoarrow.int32)
    stream = Nanoarrow::ArrayStream.new(chunked)
    chunks = []
    stream.each do |chunk|
      chunks << chunk.to_a
    end
    assert_equal [[1, 2, 3], [4, 5, 6]], chunks
  end

  def test_polars
    skip unless polars?

    df = Polars::DataFrame.new({"a" => [1, 2, 3], "b" => ["one", "two", "three"]})
    stream = Nanoarrow::ArrayStream.new(df)
    df = nil
    GC.start
    assert stream.schema
    assert stream.read_next
  end
end
