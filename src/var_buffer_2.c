

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>

/*
    Attempting to keep the object file isolated from the header file 
    as well as isolated from the main program.
    This file contains the implementation of the variable buffer system.
    It provides functions to initialize, open, close, add variables, start recording,
    record data, flush data, and reset the variable buffer system.
    It also includes a logging system to track the operations performed on the variable buffer.
    The logging system allows for different log levels (DEBUG, INFO, WARNING, ERROR, FATAL)
    and provides functions to initialize, set log level, write logs, flush logs, and close the log file.
    The variable buffer system is designed to handle telemetry data recording and management,
    allowing for efficient storage and retrieval of variable data in a structured format.
*/

#ifndef VB_MAX_VAR_SIZE
    #define VB_MAX_VAR_SIZE 8 // Maximum size of a variable in bytes
#endif

#ifndef VB_BUFFER_SIZE
    #define VB_BUFFER_SIZE 0x1000 // Buffer size of 1 page
#endif

#ifndef VB_LOG_BUFFER_SIZE
    #define VB_LOG_BUFFER_SIZE 0x4000 // Buffer size of 1 page
#endif
#ifndef VB_LOG_BUFFER_OVERFLOW
    #define VB_LOG_BUFFER_OVERFLOW 0x1000 // Buffer size of 1 page
#endif

#ifdef VB2_DEBUG_ENABLED
    #define VB_DEBUG(fmt, ...) printf("DEBUG[%d]: " fmt "\n", __LINE__, ##__VA_ARGS__)
#else
    #define VB_DEBUG(fmt, ...) (void)0 // No-op if debugging is disabled
#endif

#define VB2_VERSION_MINOR 0 // Version of the variable buffer system
#define VB2_VERSION_MAJOR 1 // Major version of the variable buffer system
#define VB2_VERSION (VB2_VERSION_MAJOR * 1000 + VB2_VERSION_MINOR) // Combined version number

#define VB2_MAGIC "VB2" // Magic number to identify the variable buffer file format

/*
[VB_Header 1]
[VB_Header 2]
[VB_Header 3]
...
[VB_Header 1]
[ VB_DATA 1 ]
[    ...    ]
[    ...    ]
[ VB_DATA 2 ]
[    ...    ]
[    ...    ]
[ VB_DATA 3 ]
[    ...    ]
[    ...    ]
...
[ VB_DATA N ]
[    ...    ]
[    ...    ]
*/

#pragma pack(push, 8) // Ensure standard 8-byte alignment for the structures
struct VB_Master_Header {
    char     magic[4];        // Magic number to identify the file type
    size_t   version;         // Version of the file format
    size_t   block_count;     // Number of variable blocks in the file
    size_t   max_history;     // Maximum history size for the variable buffer
    size_t   reserved[3];     // Reserved for future use
};

struct VB_Header {
    char     name[256];        // Name of the variable
    char     unit[32];         // Unit of the variable (e.g., meters, seconds, etc.)
    char     description[512]; // Description of the variable
    char     type[16];         // Type of the variable (e.g., int, float, etc.)
    size_t   offset;           // Offset of the variable in the buffer
    size_t   count;            // Count of the variable data
};

struct VB_Block_Proxy{
    struct VB_Header header;            // Header information for the variable buffer
    void    *var_ptr;               // Pointer to the data of the variable
    uint8_t  var_size;              // Size of the data in bytes
    size_t   offset;                    // Offset of the memory block in the file
    uint8_t  buffer[VB_BUFFER_SIZE+VB_MAX_VAR_SIZE];// Buffer to hold the variable data , +VB_MAX_VAR_SIZE to reduce overflow risk
    size_t   buffer_offset;          // Current offset in the buffer for writing data
};

