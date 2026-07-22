#include <ruby/ruby.h>

#include "ext.h"
#include "nanoarrow/nanoarrow.h"

VALUE cCSchema;

static void schema_mark(void* ptr)
{
    schema_t* schema = (schema_t*) ptr;
    rb_gc_mark_movable(schema->base);
}

const rb_data_type_t schema_data_type = {
    .wrap_struct_name = "Nanoarrow::CSchema",
    .function = {
        .dmark = schema_mark,
        .dfree = RUBY_TYPED_DEFAULT_FREE,
    },
    .flags = RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE schema_allocate(VALUE klass)
{
    schema_t* schema;
    VALUE obj = TypedData_Make_Struct(klass, schema_t, &schema_data_type, schema);
    schema->base = alloc_c_schema(&schema->ptr);
    return obj;
}

static VALUE schema_import_from_c_capsule(VALUE klass, VALUE schema_capsule)
{
    schema_t* schema;
    VALUE obj = TypedData_Make_Struct(klass, schema_t, &schema_data_type, schema);
    schema->base = schema_capsule;
    schema->ptr = (struct ArrowSchema*) NUM2ULL(rb_funcall(schema_capsule, rb_intern("to_i"), 0));
    return obj;
}

static VALUE schema_deep_dup(VALUE self)
{
    schema_t* schema;
    GetSchema(self, schema);

    schema_t* out_ptr;
    VALUE out = schema_allocate(cCSchema);
    GetSchema(out, out_ptr);

    int code = ArrowSchemaDeepCopy(schema->ptr, out_ptr->ptr);
    raise_error_not_ok("ArrowSchemaDeepCopy()", code);
    return out;
}

VALUE schema_assert_valid(VALUE self)
{
    schema_t* schema;
    GetSchema(self, schema);

    if (schema->ptr == NULL)
        rb_raise(rb_eRuntimeError, "schema is NULL");
    if (schema->ptr->release == NULL)
        rb_raise(rb_eRuntimeError, "schema is released");

    return Qnil;
}

static VALUE schema_type_equals(VALUE self, VALUE other, VALUE check_nullability)
{
    schema_t* self_ptr;
    GetSchema(self, self_ptr);

    schema_assert_valid(self);

    schema_t* other_ptr;
    GetSchema(other, other_ptr);

    if (self_ptr->ptr == other_ptr->ptr)
        return Qtrue;

    // format

    VALUE format = rb_funcall(self, rb_intern("format"), 0);
    VALUE other_format = rb_funcall(other, rb_intern("format"), 0);

    if (rb_equal(format, other_format) != Qtrue)
        return Qfalse;

    // flags

    int64_t flags = NUM2LL(rb_funcall(self, rb_intern("flags"), 0));
    int64_t other_flags = NUM2LL(rb_funcall(other, rb_intern("flags"), 0));

    if (!RTEST(check_nullability))
    {
        flags &= ~ARROW_FLAG_NULLABLE;
        other_flags &= ~ARROW_FLAG_NULLABLE;
    }

    if (flags != other_flags)
        return Qfalse;

    // children

    int64_t n_children = NUM2LL(rb_funcall(self, rb_intern("n_children"), 0));
    int64_t other_n_children = NUM2LL(rb_funcall(other, rb_intern("n_children"), 0));

    if (n_children != other_n_children)
        return Qfalse;

    for (int64_t i = 0; i < n_children; i++)
    {
        VALUE child = rb_funcall(self, rb_intern("child"), 1, LL2NUM(i));
        VALUE other_child = rb_funcall(other, rb_intern("child"), 1, LL2NUM(i));

        if (schema_type_equals(child, other_child, check_nullability) != Qtrue)
            return Qfalse;
    }

    // dictionary

    VALUE dictionary = rb_funcall(self, rb_intern("dictionary"), 0);
    VALUE other_dictionary = rb_funcall(other, rb_intern("dictionary"), 0);

    if (NIL_P(dictionary) != NIL_P(other_dictionary))
        return Qfalse;

    if (!NIL_P(dictionary))
    {
        if (schema_type_equals(dictionary, other_dictionary, check_nullability) != Qtrue)
            return Qfalse;
    }

    return Qtrue;
}

void assert_type_equal(VALUE actual, VALUE expected, VALUE check_nullability)
{
    if (!rb_obj_is_instance_of(actual, cCSchema))
        rb_raise(rb_eTypeError, "actual is not CSchema");

    if (!rb_obj_is_instance_of(expected, cCSchema))
        rb_raise(rb_eTypeError, "actual is not CSchema");

    if (!RTEST(schema_type_equals(actual, expected, check_nullability)))
        rb_raise(rb_eArgError, "schema mismatch");
}

static VALUE schema_format(VALUE self)
{
    schema_t* schema;
    GetSchema(self, schema);

    schema_assert_valid(self);

    return schema->ptr->format != NULL ? rb_utf8_str_new_cstr(schema->ptr->format) : Qnil;
}

static VALUE schema_name(VALUE self)
{
    schema_t* schema;
    GetSchema(self, schema);

    schema_assert_valid(self);

    return schema->ptr->name != NULL ? rb_utf8_str_new_cstr(schema->ptr->name) : Qnil;
}

static VALUE schema_flags(VALUE self)
{
    schema_t* schema;
    GetSchema(self, schema);

    return LL2NUM(schema->ptr->flags);
}

static VALUE schema_metadata(VALUE self)
{
    schema_t* schema;
    GetSchema(self, schema);

    schema_assert_valid(self);

    if (schema->ptr->metadata == NULL)
        return rb_hash_new();

    struct ArrowMetadataReader reader;
    int code = ArrowMetadataReaderInit(&reader, schema->ptr->metadata);
    raise_error_not_ok("ArrowMetadataReaderInit()", code);

    VALUE items = rb_hash_new_capa(reader.remaining_keys);
    while (reader.remaining_keys > 0)
    {
        struct ArrowStringView key;
        struct ArrowStringView value;
        code = ArrowMetadataReaderRead(&reader, &key, &value);
        raise_error_not_ok("ArrowMetadataReaderRead()", code);
        rb_hash_aset(items, rb_str_new(key.data, key.size_bytes), rb_str_new(value.data, value.size_bytes));
    }
    return items;
}

static VALUE schema_n_children(VALUE self)
{
    schema_t* schema;
    GetSchema(self, schema);

    schema_assert_valid(self);

    return LL2NUM(schema->ptr->n_children);
}

static VALUE schema_child(VALUE self, VALUE rb_i)
{
    schema_t* schema;
    GetSchema(self, schema);

    schema_assert_valid(self);

    int64_t i = NUM2LL(rb_i);
    if (i < 0 || i >= schema->ptr->n_children)
        rb_raise(rb_eIndexError, "out of range");

    schema_t* schema_ptr;
    VALUE obj = TypedData_Make_Struct(cCSchema, schema_t, &schema_data_type, schema_ptr);
    schema_ptr->base = schema->base;
    schema_ptr->ptr = schema->ptr->children[i];
    return obj;
}

static VALUE schema_children(VALUE self)
{
    int64_t n_children = NUM2LL(schema_n_children(self));
    VALUE children = rb_ary_new();
    for (int64_t i = 0; i < n_children; i++)
        rb_ary_push(children, schema_child(self, LL2NUM(i)));
    return children;
}

static VALUE schema_dictionary(VALUE self)
{
    schema_t* schema;
    GetSchema(self, schema);

    schema_assert_valid(self);

    if (schema->ptr->dictionary != NULL)
        raise_todo();

    return Qnil;
}

static VALUE schema_to_s(VALUE self)
{
    schema_t* schema;
    GetSchema(self, schema);

    int64_t n_chars = 1024;
    bool recursive = true;

    char* out = ArrowMalloc(n_chars + 1);
    if (out == NULL)
        rb_raise(rb_eNoMemError, "out of memory");

    int64_t len = ArrowSchemaToString(schema->ptr, out, n_chars + 1, recursive);
    VALUE out_str = rb_str_new(out, len);
    ArrowFree(out);
    return out_str;
}

static VALUE schema_arrow_c_schema(VALUE self)
{
    schema_t* schema;
    GetSchema(self, schema);

    schema_assert_valid(self);

    struct ArrowSchema* c_schema_out;
    VALUE schema_capsule = alloc_c_schema(&c_schema_out);

    int code = ArrowSchemaDeepCopy(schema->ptr, c_schema_out);
    raise_error_not_ok("ArrowSchemaDeepCopy()", code);
    return schema_capsule;
}

static VALUE schema_modify(int argc, VALUE* argv, VALUE self)
{
    VALUE kwargs;
    rb_scan_args(argc, argv, "0:", &kwargs);

    ID keywords[8];
    VALUE values[8];
    keywords[0] = rb_intern("format");
    keywords[1] = rb_intern("name");
    keywords[2] = rb_intern("flags");
    keywords[3] = rb_intern("nullable");
    keywords[4] = rb_intern("metadata");
    keywords[5] = rb_intern("children");
    keywords[6] = rb_intern("dictionary");
    keywords[7] = rb_intern("validate");
    rb_get_kwargs(kwargs, keywords, 0, 8, values);

    for (int i = 0; i < 8; i++)
    {
        if (values[i] == Qundef)
            values[i] = Qnil;
    }

    VALUE format = values[0];
    VALUE name = values[1];
    VALUE flags = values[2];
    VALUE nullable = values[3];
    VALUE metadata = values[4];
    VALUE children = values[5];
    VALUE dictionary = values[6];
    VALUE validate = values[7];

    if (NIL_P(validate))
        validate = Qtrue;

    VALUE builder = rb_funcall(cCSchemaBuilder, rb_intern("allocate"), 0);

    if (NIL_P(format))
        rb_funcall(builder, rb_intern("set_format"), 1, rb_funcall(self, rb_intern("format"), 0));
    else
        rb_funcall(builder, rb_intern("set_format"), 1, format);

    if (NIL_P(name))
        rb_funcall(builder, rb_intern("set_name"), 1, rb_funcall(self, rb_intern("name"), 0));
    else
        rb_funcall(builder, rb_intern("set_name"), 1, name);

    if (NIL_P(flags))
        rb_funcall(builder, rb_intern("set_flags"), 1, rb_funcall(self, rb_intern("flags"), 0));
    else
        rb_funcall(builder, rb_intern("set_flags"), 1, flags);

    if (!NIL_P(nullable))
        rb_funcall(builder, rb_intern("set_nullable"), 1, nullable);

    if (NIL_P(metadata))
    {
        VALUE self_metadata = rb_funcall(self, rb_intern("metadata"), 0);
        if (!NIL_P(self_metadata))
            rb_funcall(builder, rb_intern("append_metadata"), 1, self_metadata);
    }
    else
        rb_funcall(builder, rb_intern("append_metadata"), 1, metadata);

    if (NIL_P(children))
    {
        int64_t n_children = NUM2LL(rb_funcall(self, rb_intern("n_children"), 0));
        if (n_children > 0)
        {
            rb_funcall(builder, rb_intern("allocate_children"), 1, LL2NUM(n_children));
            VALUE self_children = rb_funcall(self, rb_intern("children"), 0);
            for (long i = 0; i < RARRAY_LEN(self_children); i++)
                rb_funcall(builder, rb_intern("set_child"), 3, LONG2NUM(i), Qnil, rb_ary_entry(self_children, i));
        }
    }
    else if (RB_TYPE_P(children, T_HASH))
    {
        // not as efficient as rb_hash_foreach, but simpler
        VALUE keys = rb_funcall(children, rb_intern("keys"), 0);
        Check_Type(keys, T_ARRAY);
        rb_funcall(builder, rb_intern("allocate_children"), 1, LONG2NUM(RARRAY_LEN(keys)));
        for (long i = 0; i < RARRAY_LEN(keys); i++)
        {
            VALUE key = rb_ary_entry(keys, i);
            rb_funcall(builder, rb_intern("set_child"), 3, LONG2NUM(i), key, rb_hash_lookup(children, key));
        }
    }
    else
    {
        Check_Type(children, T_ARRAY);
        rb_funcall(builder, rb_intern("allocate_children"), 1, LONG2NUM(RARRAY_LEN(children)));
        for (long i = 0; i < RARRAY_LEN(children); i++)
            rb_funcall(builder, rb_intern("set_child"), 3, LONG2NUM(i), Qnil, rb_ary_entry(children, i));
    }

    if (NIL_P(dictionary))
    {
        if (!NIL_P(rb_funcall(self, rb_intern("dictionary"), 0)))
            raise_todo();
    }
    else
        raise_todo();

    if (RTEST(validate))
        rb_funcall(builder, rb_intern("validate"), 0);

    return rb_funcall(builder, rb_intern("finish"), 0);
}

void Init_schema(void)
{
    cCSchema = rb_define_class_under(mNanoarrow, "CSchema", rb_cObject);
    rb_define_alloc_func(cCSchema, schema_allocate);
    rb_define_singleton_method(cCSchema, "import_from_c_capsule", schema_import_from_c_capsule, 1);
    rb_define_method(cCSchema, "deep_dup", schema_deep_dup, 0);
    rb_define_method(cCSchema, "format", schema_format, 0);
    rb_define_method(cCSchema, "name", schema_name, 0);
    rb_define_method(cCSchema, "flags", schema_flags, 0);
    rb_define_method(cCSchema, "metadata", schema_metadata, 0);
    rb_define_method(cCSchema, "n_children", schema_n_children, 0);
    rb_define_method(cCSchema, "child", schema_child, 1);
    rb_define_method(cCSchema, "children", schema_children, 0);
    rb_define_method(cCSchema, "dictionary", schema_dictionary, 0);
    rb_define_method(cCSchema, "to_s", schema_to_s, 0);
    rb_define_method(cCSchema, "arrow_c_schema", schema_arrow_c_schema, 0);
    rb_define_method(cCSchema, "modify", schema_modify, -1);
    rb_define_method(cCSchema, "assert_valid", schema_assert_valid, 0);
}
