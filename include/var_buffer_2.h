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
int  vb2_open(const char *filename);
void vb2_close();
void vb2_add_variable(const char *name, const char *unit, const char *description, const char *type, void *var_ptr, uint8_t var_size);
void vb2_start(size_t max_history);
void vb2_record_all();
void vb2_end();

#define vb2_track_variable(var, name, unit, description, type) \
    vb2_add_variable((name), (unit), (description), (type), (void *)(var), sizeof(*var))

#endif // VAR_BUFFER_2_H