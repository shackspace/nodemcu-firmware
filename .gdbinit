file app/.output/eagle/debug/image/eagle.app.v6.out
#set debug remote 1
# set a serial line BREAK to remote when typing CTRL-c on gdb prompt.
set remote interrupt-sequence BREAK
#set remote interrupt-on-connect 1
target extended-remote localhost:3333
add-symbol-file ../esp-elf-rom/bootrom.elf 0x40000000
