#!/bin/bash

#############################################################################################
#
# FILENAME:    spark-hadoop-setup.sh
#
# USAGE:       "source spark-hadoop-setup.sh"
#
# NOTE:        This script sets multiple environment variables, so you MUST use
#              "source" to run the script. Otherwise, the shell in which you ran
#              this script will not pick up the new environment variables!
#
# DESCRIPTION: This script sets up hadoop on the ARC cluster. It assumes one master node
#              and N slave nodes, where N is the number of nodes you reserved using "srun". 
#              The master node will be the lowest numbered node in your reservation. It
#              sets up the HDFS and also creates a "/user/UNITYID" directory inside the
#              HDFS, which is where any input/output will be read/written to. To make sure
#              this script ran successfully, run "hdfs dfs -ls /user". You should see the
#              "/user/UNITYID" directory that was created. 
# 
# AUTHOR:      Tyler Stocksdale
# DATE:        10/27/2017
#
#############################################################################################

ORIG_DIR=`pwd`

#Should already be in .bashrc but reloading just in case
module load java

#Set up new hadoop directory in the current directory
rm -rf hadoop/
mkdir hadoop/
cd hadoop
mkdir -p etc/hadoop
mkdir bin
cd bin
ln -s /usr/local/hadoop/bin/* . 
cd ..
mkdir libexec
cd libexec
ln -s /usr/local/hadoop/libexec/* . 
cd ..
mkdir sbin
cd sbin
ln -s /usr/local/hadoop/sbin/* . 
cd ..
ln -s /usr/local/hadoop/* .

#Set environment variables (These will not persist unless using "source" command!)
export HADOOP_HOME=`pwd`
export HADOOP_INSTALL=$HADOOP_HOME
export HADOOP_COMMON_LIB_NATIVE_DIR=$HADOOP_INSTALL/lib/native
export HADOOP_OPTS="-Djava.library.path=$HADOOP_INSTALL/lib/native"
export PATH="$PATH:$HADOOP_HOME/bin"
export CLASSPATH=`hadoop classpath`

#Spark environment variables
export SPARK_DIST_CLASSPATH=$(hadoop classpath)
export SPARK_HOME=/usr/local/spark
export PATH="$PATH:$SPARK_HOME/bin"
export CLASSPATH="$CLASSPATH:$SPARK_HOME/lib/*"

#Below files need to be put in etc/hadoop directory
cd etc/hadoop

#Create file slaves
echo $SLURM_NODELIST | 
	tr -d c | 
	tr -d [ | 
	tr -d ] | 
	perl -pe 's/(\d+)-(\d+)/join(",",$1..$2)/eg' | 
	awk 'BEGIN { RS=","} { print "c"$1 }' > slaves
	
#Create file masters
head -1 slaves > masters

MASTER=$(cat masters)

#Create file core-site.xml
echo "<configuration>
    <property>
        <name>fs.defaultFS</name>
        <value>hdfs://"$MASTER":9000</value>
    </property>
</configuration>" > core-site.xml

#Create file hdfs-site.xml
echo "<configuration>
    <property>
        <name>dfs.replication</name>
        <value>1</value>
    </property>
    <property>
        <name>dfs.datanode.data.dir</name>
        <value>/tmp/"$USER"/name/data</value>
    </property>
    <property>
        <name>dfs.namenode.name.dir</name>
        <value>/tmp/"$USER"/name</value>
    </property>
</configuration>" > hdfs-site.xml

#Create mapred-site.xml
echo "<configuration>
   <property>
      <name>mapred.job.tracker</name>
      <value>"$MASTER":9001</value>
   </property>
</configuration>" > mapred-site.xml

#Remove tmp directory then create a new one for all nodes
for curNode in `cat slaves`; do
  ssh -n $curNode "rm -rf /tmp/$USER; mkdir -p /tmp/$USER"
done

#Make sure all ssh's finish before moving on
sleep 1


#Set up the HDFS
cd $ORIG_DIR
hdfs getconf -namenodes
hdfs namenode -format
hadoop/sbin/start-dfs.sh
hdfs dfs -mkdir /user
hdfs dfs -mkdir /user/$USER

