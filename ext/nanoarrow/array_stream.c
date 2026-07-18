#include <ruby/ruby.h>

#include "ext.h"
#include "nanoarrow/nanoarrow.h"

VALUE cCArrayStream;

typedef struct array_stream {
    VALUE base;
    struct ArrowArrayStream* ptr;
    VALUE cached_schema;
} array_stream_t;

static void array_stream_mark(void* ptr)
{
    array_stream_t* stream = (array_stream_t*) ptr;
    rb_gc_mark_movable(stream->base);
    rb_gc_mark_movable(stream->cached_schema);
}

const rb_data_type_t array_stream_data_type = {
    .wrap_struct_name = "Nanoarrow::CArrayStream",
    .function = {
        .dmark = array_stream_mark,
        .dfree = RUBY_TYPED_DEFAULT_FREE,
    },
    .flags = RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE array_stream_allocate(VALUE klass)
{
    array_stream_t* stream;
    VALUE obj = TypedData_Make_Struct(klass, array_stream_t, &array_stream_data_type, stream);
    stream->base = Qnil;
    stream->ptr = NULL;
    stream->cached_schema = Qnil;
    return obj;
}

static VALUE array_stream_import_from_c_capsule(VALUE klass, VALUE schema_capsule)
{
    array_stream_t* stream;
    VALUE obj = array_stream_allocate(klass);
    GetArrayStream(obj, stream);

    VALUE address = rb_funcall(schema_capsule, rb_intern("to_i"), 0);
    stream->base = schema_capsule;
    stream->ptr = (struct ArrowArrayStream*) NUM2ULL(address);
    return obj;
}

static VALUE array_stream_from_c_arrays(VALUE klass, VALUE arrays, VALUE schema, VALUE validate)
{
    array_stream_t* stream;
    VALUE out = array_stream_allocate(klass);
    GetArrayStream(out, stream);

    // TODO move to parameter
    VALUE move = Qfalse;

    VALUE out_schema;
    VALUE validate_schema;
    if (RTEST(validate) && !RTEST(move))
    {
        validate_schema = schema;
        out_schema = rb_funcall(schema, rb_intern("deep_dup"), 0);
    }
    else if (RTEST(validate))
    {
        validate_schema = rb_funcall(schema, rb_intern("deep_dup"), 0);
        out_schema = schema;
    }
    else
        out_schema = rb_funcall(schema, rb_intern("deep_dup"), 0);

    schema_t* schema_ptr;
    GetSchema(out_schema, schema_ptr);

    Check_Type(arrays, T_ARRAY);
    long arrays_len = RARRAY_LEN(arrays);

    stream->base = alloc_c_array_stream(&stream->ptr);

    int code = ArrowBasicArrayStreamInit(stream->ptr, schema_ptr->ptr, arrays_len);
    raise_error_not_ok("ArrowBasicArrayStreamInit()", code);

    struct ArrowArrayStream* c_array_stream_out = stream->ptr;
    struct ArrowArray tmp;
    for (long i = 0; i < arrays_len; i++)
    {
        array_t* array;
        GetArray(rb_ary_entry(arrays, i), array);

        if (RTEST(validate))
            assert_type_equal(array->schema, validate_schema, Qfalse);

        if (!RTEST(move))
        {
            c_array_shallow_copy(array->base, array->ptr, &tmp);
            ArrowBasicArrayStreamSetArray(c_array_stream_out, i, &tmp);
        }
        else
            ArrowBasicArrayStreamSetArray(c_array_stream_out, i, array->ptr);
    }

    if (RTEST(validate))
    {
        struct ArrowError error;
        int code = ArrowBasicArrayStreamValidate(c_array_stream_out, &error);
        raise_message_not_ok(&error, "ArrowBasicArrayStreamValidate()", code);
    }

    return out;
}

static VALUE array_stream_assert_valid(VALUE self)
{
    array_stream_t* stream;
    GetArrayStream(self, stream);

    if (stream->ptr == NULL)
        rb_raise(rb_eRuntimeError, "array stream pointer is NULL");
    if (stream->ptr->release == NULL)
        rb_raise(rb_eRuntimeError, "array stream is released");

    return Qnil;
}

static VALUE array_stream_get_schema(VALUE self, VALUE schema)
{
    array_stream_t* stream;
    GetArrayStream(self, stream);

    array_stream_assert_valid(self);

    struct ArrowError error;
    schema_t* schema_ptr;
    GetSchema(schema, schema_ptr);
    int code = ArrowArrayStreamGetSchema(stream->ptr, schema_ptr->ptr, &error);
    raise_message_not_ok(&error, "ArrowArrayStream::get_schema()", code);
    return Qnil;
}

static VALUE array_stream_get_cached_schema(VALUE self)
{
    array_stream_t* stream;
    GetArrayStream(self, stream);

    if (NIL_P(stream->cached_schema))
    {
        stream->cached_schema = rb_funcall(cCSchema, rb_intern("allocate"), 0);
        array_stream_get_schema(self, stream->cached_schema);
    }
    return stream->cached_schema;
}

static VALUE array_stream_get_next(VALUE self)
{
    array_stream_t* stream;
    GetArrayStream(self, stream);

    array_stream_assert_valid(self);

    array_t* array_ptr;
    VALUE array = rb_funcall(cCArray, rb_intern("allocate"), 0);
    GetArray(array, array_ptr);
    array_ptr->schema = array_stream_get_cached_schema(self);

    struct ArrowError error;
    int code = ArrowArrayStreamGetNext(stream->ptr, array_ptr->ptr, &error);
    raise_message_not_ok(&error, "ArrowArrayStreamGetNext()", code);

    if (!RTEST(rb_funcall(array, rb_intern("valid?"), 0)))
        return Qnil;

    return array;
}

static VALUE array_stream_arrow_c_stream(VALUE self)
{
    array_stream_t* stream;
    GetArrayStream(self, stream);

    array_stream_assert_valid(self);

    struct ArrowArrayStream* c_array_stream_out;
    VALUE array_stream_capsule = alloc_c_array_stream(&c_array_stream_out);
    ArrowArrayStreamMove(stream->ptr, c_array_stream_out);
    return array_stream_capsule;
}

void Init_array_stream(void)
{
    cCArrayStream = rb_define_class_under(mNanoarrow, "CArrayStream", rb_cObject);
    rb_define_alloc_func(cCArrayStream, array_stream_allocate);
    rb_define_singleton_method(cCArrayStream, "import_from_c_capsule", array_stream_import_from_c_capsule, 1);
    rb_define_singleton_method(cCArrayStream, "from_c_arrays", array_stream_from_c_arrays, 3);
    rb_define_method(cCArrayStream, "get_next", array_stream_get_next, 0);
    rb_define_method(cCArrayStream, "get_cached_schema", array_stream_get_cached_schema, 0);
    rb_define_method(cCArrayStream, "arrow_c_stream", array_stream_arrow_c_stream, 0);
}
