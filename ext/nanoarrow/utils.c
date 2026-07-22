#include <ruby/ruby.h>

#include "ext.h"
#include "nanoarrow/nanoarrow.h"

void raise_todo(void)
{
    rb_raise(rb_const_get(mNanoarrow, rb_intern("Todo")), "not implemented yet");
}

void raise_message_not_ok(struct ArrowError* error, const char* message, int code)
{
    if (code != NANOARROW_OK)
        rb_raise(rb_const_get(mNanoarrow, rb_intern("Error")), "%s", error->message);
}

void raise_error_not_ok(const char* message, int code)
{
    if (code != NANOARROW_OK)
        rb_raise(rb_const_get(mNanoarrow, rb_intern("Error")), "%s", message);
}

void array_deleter(void* ptr)
{
    struct ArrowArray* array = (struct ArrowArray*) ptr;

    if (array->release != NULL)
        ArrowArrayRelease(array);

    ArrowFree(array);
}

void rbcapsule_array_deleter(capsule_t* capsule)
{
    rc_t* rc = capsule_pointer(capsule, "arrow_array");

    if (rc == NULL)
        return;

    rc_decrement(rc);
}

VALUE alloc_c_array(struct ArrowArray** c_array)
{
    *c_array = ArrowMalloc(sizeof(struct ArrowArray));
    (*c_array)->release = NULL;
    rc_t* rc = rc_new(*c_array, array_deleter);
    return capsule_new(rc, "arrow_array", rbcapsule_array_deleter);
}

void rbcapsule_array_stream_deleter(capsule_t* capsule)
{
    struct ArrowArrayStream* stream = capsule_pointer(capsule, "arrow_array_stream");

    if (stream == NULL)
        return;

    if (stream->release != NULL)
        ArrowArrayStreamRelease(stream);

    ArrowFree(stream);
}

VALUE alloc_c_array_stream(struct ArrowArrayStream** c_stream)
{
    *c_stream = ArrowMalloc(sizeof(struct ArrowArrayStream));
    (*c_stream)->release = NULL;
    return capsule_new(*c_stream, "arrow_array_stream", rbcapsule_array_stream_deleter);
}

void rbcapsule_array_view_deleter(capsule_t* capsule)
{
    struct ArrowArrayView* array_view = capsule_pointer(capsule, "nanoarrow_array_view");

    if (array_view == NULL)
        return;

    ArrowArrayViewReset(array_view);

    ArrowFree(array_view);
}

VALUE alloc_c_array_view(struct ArrowArrayView** c_array_view)
{
    *c_array_view = ArrowMalloc(sizeof(struct ArrowArrayView));
    ArrowArrayViewInitFromType(*c_array_view, NANOARROW_TYPE_UNINITIALIZED);
    return capsule_new(*c_array_view, "nanoarrow_array_view", rbcapsule_array_view_deleter);
}

void rbcapsule_buffer_deleter(capsule_t* capsule)
{
    struct ArrowBuffer* buffer = capsule_pointer(capsule, "nanoarrow_buffer");

    if (buffer == NULL)
        return;

    ArrowBufferReset(buffer);
    ArrowFree(buffer);
}

VALUE alloc_c_buffer(struct ArrowBuffer** c_buffer)
{
    *c_buffer = ArrowMalloc(sizeof(struct ArrowBuffer));
    ArrowBufferInit(*c_buffer);
    return capsule_new(*c_buffer, "nanoarrow_buffer", rbcapsule_buffer_deleter);
}

void rbcapsule_schema_deleter(capsule_t* capsule)
{
    struct ArrowSchema* schema = capsule_pointer(capsule, "arrow_schema");

    if (schema == NULL)
        return;

    if (schema->release != NULL)
        ArrowSchemaRelease(schema);

    ArrowFree(schema);
}

VALUE alloc_c_schema(struct ArrowSchema** c_schema)
{
    *c_schema = ArrowMalloc(sizeof(struct ArrowSchema));
    (*c_schema)->release = NULL;
    return capsule_new(*c_schema, "arrow_schema", rbcapsule_schema_deleter);
}

void c_deallocate_rbobject_buffer(struct ArrowBufferAllocator* allocator, uint8_t* ptr, int64_t size)
{
    rc_t* rc = allocator->private_data;
    rc_decrement(rc);
}

void c_rbobject_buffer(VALUE base, const void* buf, int64_t size_bytes, struct ArrowBuffer* out)
{
    out->data = (void*) buf;
    out->size_bytes = size_bytes;

    capsule_t* capsule;
    GetCapsule(base, capsule);
    rc_t* rc = capsule_pointer(capsule, "arrow_array");

    if (rc == NULL)
        rb_raise(rb_eRuntimeError, "cannot open capsule");

    rc_increment(rc);

    // since buffers can be moved by receiver (non-Ruby code), need to reference count
    // and only destroy C array when no more references (this can be long after Ruby object is GCed)
    out->allocator = ArrowBufferDeallocator(c_deallocate_rbobject_buffer, rc);
}

void c_array_shallow_copy(VALUE base, const struct ArrowArray* src, struct ArrowArray* dst)
{
    struct ArrowArray* tmp;
    VALUE shelter = alloc_c_array(&tmp);
    int code;

    code = ArrowArrayInitFromType(tmp, NANOARROW_TYPE_UNINITIALIZED);
    raise_error_not_ok("ArrowArrayInitFromType()", code);

    tmp->length = src->length;
    tmp->offset = src->offset;
    tmp->null_count = src->null_count;

    if (src->n_buffers > 3)
    {
        code = ArrowArrayAddVariadicBuffers(dst, src->n_buffers - 3);
        raise_error_not_ok("ArrowArrayAddVariadicBuffers()", code);
    }

    for (int64_t i = 0; i < src->n_buffers; i++)
    {
        if (src->buffers[i] != NULL)
            c_rbobject_buffer(base, src->buffers[i], 0, ArrowArrayBuffer(tmp, i));

        tmp->buffers[i] = src->buffers[i];
    }

    tmp->n_buffers = src->n_buffers;

    if (src->n_children > 0)
    {
        code = ArrowArrayAllocateChildren(tmp, src->n_children);
        raise_error_not_ok("ArrowArrayAllocateChildren()", code);

        for (int64_t i = 0; i < src->n_children; i++)
            c_array_shallow_copy(base, src->children[i], tmp->children[i]);
    }

    if (src->dictionary != NULL)
    {
        code = ArrowArrayAllocateDictionary(tmp);
        raise_error_not_ok("ArrowArrayAllocateDictionary()", code);

        c_array_shallow_copy(base, src->dictionary, tmp->dictionary);
    }

    ArrowArrayMove(tmp, dst);

    RB_GC_GUARD(shelter);
}
