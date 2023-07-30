directory ../src/
directory ../src/platform-linux/

tui new-layout debug {-horizontal src 1 asm 1} 2 cmd 1
layout debug

define hook-next
info args
info locals
end

break main
r
