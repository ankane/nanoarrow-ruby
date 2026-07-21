#include <ruby/ruby.h>

#include "ext.h"
#include "nanoarrow/nanoarrow.h"

VALUE cCArrayBuilder;

typedef struct array_builder {
    VALUE c_array;
    struct ArrowArray* ptr;
} array_builder_t;

static void array_builder_mark(void* ptr)
{
    array_builder_t* builder = (array_builder_t*) ptr;
    rb_gc_mark_movable(builder->c_array);
}

const rb_data_type_t array_builder_data_type = {
    .wrap_struct_name = "Nanoarrow::CArrayBuilder",
    .function = {
        .dmark = array_builder_mark,
        .dfree = RUBY_TYPED_DEFAULT_FREE,
    },
    .flags = RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE array_builder_allocate(VALUE klass)
{
    array_builder_t* builder;
    VALUE obj = TypedData_Make_Struct(klass, array_builder_t, &array_builder_data_type, builder);

    array_t* array;
    VALUE c_array = rb_funcall(cCArray, rb_intern("allocate"), 0);
    GetArray(c_array, array);

    array->schema = rb_funcall(cCSchema, rb_intern("allocate"), 0);

    builder->c_array = c_array;
    builder->ptr = array->ptr;
    return obj;
}

static VALUE array_builder_init_from_schema(VALUE self, VALUE schema)
{
    array_builder_t* builder;
    GetArrayBuilder(self, builder);

    if (builder->ptr->release != NULL)
        rb_raise(rb_eRuntimeError, "CArrayBuilder is already initialized");

    schema_t* schema_ptr;
    GetSchema(schema, schema_ptr);

    struct ArrowError error;
    int code = ArrowArrayInitFromSchema(builder->ptr, schema_ptr->ptr, &error);
    raise_message_not_ok(&error, "ArrowArrayInitFromType()", code);

    array_t* c_array;
    GetArray(builder->c_array, c_array);

    c_array->schema = schema;
    return self;
}

static VALUE array_builder_start_appending(VALUE self)
{
    array_builder_t* builder;
    GetArrayBuilder(self, builder);

    int code = ArrowArrayStartAppending(builder->ptr);
    raise_error_not_ok("ArrowArrayStartAppending()", code);
    return self;
}

static VALUE array_builder_append_bools(VALUE self, VALUE obj)
{
    array_builder_t* builder;
    GetArrayBuilder(self, builder);

    Check_Type(obj, T_ARRAY);

    for (long i = 0; i < RARRAY_LEN(obj); i++)
    {
        VALUE v = rb_ary_entry(obj, i);
        int code;

        if (NIL_P(v))
            code = ArrowArrayAppendNull(builder->ptr, 1);
        else
            code = ArrowArrayAppendInt(builder->ptr, RTEST(v));

        raise_error_not_ok("ArrowArrayAppendInt()", code);
    }

    return self;
}

static VALUE array_builder_append_ints(VALUE self, VALUE obj)
{
    array_builder_t* builder;
    GetArrayBuilder(self, builder);

    Check_Type(obj, T_ARRAY);

    for (long i = 0; i < RARRAY_LEN(obj); i++)
    {
        VALUE v = rb_ary_entry(obj, i);
        int code;

        if (NIL_P(v))
            code = ArrowArrayAppendNull(builder->ptr, 1);
        else
            code = ArrowArrayAppendInt(builder->ptr, NUM2LL(v));

        if (code != NANOARROW_OK)
            rb_raise(rb_eRangeError, "integer out of range");
    }

    return self;
}

static VALUE array_builder_append_uints(VALUE self, VALUE obj)
{
    array_builder_t* builder;
    GetArrayBuilder(self, builder);

    Check_Type(obj, T_ARRAY);

    for (long i = 0; i < RARRAY_LEN(obj); i++)
    {
        VALUE v = rb_ary_entry(obj, i);
        int code;

        if (NIL_P(v))
            code = ArrowArrayAppendNull(builder->ptr, 1);
        else
        {
            // prevent primitive types from wrapping
            if ((FIXNUM_P(v) && FIX2LONG(v) < 0) || (RB_FLOAT_TYPE_P(v) && RFLOAT_VALUE(v) < 0))
                rb_raise(rb_eRangeError, "integer out of range");

            code = ArrowArrayAppendUInt(builder->ptr, NUM2ULL(v));
        }

        if (code != NANOARROW_OK)
            rb_raise(rb_eRangeError, "integer out of range");
    }

    return self;
}

static VALUE array_builder_append_doubles(VALUE self, VALUE obj)
{
    array_builder_t* builder;
    GetArrayBuilder(self, builder);

    Check_Type(obj, T_ARRAY);

    for (long i = 0; i < RARRAY_LEN(obj); i++)
    {
        VALUE v = rb_ary_entry(obj, i);
        int code;

        if (NIL_P(v))
            code = ArrowArrayAppendNull(builder->ptr, 1);
        else
            code = ArrowArrayAppendDouble(builder->ptr, NUM2DBL(v));

        raise_error_not_ok("ArrowArrayAppendDouble()", code);
    }

    return self;
}

static VALUE array_builder_append_strings(VALUE self, VALUE obj)
{
    array_builder_t* builder;
    GetArrayBuilder(self, builder);

    Check_Type(obj, T_ARRAY);

    for (long i = 0; i < RARRAY_LEN(obj); i++)
    {
        VALUE v = rb_ary_entry(obj, i);
        int code;

        if (NIL_P(v))
            code = ArrowArrayAppendNull(builder->ptr, 1);
        else
        {
            Check_Type(v, T_STRING);
            struct ArrowStringView sv;
            sv.data = StringValuePtr(v);
            sv.size_bytes = RSTRING_LEN(v);
            code = ArrowArrayAppendString(builder->ptr, sv);
        }

        raise_error_not_ok("ArrowArrayAppendString()", code);
    }

    return self;
}

static VALUE array_builder_append_bytes(VALUE self, VALUE obj)
{
    array_builder_t* builder;
    GetArrayBuilder(self, builder);

    Check_Type(obj, T_ARRAY);

    for (long i = 0; i < RARRAY_LEN(obj); i++)
    {
        VALUE v = rb_ary_entry(obj, i);
        int code;

        if (NIL_P(v))
            code = ArrowArrayAppendNull(builder->ptr, 1);
        else
        {
            Check_Type(v, T_STRING);
            struct ArrowBufferView bv;
            bv.data.data = StringValuePtr(v);
            bv.size_bytes = RSTRING_LEN(v);
            code = ArrowArrayAppendBytes(builder->ptr, bv);
        }

        raise_error_not_ok("ArrowArrayAppendBytes()", code);
    }

    return self;
}

static VALUE array_builder_append_decimals(VALUE self, VALUE obj, VALUE bitwidth, VALUE precision, VALUE scale)
{
    array_builder_t* builder;
    GetArrayBuilder(self, builder);

    int pi = NUM2INT(precision);
    int si = NUM2INT(scale);
    VALUE pad = rb_str_new_cstr("0");

    struct ArrowDecimal decimal;
    ArrowDecimalInit(&decimal, NUM2INT(bitwidth), pi, si);

    Check_Type(obj, T_ARRAY);

    for (long i = 0; i < RARRAY_LEN(obj); i++)
    {
        VALUE v = rb_ary_entry(obj, i);
        int code;

        if (NIL_P(v))
            code = ArrowArrayAppendNull(builder->ptr, 1);
        else
        {
            VALUE split = rb_funcall(v, rb_intern("split"), 0);
            VALUE sign = rb_ary_entry(split, 0);
            VALUE digits = rb_ary_entry(split, 1);
            VALUE base = rb_ary_entry(split, 2);
            VALUE exponent = rb_ary_entry(split, 3);

            if (NUM2INT(base) != 10)
                raise_todo();

            Check_Type(digits, T_STRING);

            // exceeds precision
            // TODO better error
            if (RSTRING_LEN(digits) > pi)
                raise_todo();

            int target = si + NUM2INT(exponent);

            // exceeds scale
            // TODO either round in Ruby or better error
            if (RSTRING_LEN(digits) > target)
                raise_todo();

            digits = rb_funcall(digits, rb_intern("ljust"), 2, INT2NUM(target), pad);

            struct ArrowStringView sv;
            sv.data = StringValuePtr(digits);
            sv.size_bytes = RSTRING_LEN(digits);
            code = ArrowDecimalSetDigits(&decimal, sv);
            raise_error_not_ok("ArrowDecimalSetDigits()", code);

            if (NUM2INT(sign) == -1)
                ArrowDecimalNegate(&decimal);

            code = ArrowArrayAppendDecimal(builder->ptr, &decimal);
        }

        raise_error_not_ok("ArrowArrayAppendDecimal()", code);
    }

    return self;
}

static VALUE array_builder_finish(VALUE self)
{
    array_builder_t* builder;
    GetArrayBuilder(self, builder);

    struct ArrowError error;
    int code = ArrowArrayFinishBuildingDefault(builder->ptr, &error);
    raise_message_not_ok(&error, "ArrowArrayFinishBuildingDefault()", code);

    VALUE out = builder->c_array;

    array_t* array;
    VALUE c_array = rb_funcall(cCArray, rb_intern("allocate"), 0);
    GetArray(c_array, array);
    array->schema = rb_funcall(cCSchema, rb_intern("allocate"), 0);
    builder->c_array = c_array;
    builder->ptr = array->ptr;

    return out;
}

void Init_array_builder(void)
{
    cCArrayBuilder = rb_define_class_under(mNanoarrow, "CArrayBuilder", rb_cObject);
    rb_define_alloc_func(cCArrayBuilder, array_builder_allocate);
    rb_define_method(cCArrayBuilder, "init_from_schema", array_builder_init_from_schema, 1);
    rb_define_method(cCArrayBuilder, "start_appending", array_builder_start_appending, 0);
    rb_define_method(cCArrayBuilder, "append_bools", array_builder_append_bools, 1);
    rb_define_method(cCArrayBuilder, "append_ints", array_builder_append_ints, 1);
    rb_define_method(cCArrayBuilder, "append_uints", array_builder_append_uints, 1);
    rb_define_method(cCArrayBuilder, "append_doubles", array_builder_append_doubles, 1);
    rb_define_method(cCArrayBuilder, "append_strings", array_builder_append_strings, 1);
    rb_define_method(cCArrayBuilder, "append_bytes", array_builder_append_bytes, 1);
    rb_define_method(cCArrayBuilder, "append_decimals", array_builder_append_decimals, 4);
    rb_define_method(cCArrayBuilder, "finish", array_builder_finish, 0);
}
