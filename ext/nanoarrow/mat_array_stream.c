#include <ruby/ruby.h>

#include "ext.h"
#include "nanoarrow/nanoarrow.h"

VALUE cCMatArrayStream;

typedef struct mat_array_stream {
    VALUE schema;
    VALUE arrays;
    int64_t total_length;
} mat_array_stream_t;

static void mat_array_stream_mark(void* ptr)
{
    mat_array_stream_t* stream = (mat_array_stream_t*) ptr;
    rb_gc_mark_movable(stream->schema);
    rb_gc_mark_movable(stream->arrays);
}

const rb_data_type_t mat_array_stream_data_type = {
    .wrap_struct_name = "Nanoarrow::CMaterializedArrayStream",
    .function = {
        .dmark = mat_array_stream_mark,
        .dfree = RUBY_TYPED_DEFAULT_FREE,
    },
    .flags = RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE mat_array_stream_allocate(VALUE klass)
{
    mat_array_stream_t* stream;
    VALUE obj = TypedData_Make_Struct(klass, mat_array_stream_t, &mat_array_stream_data_type, stream);
    stream->schema = rb_funcall(cCSchema, rb_intern("allocate"), 0);
    stream->arrays = rb_ary_new();
    stream->total_length = 0;
    return obj;
}

static VALUE mat_array_stream_from_c_arrays(VALUE klass, VALUE arrays, VALUE schema, VALUE validate)
{
    mat_array_stream_t* out_ptr;
    VALUE out = mat_array_stream_allocate(klass);
    GetMatArrayStream(out, out_ptr);

    Check_Type(arrays, T_ARRAY);

    for (long i = 0; i < RARRAY_LEN(arrays); i++)
    {
        VALUE array = rb_ary_entry(arrays, i);

        if (!rb_obj_is_instance_of(array, cCArray))
            rb_raise(rb_eTypeError, "Expected CArray");

        int64_t len = NUM2LL(rb_funcall(array, rb_intern("length"), 0));
        if (len == 0)
            continue;

        if (RTEST(validate))
        {
            array_t* array_ptr;
            GetArray(array, array_ptr);
            assert_type_equal(array_ptr->schema, schema, Qfalse);
        }

        out_ptr->total_length += len;

        rb_ary_push(out_ptr->arrays, array);
    }

    out_ptr->schema = schema;

    return out;
}

static VALUE mat_array_stream_from_c_array(VALUE klass, VALUE array)
{
    VALUE schema = rb_funcall(array, rb_intern("schema"), 0);
    return mat_array_stream_from_c_arrays(klass, rb_ary_new3(1, array), schema, Qfalse);
}

static VALUE mat_array_stream_from_c_array_stream(VALUE klass, VALUE stream)
{
    VALUE arrays = rb_ary_new();
    while (true)
    {
        VALUE v = rb_funcall(stream, rb_intern("get_next"), 0);
        if (NIL_P(v))
            break;
        rb_ary_push(arrays, v);
    }

    VALUE schema = rb_funcall(stream, rb_intern("get_cached_schema"), 0);
    return mat_array_stream_from_c_arrays(klass, arrays, schema, Qfalse);
}

static VALUE mat_array_stream_schema(VALUE self)
{
    mat_array_stream_t* stream;
    GetMatArrayStream(self, stream);

    return stream->schema;
}

static VALUE mat_array_stream_length(VALUE self)
{
    mat_array_stream_t* stream;
    GetMatArrayStream(self, stream);

    return LL2NUM(stream->total_length);
}

static VALUE mat_array_stream_array(VALUE self, VALUE idx)
{
    mat_array_stream_t* stream;
    GetMatArrayStream(self, stream);

    return rb_funcall(stream->arrays, rb_intern("fetch"), 1, idx);
}

static VALUE mat_array_stream_n_arrays(VALUE self)
{
    mat_array_stream_t* stream;
    GetMatArrayStream(self, stream);

    Check_Type(stream->arrays, T_ARRAY);
    return LONG2NUM(RARRAY_LEN(stream->arrays));
}

static VALUE mat_array_stream_arrays(VALUE self)
{
    mat_array_stream_t* stream;
    GetMatArrayStream(self, stream);

    return stream->arrays;
}

static VALUE mat_array_stream_arrow_c_stream(VALUE self)
{
    mat_array_stream_t* stream;
    GetMatArrayStream(self, stream);

    VALUE stream_ptr = rb_funcall(cCArrayStream, rb_intern("from_c_arrays"), 3, stream->arrays, stream->schema, Qfalse);
    return rb_funcall(stream_ptr, rb_intern("arrow_c_stream"), 0);
}

static VALUE mat_array_stream_child(VALUE self, VALUE idx)
{
    mat_array_stream_t* stream;
    GetMatArrayStream(self, stream);

    mat_array_stream_t* out_ptr;
    VALUE out = mat_array_stream_allocate(cCMatArrayStream);
    GetMatArrayStream(out, out_ptr);

    out_ptr->schema = rb_funcall(stream->schema, rb_intern("child"), 1, idx);

    Check_Type(stream->arrays, T_ARRAY);
    long arrays_len = RARRAY_LEN(stream->arrays);
    out_ptr->arrays = rb_ary_new_capa(arrays_len);

    for (long i = 0; i < arrays_len; i++)
    {
        VALUE child = rb_funcall(rb_ary_entry(stream->arrays, i), rb_intern("child"), 1, idx);
        out_ptr->total_length += NUM2LL(rb_funcall(child, rb_intern("length"), 0));
        rb_ary_push(out_ptr->arrays, child);
    }

    return out;
}

void Init_materialized(void)
{
    cCMatArrayStream = rb_define_class_under(mNanoarrow, "CMaterializedArrayStream", rb_cObject);
    rb_define_alloc_func(cCMatArrayStream, mat_array_stream_allocate);
    rb_define_singleton_method(cCMatArrayStream, "from_c_arrays", mat_array_stream_from_c_arrays, 3);
    rb_define_singleton_method(cCMatArrayStream, "from_c_array", mat_array_stream_from_c_array, 1);
    rb_define_singleton_method(cCMatArrayStream, "from_c_array_stream", mat_array_stream_from_c_array_stream, 1);
    rb_define_method(cCMatArrayStream, "schema", mat_array_stream_schema, 0);
    rb_define_method(cCMatArrayStream, "length", mat_array_stream_length, 0);
    rb_define_method(cCMatArrayStream, "array", mat_array_stream_array, 1);
    rb_define_method(cCMatArrayStream, "n_arrays", mat_array_stream_n_arrays, 0);
    rb_define_method(cCMatArrayStream, "arrays", mat_array_stream_arrays, 0);
    rb_define_method(cCMatArrayStream, "arrow_c_stream", mat_array_stream_arrow_c_stream, 0);
    rb_define_method(cCMatArrayStream, "child", mat_array_stream_child, 1);
}