struct VB_File{
    char     filename[4096];            // Full file path
    FILE    *fp;                        // File pointer for the file
    struct   VB_Master_Header master_header; // Master header information for the variable buffer file
    struct   VB_Block_Proxy *blocks;    // Pointer to the variable blocks in the buffer
    size_t   block_count;               // Number of variable blocks in the buffer
    size_t   max_history;
    size_t   current_history;           // Current size of the history buffer
};
#pragma pack(pop) // Restore the previous packing alignment
enum severity {
    VB_LOG_DEBUG = 0, // Debug level logging
    VB_LOG_INFO,      // Informational logging
    VB_LOG_WARNING,   // Warning level logging
    VB_LOG_ERROR,     // Error level logging
    VB_LOG_FATAL      // Fatal error logging
};  

struct VB_LOG {
    enum severity level;                // Current log level
    char     filename[4096+6];            // Name of the log file
    char     buffer[VB_LOG_BUFFER_SIZE+VB_LOG_BUFFER_OVERFLOW];    // Buffer to hold log data
    size_t   buffer_offset;             // Current offset in the log buffer
    int      echo_to_stdout;          // Flag to indicate if logs should be echoed to stdout
    int      echo_to_stderr;          // Flag to indicate if logs should be echoed to stderr
    FILE    *fp;                        // File pointer for the log file
};

struct VB_LOG  vb_log;
struct VB_File vb_file;

// ***********************************************
//         Magic Logging system
// ***********************************************

void vb_log_init(const char *filename) {
    if (filename == NULL || strlen(filename) == 0) {
        return; // Invalid filename
    }
    strncpy(vb_log.filename, filename, sizeof(vb_log.filename) - 1);
    vb_log.filename[sizeof(vb_log.filename) - 1] = '\0'; // Ensure null termination

    vb_log.fp = fopen(vb_log.filename, "w");
    if (vb_log.fp == NULL) {
        return; // Failed to open log file
    }
    
    vb_log.level = VB_LOG_DEBUG; // Set default log level to DEBUG
    vb_log.buffer[0]      = '\0'; // Initialize the log buffer
    vb_log.buffer_offset  = 0; // Initialize the buffer offset to zero
    vb_log.echo_to_stdout = 0; // Default to echo logs to stdout
    vb_log.echo_to_stderr = 0; // Default to not echo logs to stderr
}

void vb_log_set_echo(int echo_stdout, int echo_stderr) {
    vb_log.echo_to_stdout = (echo_stdout != 0); // Set whether to echo logs to stdout
    vb_log.echo_to_stderr = (echo_stderr != 0); // Set whether to echo logs to stderr
}

void vb_log_set_level(enum severity level) {
    vb_log.level = level; // Set the current log level
}

void vb_clear_log_buffer() {
    vb_log.buffer[0] = '\0'; // Clear the log buffer
    vb_log.buffer_offset = 0; // Reset the buffer offset
}

void vb_append_to_log_buffer(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int written = vsnprintf(vb_log.buffer + vb_log.buffer_offset, VB_LOG_BUFFER_SIZE - vb_log.buffer_offset, fmt, args);
    va_end(args);
    vb_log.buffer_offset += written; // Update the buffer offset
}


