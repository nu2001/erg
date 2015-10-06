define reload
kill
monitor jtag_reset
load
run
end

target extended localhost:4242
