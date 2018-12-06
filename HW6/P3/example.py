#!/bin/python

#Import libraries for simulation
import tensorflow as tf
import numpy as np
import horovod.tensorflow as hvd

hvd.init()

config = tf.ConfigProto()
config.gpu_options.allow_growth = True
config.gpu_options.visible_device_list = str(hvd.local_rank())

sess = tf.InteractiveSession() #CPU version
#sess = tf.InteractiveSession(config=config) #Use only for capability 3.0 GPU

# Size of buffer	
N = 10

# Create buffer
send_buf  = np.zeros([N], dtype=np.float32)
recv0_buf  = np.zeros([N], dtype=np.float32)
recv1_buf  = np.zeros([N], dtype=np.float32)

# Rank 0 has even numbers, Rank 1 has odds
for i in range(N):
	if hvd.rank() == 0:
		send_buf[i] = i*2
	else:
		send_buf[i] = i*2+1
		
# Print initial state
print "Rank "+str(hvd.rank())+" send initial: "+str(send_buf)
if hvd.rank() == 0:
	print "Rank "+str(hvd.rank())+" recv initial: "+str(recv0_buf)
else:
	print "Rank "+str(hvd.rank())+" recv initial: "+str(recv1_buf)

# Create tensorflow variables		
Send_Buffer  = tf.Variable(send_buf,  name='Send_Buffer')
Recv0_Buffer  = tf.Variable(recv0_buf,  name='Recv0_Buffer')
Recv1_Buffer  = tf.Variable(recv1_buf,  name='Recv1_Buffer')

#communicate
bcast = tf.group(
  tf.assign(Recv1_Buffer, hvd.broadcast(Send_Buffer, 0)),  #Rank 0's send_buffer to Rank 1's recv
  tf.assign(Recv0_Buffer, hvd.broadcast(Send_Buffer, 1)))  #Rank 1's send_buffer to Rank 0's recv

# Initialize state to initial conditions
tf.global_variables_initializer().run() 

bcast.run()

# Print final state
if hvd.rank() == 0:
	print "Rank "+str(hvd.rank())+" recv final: "+str(Recv0_Buffer.eval())
else:
	print "Rank "+str(hvd.rank())+" recv final: "+str(Recv1_Buffer.eval())
    
