#include <ruby/ruby.h>

#include "ext.h"
#include "nanoarrow/nanoarrow.h"

void Init_types(void)
{
    VALUE mType = rb_define_module_under(mNanoarrow, "Type");
    rb_define_const(mType, "NULL", INT2NUM(NANOARROW_TYPE_NA));
    rb_define_const(mType, "BOOL", INT2NUM(NANOARROW_TYPE_BOOL));
    rb_define_const(mType, "INT8", INT2NUM(NANOARROW_TYPE_INT8));
    rb_define_const(mType, "UINT8", INT2NUM(NANOARROW_TYPE_UINT8));
    rb_define_const(mType, "INT16", INT2NUM(NANOARROW_TYPE_INT16));
    rb_define_const(mType, "UINT16", INT2NUM(NANOARROW_TYPE_UINT16));
    rb_define_const(mType, "INT32", INT2NUM(NANOARROW_TYPE_INT32));
    rb_define_const(mType, "UINT32", INT2NUM(NANOARROW_TYPE_UINT32));
    rb_define_const(mType, "INT64", INT2NUM(NANOARROW_TYPE_INT64));
    rb_define_const(mType, "UINT64", INT2NUM(NANOARROW_TYPE_UINT64));
    rb_define_const(mType, "HALF_FLOAT", INT2NUM(NANOARROW_TYPE_HALF_FLOAT));
    rb_define_const(mType, "FLOAT", INT2NUM(NANOARROW_TYPE_FLOAT));
    rb_define_const(mType, "DOUBLE", INT2NUM(NANOARROW_TYPE_DOUBLE));
    rb_define_const(mType, "STRING", INT2NUM(NANOARROW_TYPE_STRING));
    rb_define_const(mType, "LARGE_STRING", INT2NUM(NANOARROW_TYPE_LARGE_STRING));
    rb_define_const(mType, "STRING_VIEW", INT2NUM(NANOARROW_TYPE_STRING_VIEW));
    rb_define_const(mType, "BINARY", INT2NUM(NANOARROW_TYPE_BINARY));
    rb_define_const(mType, "LARGE_BINARY", INT2NUM(NANOARROW_TYPE_LARGE_BINARY));
    rb_define_const(mType, "BINARY_VIEW", INT2NUM(NANOARROW_TYPE_BINARY_VIEW));
    rb_define_const(mType, "FIXED_SIZE_BINARY", INT2NUM(NANOARROW_TYPE_FIXED_SIZE_BINARY));
    rb_define_const(mType, "DATE32", INT2NUM(NANOARROW_TYPE_DATE32));
    rb_define_const(mType, "DATE64", INT2NUM(NANOARROW_TYPE_DATE64));
    rb_define_const(mType, "TIME32", INT2NUM(NANOARROW_TYPE_TIME32));
    rb_define_const(mType, "TIME64", INT2NUM(NANOARROW_TYPE_TIME64));
    rb_define_const(mType, "TIMESTAMP", INT2NUM(NANOARROW_TYPE_TIMESTAMP));
    rb_define_const(mType, "DURATION", INT2NUM(NANOARROW_TYPE_DURATION));
    rb_define_const(mType, "INTERVAL_MONTHS", INT2NUM(NANOARROW_TYPE_INTERVAL_MONTHS));
    rb_define_const(mType, "INTERVAL_DAY_TIME", INT2NUM(NANOARROW_TYPE_INTERVAL_DAY_TIME));
    rb_define_const(mType, "INTERVAL_MONTH_DAY_NANO", INT2NUM(NANOARROW_TYPE_INTERVAL_MONTH_DAY_NANO));
    rb_define_const(mType, "DECIMAL128", INT2NUM(NANOARROW_TYPE_DECIMAL128));
    rb_define_const(mType, "DECIMAL256", INT2NUM(NANOARROW_TYPE_DECIMAL256));
    rb_define_const(mType, "STRUCT", INT2NUM(NANOARROW_TYPE_STRUCT));
    rb_define_const(mType, "LIST", INT2NUM(NANOARROW_TYPE_LIST));
    rb_define_const(mType, "LARGE_LIST", INT2NUM(NANOARROW_TYPE_LARGE_LIST));
    rb_define_const(mType, "FIXED_SIZE_LIST", INT2NUM(NANOARROW_TYPE_FIXED_SIZE_LIST));
    rb_define_const(mType, "MAP", INT2NUM(NANOARROW_TYPE_MAP));
    rb_define_const(mType, "DICTIONARY", INT2NUM(NANOARROW_TYPE_DICTIONARY));
    rb_define_const(mType, "SPARSE_UNION", INT2NUM(NANOARROW_TYPE_SPARSE_UNION));
    rb_define_const(mType, "DENSE_UNION", INT2NUM(NANOARROW_TYPE_DENSE_UNION));
}
