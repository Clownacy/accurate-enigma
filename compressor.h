#ifndef ACCURATE_ENIGMA_COMPRESSOR_H
#define ACCURATE_ENIGMA_COMPRESSOR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int AccurateEngima_Compress(const unsigned char *input_buffer, size_t input_size, void (*write_byte)(unsigned int byte, void *user_data), const void *write_byte_user_data);

#ifdef __cplusplus
}
#endif

#endif /* ACCURATE_ENIGMA_COMPRESSOR_H */
