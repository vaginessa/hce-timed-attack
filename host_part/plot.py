import matplotlib.pyplot as plt
import numpy as np

import matplotlib.cbook as cbook

fname = cbook.get_sample_data('/home/mkorvin/diploma/host_part/output.csv', asfileobj=False)

plt.axis([0, 500, 0, 1])

# test 2; use names
plt.plotfile(fname, cols=(0, 1), delimiter=',',
             names=['$x$', '$y$'])

plt.show()