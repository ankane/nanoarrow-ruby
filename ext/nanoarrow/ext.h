#pragma once

#include <ruby/ruby.h>

#include "nanoarrow/nanoarrow.h"

#define GetArray(obj, ptr) TypedData_Get_Struct((obj), array_t, &array_data_type, (ptr));
#define GetArrayBuilder(obj, ptr) TypedData_Get_Struct((obj), array_builder_t, &array_builder_data_type, (ptr));
#define GetArrayStream(obj, ptr) TypedData_Get_Struct((obj), array_stream_t, &array_stream_data_type, (ptr));
#define GetArrayView(obj, ptr) TypedData_Get_Struct((obj), array_view_t, &array_view_data_type, (ptr));
#define GetBuffer(obj, ptr) TypedData_Get_Struct((obj), buffer_t, &buffer_data_type, (ptr));
#define GetCapsule(obj, ptr) TypedData_Get_Struct((obj), capsule_t, &capsule_data_type, (ptr));
#define GetMatArrayStream(obj, ptr) TypedData_Get_Struct((obj), mat_array_stream_t, &mat_array_stream_data_type, (ptr));
#define GetSchema(obj, ptr) TypedData_Get_Struct((obj), schema_t, &schema_data_type, (ptr));
#define GetSchemaBuilder(obj, ptr) TypedData_Get_Struct((obj), schema_builder_t, &schema_builder_data_type, (ptr));
#define GetSchemaView(obj, ptr) TypedData_Get_Struct((obj), schema_view_t, &schema_view_data_type, (ptr));

typedef struct array {
    VALUE base;
    struct ArrowArray* ptr;
    VALUE schema;
} array_t;

extern const rb_data_type_t array_data_type;

typedef struct buffer {
    VALUE base;
    struct ArrowBuffer* ptr;
} buffer_t;

extern const rb_data_type_t buffer_data_type;

typedef struct schema {
    VALUE base;
    struct ArrowSchema* ptr;
} schema_t;

extern const rb_data_type_t schema_data_type;

extern VALUE mNanoarrow;
extern VALUE cCArray;
extern VALUE cCArrayBuilder;
extern VALUE cCArrayStream;
extern VALUE cCArrayView;
extern VALUE cCBuffer;
extern VALUE cCapsule;
extern VALUE cCMatArrayStream;
extern VALUE cCSchema;
extern VALUE cCSchemaBuilder;
extern VALUE cCSchemaView;

void Init_array(void);
void Init_array_builder(void);
void Init_array_stream(void);
void Init_array_view(void);
void Init_buffer(void);
void Init_capsule(void);
void Init_materialized(void);
void Init_schema(void);
void Init_schema_builder(void);
void Init_schema_view(void);
void Init_types(void);

typedef struct capsule capsule_t;

extern const rb_data_type_t capsule_data_type;

VALUE capsule_new(void* ptr, const char* name, void (*destructor)(capsule_t*));
void* capsule_pointer(capsule_t* capsule, const char* name);

typedef struct rc rc_t;

rc_t* rc_new(void* ptr, void (*destructor)(void*));
void rc_increment(rc_t* rc);
void rc_decrement(rc_t* rc);

VALUE schema_assert_valid(VALUE self);
void assert_type_equal(VALUE actual, VALUE expected, VALUE check_nullability);

void raise_message_not_ok(struct ArrowError* error, const char* message, int code);
void raise_error_not_ok(const char* message, int code);
void raise_todo(void);

VALUE alloc_c_array(struct ArrowArray** c_array);
VALUE alloc_c_array_stream(struct ArrowArrayStream** c_stream);
VALUE alloc_c_array_view(struct ArrowArrayView** c_array_view);
VALUE alloc_c_buffer(struct ArrowBuffer** c_buffer);
VALUE alloc_c_schema(struct ArrowSchema** c_schema);
void c_array_shallow_copy(VALUE base, const struct ArrowArray* src, struct ArrowArray* dst);
