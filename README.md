# Authors
- Fernando Portillo (fxp200004)
- Meyli Colmenero (mxc200034)

# Overview

## What is working?
The program is able to read a file and search for a specific string in the file. The program is able to read multiple files and search for a specific string in each file recursively down file directories. 

It can take in a set of commands configurations such as buffer_size, file size limit, uid, gid, and the string to search for (even regex format).

It will traverse the file system and search for the string in each file. It will output the line as it finds the string in the file and at the end it will output the total number of lines found in all files.

All of this is done in parallel using threads, talking between each other using a shared buffer.

## Description of Critical Sections
The critical section being protected here is the manipulation of the shared buffers data within the safeAdd and safeRemove functions. 

These functions ensure thread-safe access to the buffers by using semaphores to control access and prevent race conditions. Within each function, the semaphore mutex is acquired to gain exclusive access to the critical section, which involves inserting or removing data from the buffers. 

Once the operation is completed, the semaphore mutex is released to allow other threads to access the buffers safely. This synchronization mechanism prevents concurrent access to the buffer data, ensuring consistency and preventing potential data corruption or inconsistency issues that could arise from concurrent accesses.

This keeps our critical section minimal and ensures that only one thread can access a specific buffer at a time. There should not a need for other operations to be protected by a critical section.

## Which buffer size gave optimal performance for 30 or more files?
The optimal buffer size for 30 or more files was 1000. This buffer size provided the best performance in terms of execution time and efficiency when processing a large number of files.

When using a buffer size higher than 1000, the performance decreased due to the overhead of managing a larger buffer and the associated synchronization mechanisms. Smaller buffer sizes also resulted in lower performance or at least not a huge increase in performance due to the increased frequency of buffer operations and context switches between threads.

## In which stage would you add an additional thread to improve performance and why you chose that stage?
An additional thread could be added to the stage where lines are being searched for the target string in each file. This stage involves searching the whole line for a specific string and can be computationally intensive, especially when processing large files with many lines.

Also, we could add a thread to opening the files and reading the lines from the files. This stage involves reading the file system and opening files, which can be a bottleneck when processing a large number of files. By adding an additional thread to this stage, we can parallelize the file reading process and improve overall performance by reducing the time spent on file I/O operations.

## Any bugs in your code?
There are no known bugs in the code. The program has been tested with various input configurations and file sizes, and it has been able to successfully search for the target string in the files without any issues and keep in consideration other configurations passed into it.

# Analysis - (VM 4 Cores, 4gb RAM)

## 1 File - 68mb - 1.380m lines

### 10 buffer size
`./pipgrep 10 59971520  -1 -1 "Date"`

```
real    0m15.584s (execution time)
user    0m16.530s (application instructions in user mode)
sys     0m3.442s (systems calls , kernel)
```

### 100 buffer size
`./pipgrep 100 59971520  -1 -1 "Date"`

```
real    0m15.803s
user    0m17.239s
sys     0m3.319s
```

### 1000 buffer size
`./pipgrep 1000 59971520  -1 -1 "Date"`

```
real    0m21.454s
user    0m22.015s
sys     0m5.599s
```

### 10000 buffer size
`./pipgrep 10000 59971520  -1 -1 "Date"`

```
real    1m34.905s 
user    1m11.020s 
sys     0m22.073s
```

### 100000 buffer size
`./pipgrep 100000 59971520  -1 -1 "Date"`

```
real    7m2.032s
user    6m18.876s
sys     0m23.883s
```

## 2300+ Files - 1gb+

### 10 buffer size
`./pipgrep 100 -1  -1 -1 "Date"`

```
real    5m9.394s
user    5m13.498s
sys     1m18.587s
```

### 100 buffer size
`./pipgrep 100 -1  -1 -1 "Date"`

```
real    7m17.654s
user    6m51.884s
sys     2m7.121s
```

### 1000 buffer size
`./pipgrep 1000 -1  -1 -1 "Date"`

```
real	7m13.175s
user	6m49.768s
sys	    2m9.888s
```

# Source Code
https://github.com/Fernando4242/OS-Project-2
