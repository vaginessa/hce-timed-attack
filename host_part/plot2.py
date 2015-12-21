# -*- coding: utf-8 -*-

import numpy as np
import matplotlib.pyplot as plt

with open("/home/mkorvin/diploma/host_part/output.csv") as f:
    data = f.read()

data = data.split('\n')
x = [row.split(',')[0] for row in data]
y = [row.split(',')[1] for row in data]

fig = plt.figure()

ax1 = fig.add_subplot(111)

ax1.set_xlim([0, 500])
# ax1.set_ylim([0, 0.1])
ax1.set_title(u"Время отправки и получения сообщения")
ax1.set_xlabel(' ')
ax1.set_ylabel(u'Время, с')

ax1.plot(x,y, c='r', label=' ')

leg = ax1.legend()

plt.show()