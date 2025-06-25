# Simple Column Wise Variable Logger

Tracks static variable pointers for a defined time and saves it to a file. Made in an hour or so.

This does not implement page buffering so currently the file io is fairly slow. 

For future improvements add a page size to the `VB_Block_Proxy` struct to buffer into and flush on full buffer. (Also flush on `vb_end`)

Only 1 file can be open per processes.

## How to download

```bash
git clone https://github.com/philipdavis82/azzex
```

## How to test

### Simple 

```bash
make 
./test_vb2
python3 py/vb2_reader.py
```

### Extra

```bash
make LENGTH=<record_length(int)>  EXTRA_VARS=<more_variables(int)>
./test_vbs
python3 py/vb2_reader.py
```

* Defaults:
* Length=100
* EXTRA_VARS=0


## How to add to another codebase:

Add `src/var_buffer_2.c` and `include/var_buffer_2.h` to your project and make file and build system.