#include <ruby/ruby.h>

#include "ext.h"
#include "nanoarrow/nanoarrow.h"

VALUE cCapsule;

struct capsule {
    void* ptr;
    const char* name;
    void (*destructor)(capsule_t*);
};

static void capsule_free(void* ptr)
{
    capsule_t* capsule = (capsule_t*) ptr;

    if (capsule->destructor)
        capsule->destructor(capsule);

    xfree(ptr);
}

const rb_data_type_t capsule_data_type = {
    .wrap_struct_name = "Nanoarrow::Capsule",
    .function = {
        .dfree = capsule_free,
    },
    .flags = RUBY_TYPED_FREE_IMMEDIATELY
};

VALUE capsule_new(void* ptr, const char* name, void (*destructor)(capsule_t*))
{
    capsule_t* capsule;
    VALUE obj = TypedData_Make_Struct(cCapsule, capsule_t, &capsule_data_type, capsule);
    capsule->ptr = ptr;
    capsule->name = name;
    capsule->destructor = destructor;
    return obj;
}

void* capsule_pointer(capsule_t* capsule, const char* name)
{
    bool matches;
    if (name == NULL || capsule->name == NULL)
        matches = name == capsule->name;
    else
        matches = strcmp(name, capsule->name) == 0;

    // cannot raise since may be called in dfree function
    if (!matches)
        return NULL;

    return capsule->ptr;
}

static VALUE capsule_allocate(VALUE klass)
{
    capsule_t* capsule;
    VALUE obj = TypedData_Make_Struct(klass, capsule_t, &capsule_data_type, capsule);
    capsule->ptr = NULL;
    capsule->name = NULL;
    capsule->destructor = NULL;
    return obj;
}

static VALUE capsule_to_i(VALUE self)
{
    capsule_t* capsule;
    GetCapsule(self, capsule);

    return ULL2NUM((uintptr_t) capsule->ptr);
}

static VALUE capsule_name(VALUE self)
{
    capsule_t* capsule;
    GetCapsule(self, capsule);

    return rb_utf8_str_new_cstr(capsule->name);
}

void Init_capsule(void)
{
    cCapsule = rb_define_class_under(mNanoarrow, "Capsule", rb_cObject);
    rb_define_alloc_func(cCapsule, capsule_allocate);
    rb_define_method(cCapsule, "to_i", capsule_to_i, 0);
    rb_define_method(cCapsule, "name", capsule_name, 0);
}
