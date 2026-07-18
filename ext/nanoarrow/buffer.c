#include <ruby/ruby.h>

#include "ext.h"
#include "nanoarrow/nanoarrow.h"

VALUE cCBuffer;

static void buffer_mark(void* ptr)
{
    buffer_t* buffer = (buffer_t*) ptr;
    rb_gc_mark_movable(buffer->base);
}

const rb_data_type_t buffer_data_type = {
    .wrap_struct_name = "Nanoarrow::CArrayView",
    .function = {
        .dmark = buffer_mark,
        .dfree = RUBY_TYPED_DEFAULT_FREE,
    },
    .flags = RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE buffer_allocate(VALUE klass)
{
    buffer_t* buffer;
    VALUE obj = TypedData_Make_Struct(klass, buffer_t, &buffer_data_type, buffer);
    buffer->base = Qnil;
    buffer->ptr = NULL;
    return obj;
}

static VALUE buffer_empty(VALUE klass)
{
    buffer_t* buffer;
    VALUE out = buffer_allocate(klass);
    GetBuffer(out, buffer);
    buffer->base = alloc_c_buffer(&buffer->ptr);
    return out;
}

void Init_buffer(void)
{
    cCBuffer = rb_define_class_under(mNanoarrow, "CBuffer", rb_cObject);
    rb_define_alloc_func(cCBuffer, buffer_allocate);
    rb_define_singleton_method(cCBuffer, "empty", buffer_empty, 0);
}
