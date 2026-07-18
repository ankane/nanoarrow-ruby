#include <ruby/ruby.h>

#include "ext.h"
#include "nanoarrow/nanoarrow.h"

VALUE cCSchemaView;

typedef struct schema_view {
    VALUE base;
    struct ArrowSchemaView schema_view;
    int nullable;
} schema_view_t;

static void schema_view_mark(void* ptr)
{
    schema_view_t* view = (schema_view_t*) ptr;
    rb_gc_mark_movable(view->base);
}

const rb_data_type_t schema_view_data_type = {
    .wrap_struct_name = "Nanoarrow::CSchemaView",
    .function = {
        .dmark = schema_view_mark,
        .dfree = RUBY_TYPED_DEFAULT_FREE,
    },
    .flags = RUBY_TYPED_FREE_IMMEDIATELY
};

VALUE schema_view_allocate(VALUE klass)
{
    schema_view_t* view;
    VALUE obj = TypedData_Make_Struct(klass, schema_view_t, &schema_view_data_type, view);
    view->base = Qnil;
    view->schema_view.type = NANOARROW_TYPE_UNINITIALIZED;
    view->schema_view.storage_type = NANOARROW_TYPE_UNINITIALIZED;
    view->nullable = 0;
    return obj;
}

VALUE schema_view_initialize(VALUE self, VALUE schema)
{
    schema_view_t* view;
    GetSchemaView(self, view);

    view->base = schema;
    view->schema_view.type = NANOARROW_TYPE_UNINITIALIZED;
    view->schema_view.storage_type = NANOARROW_TYPE_UNINITIALIZED;

    schema_t* schema_ptr;
    GetSchema(schema, schema_ptr);

    struct ArrowError error;
    int code = ArrowSchemaViewInit(&view->schema_view, schema_ptr->ptr, &error);
    raise_message_not_ok(&error, "ArrowSchemaViewInit()", code);

    view->nullable = schema_ptr->ptr->flags & ARROW_FLAG_NULLABLE;

    return self;
}

VALUE schema_view_type_id(VALUE self)
{
    schema_view_t* view;
    GetSchemaView(self, view);

    return INT2NUM(view->schema_view.type);
}

VALUE schema_view_storage_type_id(VALUE self)
{
    schema_view_t* view;
    GetSchemaView(self, view);

    return INT2NUM(view->schema_view.storage_type);
}

VALUE schema_view_type(VALUE self)
{
    schema_view_t* view;
    GetSchemaView(self, view);

    const char* type_str = ArrowTypeString(view->schema_view.type);
    if (type_str != NULL)
        return rb_utf8_str_new_cstr(type_str);
    else
        rb_raise(rb_eArgError, "ArrowTypeString() returned NULL");
}

VALUE schema_view_nullable(VALUE self)
{
    schema_view_t* view;
    GetSchemaView(self, view);

    return view->nullable != 0 ? Qtrue : Qfalse;
}

bool is_fixed_size(int type)
{
    switch (type)
    {
        case NANOARROW_TYPE_FIXED_SIZE_BINARY:
        case NANOARROW_TYPE_FIXED_SIZE_LIST:
            return true;
        default:
            return false;
    }
}

bool is_decimal(int type)
{
    switch (type)
    {
        case NANOARROW_TYPE_DECIMAL128:
        case NANOARROW_TYPE_DECIMAL256:
            return true;
        default:
            return false;
    }
}

bool has_time_unit(int type)
{
    switch (type)
    {
        case NANOARROW_TYPE_TIME32:
        case NANOARROW_TYPE_TIME64:
        case NANOARROW_TYPE_TIMESTAMP:
        case NANOARROW_TYPE_DURATION:
            return true;
        default:
            return false;
    }
}

VALUE schema_view_fixed_size(VALUE self)
{
    schema_view_t* view;
    GetSchemaView(self, view);

    if (is_fixed_size(view->schema_view.type))
        return INT2NUM(view->schema_view.fixed_size);
    else
        return Qnil;
}

VALUE schema_view_decimal_precision(VALUE self)
{
    schema_view_t* view;
    GetSchemaView(self, view);

    if (is_decimal(view->schema_view.type))
        return INT2NUM(view->schema_view.decimal_precision);
    else
        return Qnil;
}

VALUE schema_view_decimal_scale(VALUE self)
{
    schema_view_t* view;
    GetSchemaView(self, view);

    if (is_decimal(view->schema_view.type))
        return INT2NUM(view->schema_view.decimal_scale);
    else
        return Qnil;
}

VALUE schema_view_time_unit(VALUE self)
{
    schema_view_t* view;
    GetSchemaView(self, view);

    if (has_time_unit(view->schema_view.type))
        return rb_utf8_str_new_cstr(ArrowTimeUnitString(view->schema_view.time_unit));
    else
        return Qnil;
}

VALUE schema_view_timezone(VALUE self)
{
    schema_view_t* view;
    GetSchemaView(self, view);

    if (view->schema_view.type == NANOARROW_TYPE_TIMESTAMP)
        return rb_utf8_str_new_cstr(view->schema_view.timezone);
    else
        return Qnil;
}

void Init_schema_view(void)
{
    cCSchemaView = rb_define_class_under(mNanoarrow, "CSchemaView", rb_cObject);
    rb_define_alloc_func(cCSchemaView, schema_view_allocate);
    rb_define_method(cCSchemaView, "initialize", schema_view_initialize, 1);
    rb_define_method(cCSchemaView, "type_id", schema_view_type_id, 0);
    rb_define_method(cCSchemaView, "storage_type_id", schema_view_storage_type_id, 0);
    rb_define_method(cCSchemaView, "type", schema_view_type, 0);
    rb_define_method(cCSchemaView, "nullable", schema_view_nullable, 0);
    rb_define_method(cCSchemaView, "fixed_size", schema_view_fixed_size, 0);
    rb_define_method(cCSchemaView, "decimal_precision", schema_view_decimal_precision, 0);
    rb_define_method(cCSchemaView, "decimal_scale", schema_view_decimal_scale, 0);
    rb_define_method(cCSchemaView, "time_unit", schema_view_time_unit, 0);
    rb_define_method(cCSchemaView, "timezone", schema_view_timezone, 0);
}
