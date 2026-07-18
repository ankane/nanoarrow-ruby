require "mkmf"

append_cflags("-Wall -Wextra -Wpedantic")
append_cflags("-Wno-suggest-attribute=format")
append_cflags("-Wno-suggest-attribute=noreturn")

create_makefile("nanoarrow/ext")
