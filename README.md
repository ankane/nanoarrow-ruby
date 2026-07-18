# nanoarrow.rb

Zero-dependency Arrow library for Ruby, powered by [nanoarrow](https://arrow.apache.org/nanoarrow)

[![Build Status](https://github.com/ankane/nanoarrow-ruby/actions/workflows/build.yml/badge.svg)](https://github.com/ankane/nanoarrow-ruby/actions)

## Installation

Add this line to your application’s Gemfile:

```ruby
gem "nanoarrow"
```

## Getting Started

Create an Arrow array

```ruby
arr = Nanoarrow::Array.new([1, 2, 3], Nanoarrow.int32)
```

Get a Ruby array

```ruby
arr.to_a
```

## API

This library follows the [Python API](https://arrow.apache.org/nanoarrow/latest/reference/python/index.html). You can follow Python tutorials and convert the code to Ruby in many cases. Feel free to open an issue if you run into problems.

## History

View the [changelog](https://github.com/ankane/nanoarrow-ruby/blob/master/CHANGELOG.md)

## Contributing

Everyone is encouraged to help improve this project. Here are a few ways you can help:

- [Report bugs](https://github.com/ankane/nanoarrow-ruby/issues)
- Fix bugs and [submit pull requests](https://github.com/ankane/nanoarrow-ruby/pulls)
- Write, clarify, or fix documentation
- Suggest or add new features

To get started with development:

```sh
git clone https://github.com/ankane/nanoarrow-ruby.git
cd nanoarrow-ruby
bundle install
bundle exec rake compile
bundle exec rake test
```
