#!/bin/python

#Import libraries for simulation
import tensorflow as tf
import numpy as np
import sys
import time

#Imports for visualization
import PIL.Image

def DisplayArray(a, fmt='jpeg', rng=[0,1]):
  """Display an array as a picture."""
#  a = (a - rng[0])/float(rng[1] - rng[0])*255
#  a = np.uint8(np.clip(a, 0, 255))
  global N
  h = float(1)/N
  with open('lake_c.dat','w') as f1:
	  for i in range(len(a)):
	  	for j in range(len(a[i])):
			f1.write(str(i*h)+" "+str(j*h)+" "+str(a[i][j])+'\n')
  a = (a - rng[0])/float(rng[1] - rng[0])*255
  a = np.uint8(np.clip(a, 0, 255))
		
  with open("lake_py.jpg","w") as f:
      PIL.Image.fromarray(a).save(f, "jpeg")

sess = tf.InteractiveSession()

# Computational Convenience Functions
def make_kernel(a):
  """Transform a 2D array into a convolution kernel"""
  a = np.asarray(a)
  a = a.reshape(list(a.shape) + [1,1])
  return tf.constant(a, dtype=1)

def simple_conv(x, k):
  """A simplified 2D convolution operation"""
  x = tf.expand_dims(tf.expand_dims(x, 0), -1)
  y = tf.nn.depthwise_conv2d(x, k, [1, 1, 1, 1], padding='SAME')
  return y[0, :, :, 0]

def laplace(x):
  """Compute the 2D laplacian of an array"""
#  5 point stencil #
  five_point = [[0.0, 1.0, 0.0],
                [1.0, -4., 1.0],
                [0.0, 1.0, 0.0]]

#  9 point stencil #
  nine_point = [[0.25, 1.0, 0.25],
                [1.00, -5., 1.00],
                [0.25, 1.0, 0.25]]
# 13 point stencil #
  th_point = [[0.000,0.000,0.000,0.125,0.000,0.000,0.000],
	      [0.000,0.000,0.000,0.250,0.000,0.000,0.000],
	      [0.000,0.000,0.000,1.000,0.000,0.000,0.000],
	      [0.125,0.250,1.000,-5.50,1.000,0.250,0.125],
	      [0.000,0.000,0.000,1.000,0.000,0.000,0.000],
	      [0.000,0.000,0.000,0.250,0.000,0.000,0.000],
	      [0.000,0.000,0.000,0.125,0.000,0.000,0.000]]	 
  laplace_k = make_kernel(th_point)
  return simple_conv(x, laplace_k)

# Define the PDE
if len(sys.argv) != 4:
	print "Usage:", sys.argv[0], "N npebs num_iter"
	sys.exit()
	
N = int(sys.argv[1])
npebs = int(sys.argv[2])
num_iter = int(sys.argv[3])

# Initial Conditions -- some rain drops hit a pond

# Set everything to zero
u_init  = np.zeros([N, N], dtype=np.float32)
ut_init = np.zeros([N, N], dtype=np.float32)

# Some rain drops hit a pond at random points
for n in range(npebs):
  a,b = np.random.randint(0, N, 2)
  u_init[a,b] = np.random.uniform()

# Parameters:
# eps -- time resolution
# damping -- wave damping
eps = tf.placeholder(tf.float32, shape=())
damping = tf.placeholder(tf.float32, shape=())

# Create variables for simulation state
U  = tf.Variable(u_init)
Ut = tf.Variable(ut_init)

# Discretized PDE update rules
U_ = U + eps * Ut
Ut_ = Ut + eps * (laplace(U) - damping * Ut)

# Operation to update the state
step = tf.group(
  U.assign(U_),
  Ut.assign(Ut_))

# Initialize state to initial conditions
tf.global_variables_initializer().run()

# Run num_iter steps of PDE
start = time.time()
for i in range(num_iter):
  # Step simulation
  step.run({eps: 0.06, damping: 0.03})


end = time.time()
print('Elapsed time: {} seconds'.format(end - start))  
DisplayArray(U.eval(), rng=[-0.1, 0.1])