void vb_log_write(enum severity level, size_t lineno, const char *file ,const char *fmt, ...) {
    if (level < vb_log.level) {
        return; // Log level is lower than the current level, do not log
    }
    if (vb_log.fp == NULL) {
        return; // Log file is not open
    }
    // vb_clear_log_buffer(); // Clear the log buffer before writing new data
    char counter[64] = "\0";
    if(vb_file.fp != NULL) {
        counter[0] = '[';
        int write_size = snprintf(counter + 1, sizeof(counter) - 2, "%zu", vb_file.current_history); // Write the block count to the counter
        counter[write_size+1] = ']'; // Add closing bracket
        counter[write_size+2] = '\0'; // Null terminate the string
    }
    if(level == VB_LOG_DEBUG) {
        vb_append_to_log_buffer("%6s DEBUG   %s[%zu]: ",counter , file, lineno);
    } else if(level == VB_LOG_INFO) {
        vb_append_to_log_buffer("%6s INFO    %s[%zu]: ",counter , file, lineno);
    } else if(level == VB_LOG_WARNING) {
        vb_append_to_log_buffer("%6s WARNING %s[%zu]: ",counter , file, lineno);
    } else if(level == VB_LOG_ERROR) {
        vb_append_to_log_buffer("%6s ERROR   %s[%zu]: ",counter , file, lineno);
    } else if(level == VB_LOG_FATAL) {
        vb_append_to_log_buffer("%6s FATAL   %s[%zu]: ",counter , file, lineno);
    }

    va_list args;
    va_start(args, fmt);
    int written = vsnprintf(vb_log.buffer + vb_log.buffer_offset, VB_LOG_BUFFER_SIZE - vb_log.buffer_offset, fmt, args);
    vb_log.buffer_offset += written; 
    va_end(args);   

    vb_append_to_log_buffer("\n"); // Append a newline at the end of the log entry
    if (vb_log.echo_to_stdout) {
        printf("%s", vb_log.buffer); // Echo the log to stdout if enabled
        // Must flush so not to duplicate the log
        fwrite(vb_log.buffer, 1, vb_log.buffer_offset, vb_log.fp); // Write the log buffer to the file
        fflush(vb_log.fp); // Flush the file to ensure data is written
        vb_clear_log_buffer(); // Clear the log buffer after writing
    }
    else if(vb_log.buffer_offset > VB_LOG_BUFFER_SIZE) { // Check if the buffer is full
        fwrite(vb_log.buffer, 1, vb_log.buffer_offset, vb_log.fp); // Write the log buffer to the file
        fflush(vb_log.fp); // Flush the file to ensure data is written
        vb_clear_log_buffer(); // Clear the log buffer after writing
    }
}

void vb_log_flush() {
    if (vb_log.fp == NULL) {
        return; // Log file is not open
    }
    if (vb_log.buffer_offset > 0) { // If there is data in the buffer
        fwrite(vb_log.buffer, 1, vb_log.buffer_offset, vb_log.fp); // Write the log buffer to the file
        fflush(vb_log.fp); // Flush the file to ensure data is written
        vb_clear_log_buffer(); // Clear the log buffer after writing
    }
}

void vb_log_close() {
    if (vb_log.fp != NULL) {
        vb_log_flush(); // Flush any remaining data in the log buffer
        fclose(vb_log.fp); // Close the log file
        vb_log.fp = NULL; // Set the file pointer to NULL
    }
    vb_clear_log_buffer(); // Clear the log buffer
}
    

// ***********************************************
// Telemetry recording system for variable buffers
// ***********************************************
void vb2_init() {
    VB_DEBUG("Initializing variable buffer system");
    memset(&vb_file, 0, sizeof(vb_file)); // Initialize the variable buffer file structure
}

int vb2_open(const char *filename) {
    VB_DEBUG("Opening variable buffer file: %s", filename);
    if (filename == NULL || strlen(filename) == 0) {
        VB_DEBUG("Invalid filename provided");
        return -1; // Invalid parameters
    }

    strncpy(vb_file.filename, filename, sizeof(vb_file.filename) - 1);
    vb_file.filename[sizeof(vb_file.filename) - 1] = '\0'; // Ensure null termination

    if(vb_file.fp != NULL) {
        fclose(vb_file.fp); // Close any previously opened file
        vb_file.fp = NULL;
    }

    vb_file.fp = fopen(vb_file.filename, "wb");
    if (vb_file.fp == NULL) {
        VB_DEBUG("Failed to open file: %s", vb_file.filename);
        return -1; // Failed to open file
    }

    return 0; // Success
}

void vb2_enable_log(){
    if( vb_file.fp == NULL) {
        VB_DEBUG("Variable buffer file is not open, cannot enable logging");
        return; // Variable buffer file is not open
    }
    char accompanying_vb_text_log[4096+6];
    snprintf(accompanying_vb_text_log, sizeof(accompanying_vb_text_log), "%s.log", vb_file.filename);
    vb_log_init(accompanying_vb_text_log); // Initialize the log file with the accompanying log file name
}

void vb2_close() {
    if (vb_file.fp != NULL) {
        fclose(vb_file.fp);
        vb_file.fp = NULL;
    }
    if (vb_file.blocks != NULL) {
        free(vb_file.blocks);
        vb_file.blocks = NULL;
    }
}

