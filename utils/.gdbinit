directory ../intel-simd-learning/

tui new-layout debug {-horizontal src 1 asm 1} 2 cmd 1
layout debug

set disassembly-flavor intel

define hook-next
refresh
end

break main
r
