

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#ifndef VB_MAX_VAR_SIZE
    #define VB_MAX_VAR_SIZE 8 // Maximum size of a variable in bytes
#endif

#ifdef VB2_DEBUG_ENABLED
    #define VB_DEBUG(fmt, ...) printf("DEBUG[%d]: " fmt "\n", __LINE__, ##__VA_ARGS__)
#else
    #define VB_DEBUG(fmt, ...) (void)0 // No-op if debugging is disabled
#endif

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
    struct VB_Header header;   // Header information for the variable buffer
    void    *var_ptr;          // Pointer to the data of the variable
    uint8_t  var_size;         // Size of the data in bytes
    size_t   offset;            // Offset of the memory block in the file
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

struct VB_File vb_file;

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

    
    vb_file.fp = fopen(vb_file.filename, "wb");
    if (vb_file.fp == NULL) {
        VB_DEBUG("Failed to open file: %s", vb_file.filename);
        return -1; // Failed to open file
    }

    vb_file.block_count = 0;
    return 0; // Success
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
    strncpy(vb_file.blocks[vb_file.block_count].header.unit, unit, sizeof(vb_file.blocks[vb_file.block_count].header.unit) - 1);
    strncpy(vb_file.blocks[vb_file.block_count].header.description, description, sizeof(vb_file.blocks[vb_file.block_count].header.description) - 1);
    strncpy(vb_file.blocks[vb_file.block_count].header.type, type, sizeof(vb_file.blocks[vb_file.block_count].header.type) - 1);
    
    vb_file.blocks[vb_file.block_count].var_ptr = var_ptr;
    vb_file.blocks[vb_file.block_count].var_size = var_size;
    vb_file.blocks[vb_file.block_count].offset = 0; // Initialize offset to zero

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
    vb_file.master_header.max_history = max_history; // Set the maximum history size in the master header
    vb_file.master_header.block_count = vb_file.block_count; // Set the block count in the master header
    strncpy(vb_file.master_header.magic, "VB2", sizeof(vb_file.master_header.magic));
    vb_file.master_header.version = 1; // Set the version of the file format
    fseek(vb_file.fp, 0, SEEK_SET);
    fwrite(&vb_file.master_header, sizeof(vb_file.master_header), 1, vb_file.fp); // Write the master header to the file
    size_t total_size = offset + sizeof(vb_file.master_header); // Calculate the total size of the file
    int s = ftruncate(fileno(vb_file.fp), total_size); // Truncate the file to the new size
    if (s != 0) {
        VB_DEBUG("Failed to truncate file to size %zu", total_size);
        fclose(vb_file.fp);
        vb_file.fp = NULL;
        return; // Failed to truncate the file
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
        VB_DEBUG("Recording variable: %s, size: %d, offset: %zu", block->header.name, block->var_size, block->offset);
        if (block->var_ptr != NULL && block->var_size > 0) {
            fseek(vb_file.fp, block->offset, SEEK_SET);
            fwrite(block->var_ptr, block->var_size, 1, vb_file.fp); // Write the variable data to the file
            block->offset += block->var_size; // Update the offset for the next write
            block->header.count ++; // Increment the count of recorded data
        }
    }
    vb_file.current_history++; // Increment the current history size
}

void vb2_end() {
    VB_DEBUG("Ending recording session, current history: %zu, max history: %zu", vb_file.current_history, vb_file.max_history);\
    vb2_save_var_headers(); // Save the variable headers to the file
    if (vb_file.fp != NULL) {
        fclose(vb_file.fp);
        vb_file.fp = NULL;
    }
    if (vb_file.blocks != NULL) {
        free(vb_file.blocks);
        vb_file.blocks = NULL;
    }
    vb_file.block_count = 0; // Reset the block count
}