import serial
import re
import time
import sys
import os

from serial.tools.list_ports import comports
for p in comports():
    # print(p.vid)
    if p.vid in {0x2e8a}:
        break
else:
    raise Exception("oops")

total_read = 0
def get_bytes():
    global total_read
    count_255_0 = 0
    count_255_1 = 0
    at_0 = True
    with serial.Serial(p.device, 1152000) as ser:
        while True:
            # Assume first parity to have two 255 values is the counter
            for b in ser.read(1):
                if at_0:
                    if b == 255:
                        count_255_0 += 1
                else:
                    if b == 255:
                        count_255_1 += 1
                at_0 = not at_0
            if count_255_0 == 2 or count_255_1 == 2:
                # Read one more byte to get to next counter
                ser.read(1)
                break
        while True:
            for b in ser.read():
                total_read += 1
                yield(b)

gen = get_bytes()
last_time = time.time()
last_counter, last_state = 255, None
for counter, state in zip(gen, gen):
    if counter<last_counter:
        # last_counter 255, counter 100,,, 255-counter = 155 bytes length
        length = last_counter - counter
    elif counter>=last_counter: # counter should always be 255 here 
        length = (last_counter+256) - counter
    sys.stdout.buffer.write(bytes([state])*length)
    
