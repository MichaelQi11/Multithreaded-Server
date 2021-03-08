Procedure for using and testing the servers:
1. call make in terminal to compile all codes
2. run ./mainK size_of_array IP Port (eg. ./main1 10 127.0.0.1 3000)
3. run ./test.sh size_of_array IP Port (eg. ./test.sh 10 127.0.0.1 3000)
4. all the time outputs will be in server_output_time_aggregated in the same directory and one may want to mannually add separater in the file to split
   the results of each run

Implementations:
server_one_rwlock (main1): using one read write lock for the whole data array
server_rwlock_array (main2): using one read write lock for each element of the data array
server_one_mutex (main3): using one mutex lock for the whole data array
server_mutex_array (main4): using one mutex lock for each element of the data array
