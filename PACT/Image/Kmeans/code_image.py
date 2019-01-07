import numpy as np
import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.image as mpimg


#from sklearn.datasets.samples_generator import make_blobs

#plt.figure(num=3,figsize=(8,5))
#X, y = make_blobs(n_samples=1000, n_features=2, centers=[[-1,-1], [0,0], [1,1], [2,2]], cluster_std=[0.4, 0.2, 0.2, 0.2], random_state =9)
#plt.scatter(X[:,0],X[:,1],marker='o')
#plt.show()

ex = mpimg.imread("ex.png")
ex.shape

#from sklearn.cluster import KMeans
#X2 = X
#y_pred = KMeans(n_clusters=2, random_state=9).fit_predict(X2)
#plt.scatter(X2[:,0], X2[:,1], c=y_pred)
plt.imshow(ex)
plt.axis('off')
plt.show()

plt.savefig("treated.png")
