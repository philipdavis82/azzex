import ctypes,sys,mmap,argparse
try: import numpy as np
except ImportError: 
    print("Numpy is not installed, some features may not work.")
    np = None

STR_TYPE_TO_CTYPE = {
    'long': ctypes.c_long,
    'int': ctypes.c_int,
    'float': ctypes.c_float,
    'double': ctypes.c_double,
}

STR_TYPE_TO_NPTYPE = {
    'long'  : np.int64   if np else None,
    'int'   : np.int32   if np else None,
    'float' : np.float32 if np else None,
    'double': np.float64 if np else None,
}

class VB2MasterHeader(ctypes.Structure):
    _pack_ = 8 # Ensure 8-byte alignment
    _fields_ = [
        ('_magic'       , ctypes.c_char * 4 ),
        ('version'     , ctypes.c_size_t   ),
        ('block_count' , ctypes.c_size_t   ),
        ('max_history' , ctypes.c_size_t    ),
        ('reserved'    , ctypes.c_size_t * 3)
    ]
    @property
    def magic(self) -> str:
        return self._magic.decode('utf-8').strip('\x00')
    
    def __str__(self):
        return f"VB2MasterHeader(magic={self.magic}, version={self.version}, block_count={self.block_count}, max_history={self.max_history})"

class VB2Header(ctypes.Structure):
    _pack_ = 8  # Ensure 8-byte alignment
    _fields_ = [
        ('_name', ctypes.c_char * 256),
        ('_unit', ctypes.c_char * 32),
        ('_description', ctypes.c_char * 512),
        ('_type', ctypes.c_char * 16),
        ('_offset', ctypes.c_size_t),
        ('_count', ctypes.c_size_t)
    ]
    @property
    def name(self) -> str:
        return self._name.decode('utf-8').strip('\x00')
    @property
    def unit(self) -> str:
        return self._unit.decode('utf-8').strip('\x00')
    @property
    def description(self) -> str:
        return self._description.decode('utf-8').strip('\x00')
    @property
    def type(self) -> str:
        return self._type.decode('utf-8').strip('\x00')
    @property
    def offset(self) -> int:
        return self._offset
    @property
    def count(self) -> int:
        return self._count
    def __str__(self):
        return (f"VB2Header(name={self.name}, unit={self.unit}, "
                f"description={self.description}, type={self.type}, "
                f"offset={self.offset}, count={self.count})")

class VB2Reader:
    def __init__(self, filename):
        self.filename = filename
        self.file = None
        self.mmap_obj = None
        self.master_header = None
        self.vars = {}
        self.opened_vars = {}
        

    def open(self):
        self.file = open(self.filename, 'rb')
        # Check windows or unix style
        if sys.platform.startswith('win'):
            self.mmap_obj = mmap.mmap(self.file.fileno(), 0, access=mmap.ACCESS_READ)
        else:
            self.mmap_obj = mmap.mmap(self.file.fileno(), 0, flags=mmap.ACCESS_READ, prot=mmap.PROT_READ)
        self.master_header = VB2MasterHeader.from_buffer_copy(self.mmap_obj, 0)
        self.get_all_vars()

    def close(self):
        self.opened_vars.clear()
        self.vars.clear()
        self.master_header = None
        if self.mmap_obj:
            self.mmap_obj.close()
        if self.file:
            self.file.close()

    def get_all_vars(self):
        for i in range(self.master_header.block_count):
            offset = ctypes.sizeof(VB2MasterHeader) + i * ctypes.sizeof(VB2Header)
            header = VB2Header.from_buffer_copy(self.mmap_obj, offset)
            self.vars[header.name] = header
    
    def __getitem__(self, key):
        if key in self.opened_vars:
            return self.opened_vars[key]
        if key in self.vars:
            header = self.vars[key]
            var_type = header.type
            if np:
                np_type = STR_TYPE_TO_NPTYPE.get(var_type, None)
                if np_type is not None:
                    data_offset = header.offset
                    data = np.frombuffer(self.mmap_obj, dtype=np_type, count=header.count, offset=data_offset)
                    self.opened_vars[key] = data
                    return data
            else:
                ctype = STR_TYPE_TO_CTYPE[var_type]
                data_offset = header.offset
                data = (ctype * header.count).from_buffer_copy(self.mmap_obj, data_offset)
                self.opened_vars[key] = data
            return data
        else:
            raise KeyError(f"Variable '{key}' not found in the VB2 file.")


# Example usage:
# This part is for demonstration purposes and can be removed in production code.
if __name__ == "__main__":
    if np is None: 
        print("Numpy is not installed, This example requires numpy to run.")
        sys.exit(1)
    parser = argparse.ArgumentParser(description="Read VB2 files and extract variables.")
    parser.add_argument('filename', type=str, help='Path to the VB2 file to read', default='test.vb2', nargs='?')
    args = parser.parse_args()
    # Example usage
    import matplotlib.pyplot as plt
    reader = VB2Reader(args.filename) # filename='test.vb2' by default
    try:
        reader.open()
        print(reader.master_header.magic)
        print(list(reader.vars.keys()))
        for var,header in reader.vars.items():
            print(f"{var}: {reader[var].size} elements")
        # Variables are read only when accessed
        # copy required to modify the data
        normalize = lambda v: (v - v.min()) / (v.max() - v.min())
        v1 = reader["var1"]+1
        plt.plot(v1/np.max(v1))
        plt.plot(normalize(reader["var2"]))
        plt.plot(normalize(reader["var3"]))
        plt.show()
        print(reader["var3"][[0,-1]])
    except KeyError as e:
        print(e)
    finally:
        reader.close()

            