void vb2_add_variable(const char *name, const char *unit, const char *description, const char *type, void *var_ptr, uint8_t var_size) {
    VB_DEBUG("Adding variable: %s, unit: %s, description: %s, type: %s, size: %d", name, unit, description, type, var_size);
    if (name == NULL || unit == NULL || description == NULL || type == NULL || var_ptr == NULL || var_size <= 0 || var_size > VB_MAX_VAR_SIZE) {
        return; // Invalid parameters
    }

    struct VB_Block_Proxy *new_block = realloc(vb_file.blocks, sizeof(struct VB_Block_Proxy) * (vb_file.block_count + 1));
    if (new_block == NULL) {
        return; // Memory allocation failed
    }
    vb_file.blocks = new_block;

    strncpy(vb_file.blocks[vb_file.block_count].header.name, name, sizeof(vb_file.blocks[vb_file.block_count].header.name) - 1);
    vb_file.blocks[vb_file.block_count].header.name[sizeof(vb_file.blocks[vb_file.block_count].header.name) - 1] = '\0';
    strncpy(vb_file.blocks[vb_file.block_count].header.unit, unit, sizeof(vb_file.blocks[vb_file.block_count].header.unit) - 1);
    vb_file.blocks[vb_file.block_count].header.unit[sizeof(vb_file.blocks[vb_file.block_count].header.unit) - 1] = '\0';
    strncpy(vb_file.blocks[vb_file.block_count].header.description, description, sizeof(vb_file.blocks[vb_file.block_count].header.description) - 1);
    vb_file.blocks[vb_file.block_count].header.description[sizeof(vb_file.blocks[vb_file.block_count].header.description) - 1] = '\0';
    strncpy(vb_file.blocks[vb_file.block_count].header.type, type, sizeof(vb_file.blocks[vb_file.block_count].header.type) - 1);
    vb_file.blocks[vb_file.block_count].header.type[sizeof(vb_file.blocks[vb_file.block_count].header.type) - 1] = '\0';
    
    vb_file.blocks[vb_file.block_count].var_ptr         = var_ptr;
    vb_file.blocks[vb_file.block_count].var_size        = var_size;
    vb_file.blocks[vb_file.block_count].offset          = 0; // Initialize offset to zero
    vb_file.blocks[vb_file.block_count].buffer_offset   = 0; // Initialize offset to zero

    vb_file.block_count++;
}

void vb2_save_var_headers(){
    for (size_t i = 0; i < vb_file.block_count; i++) {
        struct VB_Block_Proxy *block = &vb_file.blocks[i];
        size_t header_offset = sizeof(struct VB_Header) * i + sizeof(struct VB_Master_Header); // Calculate the header offset for each block
        fseek(vb_file.fp, header_offset, SEEK_SET);
        fwrite(&block->header,sizeof(block->header), 1, vb_file.fp); // Write the variable data to the file
    }
}

void vb2_save_master_header(size_t block_count, size_t max_history) {
    VB_DEBUG("Saving master header with block count: %zu, max history: %zu", block_count, max_history);
    strncpy(vb_file.master_header.magic, VB2_MAGIC, sizeof(vb_file.master_header.magic) - 1);
    vb_file.master_header.magic[sizeof(vb_file.master_header.magic) - 1] = '\0'; // Ensure null termination
    vb_file.master_header.block_count   = block_count;
    vb_file.master_header.max_history   = max_history;
    vb_file.master_header.version       = VB2_VERSION; // Set the version of the file format
    fseek(vb_file.fp, 0, SEEK_SET);
    fwrite(&vb_file.master_header, sizeof(vb_file.master_header), 1, vb_file.fp); // Write the master header to the file
}

