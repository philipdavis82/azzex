from __future__ import annotations

import sys,os,time
sys.path.append('../py')
try: 
    from ..py import vb2_reader as vb2
except ImportError:  # If running in a different context, adjust the import path
    import vb2_reader as vb2

class TimeInfo:
    def __init__(self, loops, write_time, read_time):
        self.loops = loops
        self.write_time = write_time
        self.read_time = read_time
    
    def average_times(self,other:list[TimeInfo]):
        self.loops = other[0].loops
        self.write_time = 0
        self.read_time  = 0
        for t in other:
            self.write_time += t.write_time
            self.read_time  += t.read_time
        self.write_time /= len(other)
        self.read_time  /= len(other)

def save_read_timer(loops):
    os.system(f"cd ..; make NO_EXTRA_FILE=1 LENGTH={loops} EXTRA_VARS=97;")
    run = lambda : (time.time(),os.system("../test_vb2"))[0] 
    
    tstart = run()
    write_time = time.time() - tstart
    if write_time < 0.01: write_time = time.time() - tstart  # Ensure write_time is not zero
    if write_time < 0.01:
        tstart = run()
        write_time = time.time() - tstart

        
    print(f"Write time: {write_time:.2f} s for {loops} loops")
    tstart = time.time()
    file = vb2.VB2Reader("test.vb2")
    file.open()
    # Read all variables to ensure they are loaded
    for var in file.vars:
        data = file[var]
    data = None
    file.close()
    read_time = time.time() - tstart
    print(f"Read time: {read_time*1000:.2f} ms for {loops} loops")
    return TimeInfo(loops, write_time, read_time)

def test_save_read_timer():
    loops = list(range(100, 1000100, 50000))  # Test with different loop counts
    times = []
    for loop in loops:
        samples = []
        for i in range(10):
            samples.append(save_read_timer(loop))
        # Average Times 
        time_info = TimeInfo(0,0,0)
        time_info.average_times(samples)
        times.append(time_info)
        print(f"Loops: {time_info.loops}, Write Time: {time_info.write_time:.2f}s, Read Time: {time_info.read_time:.2f}s")
    import matplotlib.pyplot as plt
    write_times = [t.write_time for t in times]
    read_times = [t.read_time for t in times]
    plt.figure(figsize=(10, 5)) 
    plt.plot(loops, write_times, label='Write Time', marker='o')
    plt.plot(loops, read_times, label='Read Time', marker='x')
    plt.xlabel('Array Size')
    plt.ylabel('Time (seconds)')
    plt.title('Write and Read Times for VB2 Files for 100 Vars')
    plt.legend()
    plt.grid()
    plt.savefig("save_read_timer.svg")
    plt.show()

    read_times = [t.read_time*1000 for t in times]
    plt.figure(figsize=(10, 5))
    plt.plot(loops, read_times, label='Read Time', marker='o')
    plt.xlabel('Array Size')
    plt.ylabel('Read Time (milliseconds)')
    plt.title('Read Times for VB2 Files for 100 Vars')
    plt.legend()
    plt.grid()
    plt.savefig("read_timer.svg")
    plt.show()
    return time_info


if __name__ == "__main__":
    test_save_read_timer()

