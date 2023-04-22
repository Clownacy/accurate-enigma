#ifndef ACCURATE_ENIGMA_DECOMPRESSOR_H
#define ACCURATE_ENIGMA_DECOMPRESSOR_H

#ifdef __cplusplus
extern "C" {
#endif

int AccurateEngima_Decompress(unsigned int (*read_byte)(void *user_data), const void *read_byte_user_data, void (*write_byte)(unsigned int byte, void *user_data), const void *write_byte_user_data);

#ifdef __cplusplus
}
#endif

#endif /* ACCURATE_ENIGMA_DECOMPRESSOR_H */