void vb2_start(size_t max_history){
    VB_DEBUG("Starting recording session with max history: %zu", max_history);
    size_t offset = sizeof(struct VB_Header) * vb_file.block_count + sizeof(struct VB_Master_Header); // Calculate the initial offset based on the number of blocks
    vb_file.max_history = max_history; // Set the maximum history size
    for (size_t i = 0; i < vb_file.block_count; i++) {
        struct VB_Block_Proxy *block = &vb_file.blocks[i];
        block->header.offset = offset;
        block->header.count = 0; // Assuming each variable is counted once
        block->offset = offset; // Set the offset for the variable data
        offset += block->var_size*max_history; // Update the offset for the next variable
    }
    vb2_save_var_headers(); // Save the variable headers to the file
    vb2_save_master_header(vb_file.block_count, max_history); // Save the master header with the block count and max history

    int s = ftruncate(fileno(vb_file.fp), offset); // Truncate the file to the new size
    if (s != 0) {
        VB_DEBUG("Failed to truncate file to size %zu", offset);
        fclose(vb_file.fp);
        vb_file.fp = NULL;
        return; // Failed to truncate the file
    }
}

void vb2_flush_all() {
    if (vb_file.fp == NULL || vb_file.blocks == NULL) {
        return; // File not open or no blocks to flush
    }
    for (size_t i = 0; i < vb_file.block_count; i++) {
        struct VB_Block_Proxy *block = &vb_file.blocks[i];
        if (block->buffer_offset > 0) { // If there is data in the buffer
            fseek(vb_file.fp, block->offset, SEEK_SET); // Seek to the variable's offset in the file
            fwrite(block->buffer, 1, block->buffer_offset, vb_file.fp); // Write the buffer to the file
            VB_DEBUG("Flushed %zu bytes to file for variable: %s", block->buffer_offset, block->header.name);
            block->offset += block->buffer_offset; // Update the offset for the next write
            block->buffer_offset = 0; // Reset the buffer offset
        }
    }
}

void vb2_record_all() {
    if (vb_file.fp == NULL || vb_file.blocks == NULL) {
        return; // File not open or no blocks to record
    }
    if (vb_file.current_history >= vb_file.max_history) {
        VB_DEBUG("Maximum history size reached, cannot record more data.");
        return; // Maximum history size reached
    }
    for (size_t i = 0; i < vb_file.block_count; i++) {
        struct VB_Block_Proxy *block = &vb_file.blocks[i];
        VB_DEBUG("Recording variable: %s, size: %d, offset: %zu", block->header.name, (int)block->var_size, block->offset);
        if (block->var_ptr != NULL && block->var_size > 0) {
            memcpy(block->buffer + block->buffer_offset, block->var_ptr, block->var_size); // Copy the variable data to the buffer
            block->buffer_offset += block->var_size; // Update the buffer offset for the next write
            if (block->buffer_offset >= VB_BUFFER_SIZE) {
                fseek(vb_file.fp, block->offset, SEEK_SET); // Seek to the variable's offset in the file
                fwrite(block->buffer, 1, block->buffer_offset, vb_file.fp); // Write the buffer to the file
                VB_DEBUG("Wrote %zu bytes to file for variable: %s", block->buffer_offset, block->header.name);
                block->offset += block->buffer_offset; // Update the offset for the next write
                block->buffer_offset = 0; // Reset the buffer offset if it exceeds the buffer size
            }
            block->header.count ++; // Increment the count of recorded data
        }
    }
    vb_file.current_history++; // Increment the current history size
}

void vb2_reset() {
    VB_DEBUG("Resetting variable buffer system");
    vb_file.current_history = 0; // Reset the current history size
    for(size_t i = 0; i < vb_file.block_count; i++) {
        struct VB_Block_Proxy *block = &vb_file.blocks[i];
        block->buffer_offset = 0; // Reset the buffer offset for each block
        block->offset = block->header.offset; // Reset the offset to the initial value
        block->header.count = 0; // Reset the count of recorded data
        block->buffer_offset = 0; // Reset the buffer offset
    }
}

void vb2_end() {
    VB_DEBUG("Ending recording session, current history: %zu, max history: %zu", vb_file.current_history, vb_file.max_history);
    vb2_flush_all(); // Flush all recorded data to the file
    vb2_save_var_headers(); // Save the variable headers to the file
    vb2_reset(); // Reset the variable buffer system for the next recording session
    // Safe to open new file or continue recording
    vb_log_close(); // Close the log file
}