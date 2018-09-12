Single Author info:
hmajety	 Hari Krishna Majety
Group info:
hmajety	 Hari Krishna Majety
srout Sweta Rout
mreddy2 Harshavardhan Reddy Muppidi

Our Solution in brief:
1) First we divided the grid points between the processes equally. If the points are not multiple of number of processes, the remaining points are assigned to the last process.
2)Each process will compute the values of the functions at the points it is assigned to.
3)The boundary points are now exchanged between the processes using the MPI method specified. To avoid deadlocks in blocking calls, first the even processes will be in receive mode while odd processes will be in sending mode. Then they exchange the roles and odd processes will receive the boundary values from the even processes.
4)Derivatives are calculated by each process at their assigned points.
5)The derivatives and the function values are now collected at the root(0) using the MPIGatherv or Manual gather depending upon the user input.
6) The results are written to a .dat file.

How to Compile and Run:
Step1:
Copy files p2_mpi.c, p2_func.c and p2.Makefile into a folder
Step 2:
make -f p2.Makefile	(Runs the makefile)
Step 3:
Execute command "prun ./p2 <NGRID> <Blocking/NonBlocking(0/1)> <MPIGather/Manual Gather(0/1)>"


Observations:
The results will be populated into the fn-<NGRID>.dat file.

Time Analysis:
We created a batch file and ran the code for 100000 grid points for all the possible combinations of <Blocking/NonBlocking(0/1)> and <MPIGather/Manual Gather(0/1)> for a number of times and computed the average.
The averages are as follows:
Blocking with MPIGatherv - 13.004ms
Blocking with Manual Gather - 25.574ms
Non Blocking with MPIGatherv - 23.277ms
Non Blocking with Manual Gather - 32.902ms

Comments:
As we can see above, Blocking 



