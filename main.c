#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAXPATH 1024
#define MAX_ENTRIES 65536

static int snapshot_dir(const char *path, char (**out)[MAXPATH]){
DIR *d = opendir(path);
if (!d) return -1;

char (*buf)[MAXPATH] = malloc(MAX_ENTRIES * sizeof(*buf));
if (!buf){
closedir(d);
return -1;
}

int n = 0;
struct dirent *ent;

while (n < MAX_ENTRIES && (ent = readdir(d))){
if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
continue;

snprintf(buf[n++], MAXPATH, "%s/%s", path, ent->d_name);
}

closedir(d);
*out = buf;

return n;
}

static void search_task(const char *root, const char *pattern, int rank){
char (*entries)[MAXPATH];
int n = snapshot_dir(root, &entries);

if (n < 0) return;

for (int i = 0; i < n; i++){
struct stat st;

if (stat(entries[i], &st) != 0) continue;

if (S_ISDIR(st.st_mode)){
char taskpath[MAXPATH];
snprintf(taskpath, MAXPATH, "%s", entries[i]);

#pragma omp task firstprivate(taskpath)
search_task(taskpath, pattern, rank);
}
else{
const char *base = strrchr(entries[i], '/');
base = base ? base + 1 : entries[i];

if (strstr(base, pattern)){
#pragma omp critical
printf("[MPI %d OMP %d] %s\n", rank, omp_get_thread_num(), entries[i]);
}
}
}

free(entries);
}

static void search_dir(const char *root, const char *pattern, int rank){
#pragma omp parallel
#pragma omp single
{
search_task(root, pattern, rank);

#pragma omp taskwait
}
}

static int gather_work(const char *root, char (**out)[MAXPATH]){
char (*buf)[MAXPATH] = malloc(MAX_ENTRIES * sizeof(*buf));
if (!buf) return -1;

int total = 0;

char (*top)[MAXPATH];
int ntop = snapshot_dir(root, &top);

if (ntop < 0){
free(buf);
return -1;
}

for (int i = 0; i < ntop && total < MAX_ENTRIES; i++){
struct stat st;

if (stat(top[i], &st) != 0)
continue;

if (!S_ISDIR(st.st_mode)){
snprintf(buf[total++], MAXPATH, "%s", top[i]);
continue;
}

char (*sub)[MAXPATH];
int nsub = snapshot_dir(top[i], &sub);

if (nsub <= 0){
snprintf(buf[total++], MAXPATH, "%s", top[i]);

if (nsub == 0)
free(sub);

continue;
}

for (int j = 0; j < nsub && total < MAX_ENTRIES; j++)
snprintf(buf[total++], MAXPATH, "%s", sub[j]);

free(sub);
}

free(top);
*out = buf;

return total;
}

int main(int argc, char **argv){
MPI_Init(&argc, &argv);

int rank, size;

MPI_Comm_rank(MPI_COMM_WORLD, &rank);
MPI_Comm_size(MPI_COMM_WORLD, &size);

if (argc < 2){
if (rank == 0)
fprintf(stderr, "Usage: make PATTERN=<pattern>\n");

MPI_Finalize();
return 1;
}

const char *pattern = argv[1];

if (rank == 0){
char (*work)[MAXPATH];
int nwork = gather_work("/", &work);

if (nwork < 0){
fprintf(stderr, "rank 0: failed to read /\n");

for (int i = 0; i < size; i++)
MPI_Send("", MAXPATH, MPI_CHAR, i, 1, MPI_COMM_WORLD);

MPI_Finalize();
return 1;
}

int total = nwork + size;
MPI_Request *reqs = malloc(total * sizeof(MPI_Request));

for (int i = 0; i < nwork; i++){
MPI_Isend(work[i], MAXPATH, MPI_CHAR, i % size, 0, MPI_COMM_WORLD, &reqs[i]);
}

char empty[MAXPATH] = "";

for (int i = 0; i < size; i++){
MPI_Isend(empty, MAXPATH, MPI_CHAR, i, 1, MPI_COMM_WORLD, &reqs[nwork + i]);
}

MPI_Waitall(total, reqs, MPI_STATUSES_IGNORE);

free(reqs);
free(work);
}

while (1){
char p[MAXPATH];
MPI_Status st;

MPI_Recv(p, MAXPATH, MPI_CHAR, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &st);

if (st.MPI_TAG == 1) break;

search_dir(p, pattern, rank);
}

MPI_Barrier(MPI_COMM_WORLD);

if (rank == 0)
printf("[done] search for \"%s\" complete across %d ranks.\n", pattern, size);

MPI_Finalize();
return 0;
}