import ctypes,sys, mmap

STR_TYPE_TO_CTYPE = {
    'long': ctypes.c_long,
    'int': ctypes.c_int,
    'float': ctypes.c_float,
    'double': ctypes.c_double,
}

class VB2MasterHeader(ctypes.Structure):
    _fields_ = [
        ('magic', ctypes.c_char * 4),
        ('version', ctypes.c_size_t),
        ('block_count', ctypes.c_size_t),
        ('max_history', ctypes.c_size_t),
        ('reserved', ctypes.c_size_t * 3)
    ]
    def __str__(self):
        return f"VB2MasterHeader(magic={self.magic.decode('utf-8')}, version={self.version}, block_count={self.block_count}, max_history={self.max_history})"

class VB2Header(ctypes.Structure):
    _fields_ = [
        ('name', ctypes.c_char * 256),
        ('unit', ctypes.c_char * 32),
        ('description', ctypes.c_char * 512),
        ('type', ctypes.c_char * 16),
        ('offset', ctypes.c_size_t),
        ('count', ctypes.c_size_t)
    ]
    def __str__(self):
        return (f"VB2Header(name={self.name.decode('utf-8').strip()}, "
                f"unit={self.unit.decode('utf-8').strip()}, "
                f"description={self.description.decode('utf-8').strip()}, "
                f"type={self.type.decode('utf-8').strip()}, "
                f"offset={self.offset}, count={self.count})")

class VB2Reader:
    def __init__(self, filename):
        self.filename = filename
        self.file = None
        self.mmap_obj = None
        self.master_header = None
        self.vars = {}

    def open(self):
        self.file = open(self.filename, 'rb')
        # Check windows or unix style
        if sys.platform.startswith('win'):
            self.mmap_obj = mmap.mmap(self.file.fileno(), 0, access=mmap.ACCESS_READ)
        else:
            self.mmap_obj = mmap.mmap(self.file.fileno(), 0, flags=mmap.ACCESS_READ, prot=mmap.PROT_READ)
        self.master_header = VB2MasterHeader.from_buffer_copy(self.mmap_obj, 0)
        print(self.master_header)
        self.get_all_vars()

    def close(self):
        if self.mmap_obj:
            self.mmap_obj.close()
        if self.file:
            self.file.close()

    def get_all_vars(self):
        for i in range(self.master_header.block_count):
            offset = ctypes.sizeof(VB2MasterHeader) + i * ctypes.sizeof(VB2Header)
            header = VB2Header.from_buffer_copy(self.mmap_obj, offset)
            print(header)
            self.vars[header.name.decode('utf-8').strip('\x00')] = header
    
    def __getitem__(self, key):
        if key in self.vars:
            header = self.vars[key]
            var_type = header.type.decode('utf-8').strip('\x00')
            ctype = STR_TYPE_TO_CTYPE[var_type]
            data_offset = header.offset
            data = (ctype * header.count).from_buffer_copy(self.mmap_obj, data_offset)
            return data
        else:
            raise KeyError(f"Variable '{key}' not found in the VB2 file.")

if __name__ == "__main__":
    # Example usage
    import matplotlib.pyplot as plt
    reader = VB2Reader('test.vb2')
    try:
        reader.open()
        print(reader.master_header.magic.decode('utf-8'))
        print(list(reader.vars.keys()))
        for var,header in reader.vars.items():
            print(f"{var}: {header.count} elements")
        plt.plot(reader["var3"])
        
        plt.show()
    except KeyError as e:
        print(e)
    finally:
        reader.close()

            