#!/usr/bin/python

import serial
import time
from sys import platform

if 'linux' in platform:
  comport='/dev/ttyUSB0'
elif 'win' in platform:
  comport='COM1'


# flush the banner:
ser=serial.Serial(comport,9600,timeout=1)
banner=ser.read(1000)
ser.close()


print "chipID: "
ser=serial.Serial(comport,9600,timeout=1)
ser.write('*'); time.sleep(0.5)
ser.write('1'); time.sleep(0.5)
ser.write('#'); time.sleep(0.5)
print ser.read(1000)

ser.close()
