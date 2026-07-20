#include <ruby/ruby.h>

#include "ext.h"
#include "nanoarrow/nanoarrow.h"

VALUE cCSchemaBuilder;

typedef struct schema_builder {
    VALUE c_schema;
    struct ArrowSchema* ptr;
} schema_builder_t;

static void schema_builder_mark(void* ptr)
{
    schema_builder_t* builder = (schema_builder_t*) ptr;
    rb_gc_mark_movable(builder->c_schema);
}

const rb_data_type_t schema_builder_data_type = {
    .wrap_struct_name = "Nanoarrow::CSchemaBuilder",
    .function = {
        .dmark = schema_builder_mark,
        .dfree = RUBY_TYPED_DEFAULT_FREE,
    },
    .flags = RUBY_TYPED_FREE_IMMEDIATELY
};

VALUE schema_builder_allocate(VALUE klass)
{
    schema_builder_t* builder;
    VALUE obj = TypedData_Make_Struct(klass, schema_builder_t, &schema_builder_data_type, builder);

    schema_t* schema_ptr;
    VALUE schema = rb_funcall(cCSchema, rb_intern("allocate"), 0);
    GetSchema(schema, schema_ptr);

    builder->c_schema = schema;
    builder->ptr = schema_ptr->ptr;
    if (builder->ptr->release == NULL)
        ArrowSchemaInit(builder->ptr);

    return obj;
}

static int foreach_metadata(st_data_t key_data, st_data_t value_data, st_data_t buffer_data)
{
    VALUE k = (VALUE) key_data;
    VALUE v = (VALUE) value_data;
    buffer_t* buffer;
    GetBuffer((VALUE) buffer_data, buffer);

    struct ArrowStringView key;
    struct ArrowStringView value;

    Check_Type(k, T_STRING);
    key.data = StringValuePtr(k);
    key.size_bytes = RSTRING_LEN(k);

    Check_Type(v, T_STRING);
    value.data = StringValuePtr(v);
    value.size_bytes = RSTRING_LEN(v);

    int code = ArrowMetadataBuilderAppend(buffer->ptr, key, value);
    raise_error_not_ok("ArrowMetadataBuilderAppend()", code);

    return ST_CONTINUE;
}

static VALUE schema_builder_append_metadata(VALUE self, VALUE metadata)
{
    schema_builder_t* builder;
    GetSchemaBuilder(self, builder);

    Check_Type(metadata, T_HASH);

    if (RHASH_SIZE(metadata) > 0)
    {
        VALUE buffer = rb_funcall(cCBuffer, rb_intern("empty"), 0);
        buffer_t* buffer_ptr;
        GetBuffer(buffer, buffer_ptr);

        schema_t* c_schema;
        GetSchema(builder->c_schema, c_schema);

        const char* existing_metadata = c_schema->ptr->metadata;
        int code = ArrowMetadataBuilderInit(buffer_ptr->ptr, existing_metadata);
        raise_error_not_ok("ArrowMetadataBuilderInit()", code);

        rb_hash_foreach(metadata, foreach_metadata, buffer);

        code = ArrowSchemaSetMetadata(c_schema->ptr, (const char*) buffer_ptr->ptr->data);
        raise_error_not_ok("ArrowSchemaSetMetadata()", code);
    }

    return self;
}

static VALUE schema_builder_set_type(VALUE self, VALUE type_id)
{
    schema_builder_t* builder;
    GetSchemaBuilder(self, builder);

    schema_assert_valid(builder->c_schema);

    int code = ArrowSchemaSetType(builder->ptr, NUM2INT(type_id));
    raise_error_not_ok("ArrowSchemaSetType()", code);

    return self;
}

static VALUE schema_builder_set_type_decimal(VALUE self, VALUE type_id, VALUE precision, VALUE scale)
{
    schema_builder_t* builder;
    GetSchemaBuilder(self, builder);

    schema_assert_valid(builder->c_schema);

    int code = ArrowSchemaSetTypeDecimal(builder->ptr, NUM2INT(type_id), NUM2INT(precision), NUM2INT(scale));
    raise_error_not_ok("ArrowSchemaSetTypeDecimal()", code);

    return self;
}

static VALUE schema_builder_set_type_fixed_size(VALUE self, VALUE type_id, VALUE fixed_size)
{
    schema_builder_t* builder;
    GetSchemaBuilder(self, builder);

    schema_assert_valid(builder->c_schema);

    int code = ArrowSchemaSetTypeFixedSize(builder->ptr, NUM2INT(type_id), NUM2INT(fixed_size));
    raise_error_not_ok("ArrowSchemaSetTypeFixedSize()", code);

    return self;
}

static VALUE schema_builder_set_type_date_time(VALUE self, VALUE type_id, VALUE time_unit, VALUE timezone)
{
    schema_builder_t* builder;
    GetSchemaBuilder(self, builder);

    schema_assert_valid(builder->c_schema);

    int code;
    if (NIL_P(timezone))
        code = ArrowSchemaSetTypeDateTime(builder->ptr, NUM2INT(type_id), NUM2INT(time_unit), NULL);
    else
        code = ArrowSchemaSetTypeDateTime(builder->ptr, NUM2INT(type_id), NUM2INT(time_unit), StringValueCStr(timezone));

    raise_error_not_ok("ArrowSchemaSetTypeDateTime()", code);

    return self;
}

static VALUE schema_builder_set_format(VALUE self, VALUE format)
{
    schema_builder_t* builder;
    GetSchemaBuilder(self, builder);

    schema_assert_valid(builder->c_schema);

    int code = ArrowSchemaSetFormat(builder->ptr, StringValueCStr(format));
    raise_error_not_ok("ArrowSchemaSetFormat()", code);

    return self;
}

