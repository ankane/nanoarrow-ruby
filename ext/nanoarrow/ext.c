#include <ruby/ruby.h>

#include "ext.h"
#include "nanoarrow/nanoarrow.h"

VALUE mNanoarrow;

void Init_ext(void)
{
    mNanoarrow = rb_define_module("Nanoarrow");

    Init_array();
    Init_array_builder();
    Init_array_stream();
    Init_array_view();
    Init_buffer();
    Init_capsule();
    Init_materialized();
    Init_schema();
    Init_schema_builder();
    Init_schema_view();
    Init_types();
}
