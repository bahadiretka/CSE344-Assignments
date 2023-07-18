#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <math.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>

#define MAX_PATH_LENGTH 256
#define MAX_BUFFER_SIZE 10

// Structure to hold file information
typedef struct {
    char src_path[MAX_PATH_LENGTH];
    char dst_path[MAX_PATH_LENGTH];
} FileInfo;

// Buffer variables
FileInfo buffer[MAX_BUFFER_SIZE];
int buffer_count = 0;
int buffer_start = 0;
int buffer_end = 0;

// Mutex and condition variables for synchronization
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buffer_not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t buffer_not_empty = PTHREAD_COND_INITIALIZER;

// Counters for statistics
int num_files_copied = 0;
int num_directories_copied = 0;
int num_bytes_copied = 0;

// Flag to indicate producer has finished
int producer_done = 0;

// Function to check if the buffer is full
int buffer_full() {
    return buffer_count == MAX_BUFFER_SIZE;
}

// Function to put file information into the buffer
void buffer_put(FileInfo file_info) {
    buffer[buffer_end] = file_info;
    buffer_end = (buffer_end + 1) % MAX_BUFFER_SIZE;
    buffer_count++;
}

// Function to check if the buffer is empty
int buffer_empty() {
    return buffer_count == 0;
}

// Function to get file information from the buffer
FileInfo buffer_get() {
    FileInfo file_info = buffer[buffer_start];
    buffer_start = (buffer_start + 1) % MAX_BUFFER_SIZE;
    buffer_count--;
    return file_info;
}

// Function to copy a directory recursively
void copy_directory(const char* src_dir, const char* dst_dir) {
    DIR* dir = opendir(src_dir);
    if (dir == NULL) {
        printf("Error opening source directory: %s\n", src_dir);
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_path[MAX_PATH_LENGTH];
        char dst_path[MAX_PATH_LENGTH];
        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst_dir, entry->d_name);

        struct stat st;
        if (lstat(src_path, &st) == -1) {
            printf("Error getting file/directory information: %s\n", src_path);
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            // Create destination directory if it doesn't exist
            if (mkdir(dst_path, st.st_mode) != 0) {
                if (errno != EEXIST) {
                    printf("Error creating directory: %s\n", dst_path);
                }
            }

            // Recursively copy the subdirectory
            copy_directory(src_path, dst_path);
        } else if (S_ISREG(st.st_mode)) {
            FileInfo file_info;
            strncpy(file_info.src_path, src_path, sizeof(file_info.src_path));
            strncpy(file_info.dst_path, dst_path, sizeof(file_info.dst_path));

            // Acquire the buffer mutex
            pthread_mutex_lock(&buffer_mutex);

            // Wait until the buffer is not full
            while (buffer_full()) {
                pthread_cond_wait(&buffer_not_full, &buffer_mutex);
            }

            // Put the file information into the buffer
            buffer_put(file_info);

            // Signal that the buffer is not empty
            pthread_cond_signal(&buffer_not_empty);

            // Release the buffer mutex
            pthread_mutex_unlock(&buffer_mutex);

            num_files_copied++;
            num_bytes_copied += st.st_size;
        }
    }

    closedir(dir);
}

// Producer thread function
void* producer_thread(void* args) {
    char** dirs = (char**)args;
    char* src_dir = dirs[0];
    char* dst_dir = dirs[1];

    // Create destination directory if it doesn't exist
    if (mkdir(dst_dir, 0777) != 0) {
        if (errno != EEXIST) {
            printf("Error creating destination directory: %s\n", dst_dir);
        }
    }

    // Copy the main directory
    copy_directory(src_dir, dst_dir);

    // Set the producer done flag
    pthread_mutex_lock(&buffer_mutex);
    producer_done = 1;

    // Signal that the buffer is not empty (in case consumers are waiting)
    pthread_cond_broadcast(&buffer_not_empty);
    pthread_mutex_unlock(&buffer_mutex);

    pthread_exit(NULL);
}

// Consumer thread function
void* consumer_thread(void* arg) {
    (void)arg;
    while (1) {
        // Acquire the buffer mutex
        pthread_mutex_lock(&buffer_mutex);

        // Wait until the buffer is not empty or the producer is done
        while (buffer_empty() && !producer_done) {
            pthread_cond_wait(&buffer_not_empty, &buffer_mutex);
        }

        // Check if the producer has finished and the buffer is empty
        if (buffer_empty() && producer_done) {
            pthread_mutex_unlock(&buffer_mutex);
            break;
        }

        // Get the file information from the buffer
        FileInfo file_info = buffer_get();

        // Signal that the buffer is not full
        pthread_cond_signal(&buffer_not_full);

        // Release the buffer mutex
        pthread_mutex_unlock(&buffer_mutex);

        // Open the source file for reading
        int src_fd = open(file_info.src_path, O_RDONLY);
        if (src_fd < 0) {
            printf("Error opening source file: %s\n", file_info.src_path);
            continue;
        }

        // Open the destination file for writing
        int dst_fd = open(file_info.dst_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (dst_fd < 0) {
            printf("Error opening destination file: %s\n", file_info.dst_path);
            close(src_fd);
            continue;
        }

        // Read from source file and write to destination file
        char buffer[4096];
        ssize_t bytes_read, bytes_written;
        while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
            bytes_written = write(dst_fd, buffer, bytes_read);
            if (bytes_written < 0) {
                printf("Error writing to file: %s\n", file_info.dst_path);
                break;
            }
        }

        // Close the file descriptors
        close(src_fd);
        close(dst_fd);

        // Print completion status
        if (bytes_read < 0) {
            printf("Error reading from file: %s\n", file_info.src_path);
        } else {
            printf("Copied file: %s\n", file_info.dst_path);
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        printf("Usage: %s buffer_size num_consumers src_dir dst_dir\n", argv[0]);
        return 1;
    }
    int buffer_size __attribute__((unused)) = atoi(argv[1]);
    int num_consumers __attribute__((unused)) = atoi(argv[2]);
    char* src_dir = argv[3];
    char* dst_dir = argv[4];

    // Initialize buffer counters
    buffer_count = 0;
    buffer_start = 0;
    buffer_end = 0;

    // Create consumer threads
    pthread_t consumer_threads[num_consumers];
    for (int i = 0; i < num_consumers; i++) {
        pthread_create(&consumer_threads[i], NULL, consumer_thread, NULL);
    }

    // Get start time
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    // Create the producer thread
    char* dirs[] = { src_dir, dst_dir };
    pthread_t producer_thread_id;
    pthread_create(&producer_thread_id, NULL, producer_thread, dirs);

    // Wait for the producer to finish
    pthread_join(producer_thread_id, NULL);

    // Wait for the consumers to finish
    for (int i = 0; i < num_consumers; i++) {
        pthread_join(consumer_threads[i], NULL);
    }

    // Get end time
    gettimeofday(&end_time, NULL);

    // Calculate total time
    double total_time = (end_time.tv_sec - start_time.tv_sec) +
                        (end_time.tv_usec - start_time.tv_usec) / 1000.0;

    // Print statistics
    printf("Total files copied: %d\n", num_files_copied);
    printf("Total directories copied: %d\n", num_directories_copied);
    printf("Total bytes copied: %d\n", num_bytes_copied);
    printf("Total time taken: %.2f miliseconds\n", fabs(total_time));

    return 0;
}
