# Parallel-programming
Experimental recreation of the "find" utility that combines MPI for distributed computing with OpenMP for shared-memory parallelism.
<br><br>
usage: <code>make (path) (file)</code>
<br><br>

The program runs with 8 MPI processes by default. You can change this by appending `NP=<num>`

To change threads num: <code>export OMP_NUM_THREADS="num"</code>
<br/><br/>
<img width="850" height="637" alt="Screenshot 2026-06-15 074127a" src="https://github.com/user-attachments/assets/19424af0-2419-4fbd-a25e-070caa25e388" />

##

The program begins with MPI_Init_thread() with MPI_THREAD_FUNNELED for thread-safe MPI,
requiring only the main OpenMP thread to make MPI calls.

MPI rank 0 partitions the filesystem into large chunks:
- fanout_thread recursively explores the filesystem down to FANOUT_DEPTH using list_dir()
- list_dir() invokes getdents64() syscall (instead of opendir()/readdir() to avoid library overhead)
- All directories at depth FANOUT_DEPTH become work units stored in StrVec
- The fanout thread acts as a producer, inserting work units into WorkQueue (protected by mutex and condition variable)
- The main thread (rank 0) acts as a consumer, removing work units from the queue

Work units are then distributed using MPI_Scatterv() (with round-robin distribution), and every rank searches its assigned subtree independently using search_root() and search_node():

search_root() calls search_node() which:
- Reads a directory using list_dir(), collecting both files and subdirectories
- Checks all files for pattern matches (matching files are added to a PrintBuf of 4096 bytes)
- Processes subdirectories: if there are at least TASK_GRAIN (8) subdirectories, creates OpenMP tasks for each subdirectory (while each thread maintains its own PrintBuf). Otherwise, recurses normally in the current thread.

MPI_Barrier() blocks each rank until all ranks have completed their searches, and finally rank 0 prints the completion message.

