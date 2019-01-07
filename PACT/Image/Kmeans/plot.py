import matplotlib as mpl
mpl.use('pdf')
import matplotlib.pyplot as plt 
import numpy as np

t = np.arange(0.0, 2.0, 0.01)
s = 1 + np.sin(2*np.pi*t)
plt.plot(t,s)

plt.title('first plot on ubuntu')
plt.show()
