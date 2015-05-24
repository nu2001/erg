from __future__ import print_function
import serial
import matplotlib.pyplot as plt
import sys
import time
import pickle

timestr = time.strftime('%Y%m%d-%H%M%S')

port = serial.Serial('/dev/ttyACM0', 921600)

port.write('c')
line = port.readline()
values = line.split(' ')
num_samples = int(values[1])
start_time_ms = int(values[2])
end_time_ms = int(values[3])

#print('ns: {}, st: {}, et: {}'.format(num_samples, start_time_ms, end_time_ms))
samples_per_sec = float(num_samples)*1000.0/(end_time_ms-start_time_ms)
print('Captured {} samples in {} ms'.format(num_samples, end_time_ms-start_time_ms))
print('  {0:.4f} Msamples/sec'.format(samples_per_sec/1e6))

samples = []
line = port.readline()

print('|', end='')
for i in range(num_samples/1000):
    print('-', end='')
print('|')


print('|', end='')
while line != 'd\r\n':
    values = line.split(' ')
    samples.append(int(values[1]))
    if len(samples) % 1000 == 0:
        print('.', end='')
        sys.stdout.flush()
    line = port.readline()
print('|')

port.close();

print('received {} samples'.format(len(samples)))

pickle.dump(samples, open(timestr+'.pickle', 'w'))

plt.plot(samples)
plt.ylim(-50, 5050)
plt.grid('on')

plt.show()
