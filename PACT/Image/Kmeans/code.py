import numpy as np
import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt

"matplotlib inline"
from sklearn.datasets.samples_generator import make_blobs

plt.figure(num=3,figsize=(8,5))
X, y = make_blobs(n_samples=1000, n_features=2, centers=[[-1,-1], [0,0], [1,1], [2,2]], cluster_std=[0.4, 0.2, 0.2, 0.2], random_state =9)
plt.scatter(X[:,0],X[:,1],marker='o')
plt.show()
plt.savefig("1.png")

from sklearn.cluster import KMeans
X2 = X
y_pred = KMeans(n_clusters=2, random_state=9).fit_predict(X2)
plt.scatter(X2[:,0], X2[:,1], c=y_pred)
plt.show()
plt.savefig("2.png")


X3 = X
y_pred = KMeans(n_clusters=3, random_state=9).fit_predict(X3)
plt.scatter(X3[:,0], X3[:,1], c=y_pred)
plt.show()
plt.savefig("3.png")


X4 = X
y_pred = KMeans(n_clusters=4, random_state=9).fit_predict(X4)
plt.scatter(X4[:,0], X4[:,1], c=y_pred)
plt.show()
plt.savefig("4.png")




