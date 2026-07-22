#include <ruby/ruby.h>

#include "ext.h"
#include "nanoarrow/nanoarrow.h"

VALUE cCArrayView;

typedef struct array_view {
    VALUE base;
    VALUE array_base;
    struct ArrowArrayView* ptr;
} array_view_t;

static void array_view_mark(void* ptr)
{
    array_view_t* view = (array_view_t*) ptr;
    rb_gc_mark_movable(view->base);
    rb_gc_mark_movable(view->array_base);
}

const rb_data_type_t array_view_data_type = {
    .wrap_struct_name = "Nanoarrow::CArrayView",
    .function = {
        .dmark = array_view_mark,
        .dfree = RUBY_TYPED_DEFAULT_FREE,
    },
    .flags = RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE array_view_allocate(VALUE klass)
{
    array_view_t* view;
    VALUE obj = TypedData_Make_Struct(klass, array_view_t, &array_view_data_type, view);
    view->base = Qnil;
    view->array_base = Qnil;
    view->ptr = NULL;
    return obj;
}

static VALUE array_view_from_schema(VALUE klass, VALUE schema)
{
    struct ArrowArrayView* c_array_view;
    VALUE base = alloc_c_array_view(&c_array_view);

    schema_t* schema_ptr;
    GetSchema(schema, schema_ptr);

    struct ArrowError error;
    int code = ArrowArrayViewInitFromSchema(c_array_view, schema_ptr->ptr, &error);
    raise_message_not_ok(&error, "ArrowArrayViewInitFromSchema()", code);

    array_view_t* view;
    VALUE obj = TypedData_Make_Struct(klass, array_view_t, &array_view_data_type, view);
    view->base = base;
    view->array_base = Qnil;
    view->ptr = c_array_view;
    return obj;
}

static VALUE array_view_set_array(VALUE self, VALUE array)
{
    array_view_t* view;
    GetArrayView(self, view);

    struct ArrowError error;
    int code;

    array_t* array_ptr;
    GetArray(array, array_ptr);

    code = ArrowArrayViewSetArray(view->ptr, array_ptr->ptr, &error);

    raise_message_not_ok(&error, "ArrowArrayViewSetArray()", code);
    view->array_base = array_ptr->base;

    return self;
}

static VALUE array_view_n_children(VALUE self)
{
    array_view_t* view;
    GetArrayView(self, view);

    return LL2NUM(view->ptr->n_children);
}

static VALUE array_view_child(VALUE self, VALUE idx)
{
    array_view_t* view;
    GetArrayView(self, view);

    int64_t i = NUM2LL(idx);
    if (i < 0 || i >= view->ptr->n_children)
        rb_raise(rb_eIndexError, "out of range");

    VALUE child = array_view_allocate(cCArrayView);
    array_view_t* child_ptr;
    GetArrayView(child, child_ptr);
    child_ptr->base = view->base;
    child_ptr->ptr = view->ptr->children[i];

    return child;
}

static VALUE array_view_children(VALUE self)
{
    int64_t n_children = NUM2LL(array_view_n_children(self));
    VALUE children = rb_ary_new();
    for (int64_t i = 0; i < n_children; i++)
        rb_ary_push(children, array_view_child(self, LL2NUM(i)));
    return children;
}

static VALUE array_view_each_bool(VALUE self)
{
    array_view_t* view;
    GetArrayView(self, view);

    for (int64_t i = 0; i < view->ptr->length; i++)
        rb_yield(ArrowArrayViewIsNull(view->ptr, i) ? Qnil : ArrowArrayViewGetIntUnsafe(view->ptr, i) ? Qtrue : Qfalse);

    return self;
}

static VALUE array_view_each_int(VALUE self)
{
    array_view_t* view;
    GetArrayView(self, view);

    for (int64_t i = 0; i < view->ptr->length; i++)
        rb_yield(ArrowArrayViewIsNull(view->ptr, i) ? Qnil : LL2NUM(ArrowArrayViewGetIntUnsafe(view->ptr, i)));

    return self;
}

static VALUE array_view_each_uint(VALUE self)
{
    array_view_t* view;
    GetArrayView(self, view);

    for (int64_t i = 0; i < view->ptr->length; i++)
        rb_yield(ArrowArrayViewIsNull(view->ptr, i) ? Qnil : ULL2NUM(ArrowArrayViewGetUIntUnsafe(view->ptr, i)));

    return self;
}

static VALUE array_view_each_double(VALUE self)
{
    array_view_t* view;
    GetArrayView(self, view);

    for (int64_t i = 0; i < view->ptr->length; i++)
        rb_yield(ArrowArrayViewIsNull(view->ptr, i) ? Qnil : DBL2NUM(ArrowArrayViewGetDoubleUnsafe(view->ptr, i)));

    return self;
}

static VALUE array_view_each_string(VALUE self)
{
    array_view_t* view;
    GetArrayView(self, view);

    for (int64_t i = 0; i < view->ptr->length; i++)
    {
        VALUE v;
        if (ArrowArrayViewIsNull(view->ptr, i))
            v = Qnil;
        else
        {
            struct ArrowStringView sv = ArrowArrayViewGetStringUnsafe(view->ptr, i);
            v = rb_str_new(sv.data, sv.size_bytes);
        }
        rb_yield(v);
    }

    return self;
}

static VALUE array_view_each_binary(VALUE self)
{
    array_view_t* view;
    GetArrayView(self, view);

    for (int64_t i = 0; i < view->ptr->length; i++)
    {
        VALUE v;
        if (ArrowArrayViewIsNull(view->ptr, i))
            v = Qnil;
        else
        {
            struct ArrowBufferView bv = ArrowArrayViewGetBytesUnsafe(view->ptr, i);
            v = rb_str_new(bv.data.data, bv.size_bytes);
        }
        rb_yield(v);
    }

    return self;
}

static VALUE array_view_each_decimal(VALUE self, VALUE bitwidth, VALUE precision, VALUE scale)
{
    array_view_t* view;
    GetArrayView(self, view);

    struct ArrowDecimal decimal;
    ArrowDecimalInit(&decimal, NUM2INT(bitwidth), NUM2INT(precision), NUM2INT(scale));

    struct ArrowBuffer buffer;
    ArrowBufferInit(&buffer);

    for (int64_t i = 0; i < view->ptr->length; i++)
    {
        VALUE v;
        if (ArrowArrayViewIsNull(view->ptr, i))
            v = Qnil;
        else
        {
            ArrowArrayViewGetDecimalUnsafe(view->ptr, i, &decimal);
            int code = ArrowDecimalAppendStringToBuffer(&decimal, &buffer);
            raise_error_not_ok("ArrowDecimalAppendStringToBuffer()", code);
            v = rb_str_new((char*) buffer.data, buffer.size_bytes);
            ArrowBufferReset(&buffer);
        }
        rb_yield(v);
    }

    return self;
}

void Init_array_view(void)
{
    cCArrayView = rb_define_class_under(mNanoarrow, "CArrayView", rb_cObject);
    rb_define_alloc_func(cCArrayView, array_view_allocate);
    rb_define_singleton_method(cCArrayView, "from_schema", array_view_from_schema, 1);
    rb_define_method(cCArrayView, "set_array", array_view_set_array, 1);
    rb_define_method(cCArrayView, "n_children", array_view_n_children, 0);
    rb_define_method(cCArrayView, "child", array_view_child, 1);
    rb_define_method(cCArrayView, "children", array_view_children, 0);
    rb_define_method(cCArrayView, "each_bool", array_view_each_bool, 0);
    rb_define_method(cCArrayView, "each_int", array_view_each_int, 0);
    rb_define_method(cCArrayView, "each_uint", array_view_each_uint, 0);
    rb_define_method(cCArrayView, "each_double", array_view_each_double, 0);
    rb_define_method(cCArrayView, "each_string", array_view_each_string, 0);
    rb_define_method(cCArrayView, "each_binary", array_view_each_binary, 0);
    rb_define_method(cCArrayView, "each_decimal", array_view_each_decimal, 3);
}
