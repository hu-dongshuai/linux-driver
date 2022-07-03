out_file_name = "framebufer"

all: framebuffer.c oled_code_table.c
	arm-linux-gcc $^ -g -o $(out_file_name)


.PHONY: clean
clean:
	rm $(out_file_name)

	
