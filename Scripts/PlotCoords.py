#!/usr/bin/env python3

import matplotlib.pyplot as plt
import csv

x = []
y = []

file_path = '/tmp/imu_path.txt'

with open(file_path,'r') as csvfile:
    plots = csv.reader(csvfile, delimiter=',')
    for row in plots:
        x.append(float(row[0]))
        y.append(float(row[1]))

plt.plot(x,y, label='Path')
plt.grid()
plt.xlabel('x')
plt.ylabel('y')
plt.title(file_path)
plt.legend()
plt.show()

