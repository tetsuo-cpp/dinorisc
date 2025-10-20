#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DinoRISCTranslator DinoRISCTranslator;

// Create a new translator instance
DinoRISCTranslator *dinorisc_create();

// Destroy a translator instance
void dinorisc_destroy(DinoRISCTranslator *translator);

// Set function arguments (up to 8 arguments)
void dinorisc_set_arguments(DinoRISCTranslator *translator,
                            const uint64_t *args, size_t num_args);

// Execute a specific function from a RISC-V binary
// Returns the function result on success, or -1 on error
int dinorisc_execute_function(DinoRISCTranslator *translator,
                              const char *binary_path,
                              const char *function_name);

// Get last error message (returns NULL if no error)
const char *dinorisc_get_last_error(DinoRISCTranslator *translator);

#ifdef __cplusplus
}
#endif
