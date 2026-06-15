# parallel-programming
"find" command experimental recreation combined with MPI and OpenMP utilization.
<br><br>
usage: <code>make (path) (file)</code>
<br><br>
The parallelism is at two levels, MPI processes and OpenMP threads within each process. <br/>
The program runs with 8 MPI processes by default, you can change this by appending <code>NP="x"</code> <br/>
To change threads num: 
<code>export OMP_THREADS_NUM="x" </code>
<br/><br/>
<img width="850" height="637" alt="Screenshot 2026-06-15 074127a" src="https://github.com/user-attachments/assets/19424af0-2419-4fbd-a25e-070caa25e388" />