static VALUE schema_builder_set_name(VALUE self, VALUE name)
{
    schema_builder_t* builder;
    GetSchemaBuilder(self, builder);

    schema_assert_valid(builder->c_schema);

    int code;
    if (NIL_P(name))
        code = ArrowSchemaSetName(builder->ptr, NULL);
    else
        code = ArrowSchemaSetName(builder->ptr, StringValueCStr(name));

    raise_error_not_ok("ArrowSchemaSetName()", code);

    return self;
}

static VALUE schema_builder_allocate_children(VALUE self, VALUE n)
{
    schema_builder_t* builder;
    GetSchemaBuilder(self, builder);

    schema_assert_valid(builder->c_schema);

    int code = ArrowSchemaAllocateChildren(builder->ptr, NUM2LL(n));
    raise_error_not_ok("ArrowSchemaAllocateChildren()", code);

    return self;
}

static VALUE schema_builder_set_child(VALUE self, VALUE idx, VALUE name, VALUE child_src)
{
    schema_builder_t* builder;
    GetSchemaBuilder(self, builder);

    schema_assert_valid(builder->c_schema);

    int64_t i = NUM2LL(idx);
    if (i < 0 || i >= builder->ptr->n_children)
        rb_raise(rb_eIndexError, "Index out of range");

    if (builder->ptr->children[i]->release != NULL)
        ArrowSchemaRelease(builder->ptr->children[i]);

    schema_t* child_src_ptr;
    GetSchema(child_src, child_src_ptr);

    int code = ArrowSchemaDeepCopy(child_src_ptr->ptr, builder->ptr->children[i]);
    raise_error_not_ok("ArrowSchemaDeepCopy()", code);

    if (!NIL_P(name))
    {
        Check_Type(name, T_STRING);
        code = ArrowSchemaSetName(builder->ptr->children[i], StringValueCStr(name));
        raise_error_not_ok("ArrowSchemaSetName()", code);
    }

    return self;
}

static VALUE schema_builder_set_flags(VALUE self, VALUE flags)
{
    schema_builder_t* builder;
    GetSchemaBuilder(self, builder);

    builder->ptr->flags = NUM2LL(flags);
    return self;
}

static VALUE schema_builder_set_nullable(VALUE self, VALUE nullable)
{
    schema_builder_t* builder;
    GetSchemaBuilder(self, builder);

    if (RTEST(nullable))
        builder->ptr->flags |= ARROW_FLAG_NULLABLE;
    else
        builder->ptr->flags &= ~ARROW_FLAG_NULLABLE;

    return self;
}

static VALUE schema_builder_set_map_keys_sorted(VALUE self, VALUE map_keys_sorted)
{
    schema_builder_t* builder;
    GetSchemaBuilder(self, builder);

    if (RTEST(map_keys_sorted))
        builder->ptr->flags |= ARROW_FLAG_MAP_KEYS_SORTED;
    else
        builder->ptr->flags &= ~ARROW_FLAG_MAP_KEYS_SORTED;

    return self;
}

static VALUE schema_builder_validate(VALUE self)
{
    schema_builder_t* builder;
    GetSchemaBuilder(self, builder);

    return rb_funcall(cCSchemaView, rb_intern("new"), 1, builder->c_schema);
}

static VALUE schema_builder_finish(VALUE self)
{
    schema_builder_t* builder;
    GetSchemaBuilder(self, builder);

    schema_assert_valid(builder->c_schema);

    VALUE out = rb_funcall(cCSchema, rb_intern("allocate"), 0);

    schema_t* c_schema_ptr;
    GetSchema(builder->c_schema, c_schema_ptr);

    schema_t* out_ptr;
    GetSchema(out, out_ptr);

    ArrowSchemaMove(c_schema_ptr->ptr, out_ptr->ptr);
    ArrowSchemaInit(c_schema_ptr->ptr);
    return out;
}

void Init_schema_builder(void)
{
    cCSchemaBuilder = rb_define_class_under(mNanoarrow, "CSchemaBuilder", rb_cObject);
    rb_define_alloc_func(cCSchemaBuilder, schema_builder_allocate);
    rb_define_method(cCSchemaBuilder, "append_metadata", schema_builder_append_metadata, 1);
    rb_define_method(cCSchemaBuilder, "set_type", schema_builder_set_type, 1);
    rb_define_method(cCSchemaBuilder, "set_type_decimal", schema_builder_set_type_decimal, 3);
    rb_define_method(cCSchemaBuilder, "set_type_fixed_size", schema_builder_set_type_fixed_size, 2);
    rb_define_method(cCSchemaBuilder, "set_type_date_time", schema_builder_set_type_date_time, 3);
    rb_define_method(cCSchemaBuilder, "set_format", schema_builder_set_format, 1);
    rb_define_method(cCSchemaBuilder, "set_name", schema_builder_set_name, 1);
    rb_define_method(cCSchemaBuilder, "allocate_children", schema_builder_allocate_children, 1);
    rb_define_method(cCSchemaBuilder, "set_child", schema_builder_set_child, 3);
    rb_define_method(cCSchemaBuilder, "set_flags", schema_builder_set_flags, 1);
    rb_define_method(cCSchemaBuilder, "set_nullable", schema_builder_set_nullable, 1);
    rb_define_method(cCSchemaBuilder, "set_map_keys_sorted", schema_builder_set_map_keys_sorted, 1);
    rb_define_method(cCSchemaBuilder, "validate", schema_builder_validate, 0);
    rb_define_method(cCSchemaBuilder, "finish", schema_builder_finish, 0);
}
