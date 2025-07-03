#ifndef VAR_BUFFER_2_H
#define VAR_BUFFER_2_H
#include <stdint.h>
#include <stddef.h>




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define VB2_INT "int"
#define VB2_FLOAT "float"
#define VB2_DOUBLE "double"
#define VB2_LONG "long"

void vb2_init();
void vb2_enable_log();
int  vb2_open(const char *filename);
void vb2_close();
void vb2_add_variable(const char *name, const char *unit, const char *description, const char *type, void *var_ptr, uint8_t var_size);
void vb2_start(size_t max_history);
void vb2_record_all();
void vb2_end();

#define vb2_track_variable(var, name, unit, description, type) \
    vb2_add_variable((name), (unit), (description), (type), (void *)(var), sizeof(*var))

enum severity {
    VB_LOG_DEBUG = 0, // Debug level logging
    VB_LOG_INFO,      // Informational logging
    VB_LOG_WARNING,   // Warning level logging
    VB_LOG_ERROR,     // Error level logging
    VB_LOG_FATAL      // Fatal error logging
};                // Current log level

// Logging Macros
// Usage:
//    VB_DEBUG("Debug message: %s", "This is a debug message");
//    VB_INFO("Info message: %s", "This is an info message");
#define VB_DEBUG(fmt, ...)   vb_log_write(VB_LOG_DEBUG, __LINE__, __FILE__, fmt, ##__VA_ARGS__)
#define VB_INFO(fmt, ...)    vb_log_write(VB_LOG_INFO, __LINE__, __FILE__, fmt, ##__VA_ARGS__)
#define VB_WARNING(fmt, ...) vb_log_write(VB_LOG_WARNING, __LINE__, __FILE__, fmt, ##__VA_ARGS__)
#define VB_ERROR(fmt, ...)   vb_log_write(VB_LOG_ERROR, __LINE__, __FILE__, fmt, ##__VA_ARGS__)
#define VB_FATAL(fmt, ...)   vb_log_write(VB_LOG_FATAL, __LINE__, __FILE__, fmt, ##__VA_ARGS__)

void vb_log_init(const char *filename);
void vb_log_set_echo(int echo_stdout, int echo_stderr);
void vb_log_set_level(enum severity level);
void vb_log_write(enum severity level, size_t lineno, const char *file, const char *fmt, ...);
void vb_log_flush();
void vb_log_close();

#endif // VAR_BUFFER_2_H