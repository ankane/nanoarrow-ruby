#include <ruby/ruby.h>

#include "ext.h"
#include "nanoarrow/nanoarrow.h"

VALUE cCArray;

static void array_mark(void* ptr)
{
    array_t* array = (array_t*) ptr;
    rb_gc_mark_movable(array->base);
    rb_gc_mark_movable(array->schema);
}

const rb_data_type_t array_data_type = {
    .wrap_struct_name = "Nanoarrow::CArray",
    .function = {
        .dmark = array_mark,
        .dfree = RUBY_TYPED_DEFAULT_FREE,
    },
    .flags = RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE array_allocate(VALUE klass)
{
    array_t* array;
    VALUE obj = TypedData_Make_Struct(klass, array_t, &array_data_type, array);
    array->base = alloc_c_array(&array->ptr);
    array->schema = Qnil;
    return obj;
}

static VALUE array_schema(VALUE self)
{
    array_t* array;
    GetArray(self, array);

    return array->schema;
}

static VALUE array_is_valid(VALUE self)
{
    array_t* array;
    GetArray(self, array);

    return array->ptr != NULL && array->ptr->release != NULL ? Qtrue : Qfalse;
}

static VALUE array_assert_valid(VALUE self)
{
    array_t* array;
    GetArray(self, array);

    if (array->ptr == NULL)
        rb_raise(rb_eRuntimeError, "CArray is NULL");
    if (array->ptr->release == NULL)
        rb_raise(rb_eRuntimeError, "CArray is released");

    return Qnil;
}

static VALUE array_length(VALUE self)
{
    array_t* array;
    GetArray(self, array);

    array_assert_valid(self);

    return LL2NUM(array->ptr->length);
}

static VALUE array_child(VALUE self, VALUE idx)
{
    array_t* array;
    GetArray(self, array);

    array_assert_valid(self);

    int64_t i = NUM2LL(idx);
    if (i < 0 || i >= array->ptr->n_children)
        rb_raise(rb_eIndexError, "out of range");

    array_t* out_ptr;
    VALUE out = TypedData_Make_Struct(cCArray, array_t, &array_data_type, out_ptr);
    out_ptr->base = array->base;
    out_ptr->ptr = array->ptr->children[i];
    out_ptr->schema = rb_funcall(array->schema, rb_intern("child"), 1, idx);
    return out;
}

void Init_array(void)
{
    cCArray = rb_define_class_under(mNanoarrow, "CArray", rb_cObject);
    rb_define_alloc_func(cCArray, array_allocate);
    rb_define_method(cCArray, "schema", array_schema, 0);
    rb_define_method(cCArray, "valid?", array_is_valid, 0);
    rb_define_method(cCArray, "assert_valid", array_assert_valid, 0);
    rb_define_method(cCArray, "length", array_length, 0);
    rb_define_method(cCArray, "child", array_child, 1);
}
