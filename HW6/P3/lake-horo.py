#!/bin/python

#Group info:
#hmajety  Hari Krishna Majety
#srout Sweta Rout
#mreddy2 Muppidi Harshavardhan Reddy

#Import libraries for simulation
import tensorflow as tf
import numpy as np
import sys
import time
import horovod.tensorflow as hvd

hvd.init()

config = tf.ConfigProto()
config.gpu_options.allow_growth = True
config.gpu_options.visible_device_list = str(hvd.local_rank())

sess = tf.InteractiveSession()



#Imports for visualization
import PIL.Image

def DisplayArray(a, rank, fmt='jpeg', rng=[0,1]):
  """Display an array as a picture."""
#  a = (a - rng[0])/float(rng[1] - rng[0])*255
#  a = np.uint8(np.clip(a, 0, 255))
  global N
  h = float(1)/N
  file1 = "lake_py_" + str(rank) +".jpg"
  file2 = "lake_c_" + str(rank) +".dat"
  with open(file2,'w') as f1:
	  for i in range(len(a)):
	  	for j in range(len(a[i])):
			f1.write(str(i*h)+" "+str(j*h)+" "+str(a[i][j])+'\n')
  a = (a - rng[0])/float(rng[1] - rng[0])*255
  a = np.uint8(np.clip(a, 0, 255))
		
  with open(file1,"w") as f:
      PIL.Image.fromarray(a).save(f, "jpeg")


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

# Create buffer
#Ur0_buf  = np.zeros([3][N], dtype=np.float32)
#Utr0_buf  = np.zeros([3][N], dtype=np.float32)
#Ur1_buf  = np.zeros([3][N] , dtype=np.float32)
#Utr1_buf  = np.zeros([3] [N], dtype=np.float32)


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
U_Send  = tf.Variable(u_init,  name='U_Send')
Ut_Send  = tf.Variable(ut_init,  name='Ut_Send')

# Create tensor flow variable to receive into
Ur0 = tf.Variable(np.zeros([3, N], dtype=np.float32))
Utr0 = tf.Variable(np.zeros([3, N], dtype=np.float32))
Ur1 = tf.Variable(np.zeros([3, N], dtype=np.float32))
Utr1 = tf.Variable(np.zeros([3, N], dtype=np.float32))


#Used for calculations
U_main = tf.Variable(np.zeros([N+3, N], dtype=np.float32))
Ut_main = tf.Variable(np.zeros([N+3, N], dtype=np.float32))


#Communicate 3 rows
rank_bcast = tf.group(
  tf.assign(Ur0, hvd.broadcast(U[:3], 1)),  #Rank 1's send_buffer to Rank 0's recv for U
  tf.assign(Utr0, hvd.broadcast(Ut[:3], 1)),  #Rank 1's send_buffer to Rank 0's recv for Ut
  tf.assign(Ur1, hvd.broadcast(U[-3:], 0)),  #Rank 0's send_buffer to Rank 1's recv for U
  tf.assign(Utr1, hvd.broadcast(Ut[-3:], 0)))  #Rank 0's send_buffer to Rank 1's recv for Ut

rank0_join = tf.group(                         
    U_main.assign(tf.concat([U, Ur0], 0)),
    Ut_main.assign(tf.concat([Ut, Utr0], 0)))

rank1_join = tf.group(
      U_main.assign((tf.concat([ Ur1,U], 0))),
      Ut_main.assign((tf.concat([Utr1,Ut],0))
      ))
    


# Discretized PDE update rules
U_ = U_main + eps * Ut_main
Ut_ = Ut_main + eps * (laplace(U_main) - damping * Ut_main)


# Operation to update the state
step = tf.group(
  U_main.assign(U_),
  Ut_main.assign(Ut_))


#Operation to extract back the actual size
rank0_extract1 = tf.group(
  U.assign(tf.slice(U_main,[0,0],[N,N])))
rank0_extract2 = tf.group(
  Ut.assign(tf.slice(Ut_main,[0,0],[N,N])))

rank1_extract1 = tf.group(
  U.assign(tf.slice(U_main,[3,0],[N,N])))
rank1_extract2 = tf.group(
  Ut.assign(tf.slice(Ut_main,[3,0],[N,N])))

# Initialize state to initial conditions
tf.global_variables_initializer().run()

# Run num_iter steps of PDE
start = time.time()
for i in range(num_iter):
  # Step simulation
  rank_bcast.run()          #Broadcast
  if hvd.rank() == 0:        #Attaching the rows which they get
      rank0_join.run()    
  else:
      rank1_join.run()
  step.run({eps: 0.06, damping: 0.03})    #Calculation
  if hvd.rank() == 0:           #Dettaching the extra rows
      rank0_extract1.run()
      rank0_extract2.run()
  else:
      rank1_extract1.run()
      rank1_extract2.run()

end = time.time()
print('Elapsed time: {} seconds'.format(end - start))  

if hvd.rank() == 0:                               #For genrating the desired files and jpegs.
  DisplayArray(U.eval(), 0, rng=[-0.1, 0.1])
else:
  DisplayArray(U.eval(), 1, rng=[-0.1, 0.1])


#DisplayArray(U.eval(), rng=[-0.1, 0.1])
