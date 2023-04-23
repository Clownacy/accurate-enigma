#include "compressor.h"

#include <stdlib.h>
#include <string.h>

#define ACCURATE_ENIGMA_COMPRESS_MAXIMUM_RENDER_FLAGS_BITS 5
#define ACCURATE_ENIGMA_COMPRESS_MAXIMUM_RENDER_FLAGS_VALUES (1u << ACCURATE_ENIGMA_DECOMPRESS_MAXIMUM_RENDER_FLAGS_BITS)
#define ACCURATE_ENIGMA_COMPRESS_REPEAT_BITS 4

#define ACCURATE_ENIGMA_COMPRESS_MIN(A, B) ((A) < (B) ? (A) : (B))

typedef struct AccurateEngima_Compress_State
{
	void (*write_byte)(unsigned int byte, void *user_data);
	void *write_byte_user_data;
	unsigned int bits_remaining;
	unsigned int bits;
} AccurateEngima_Compress_State;

static unsigned int AccurateEngima_Compress_BytesToWord(const unsigned char* const bytes)
{
	return ((unsigned int)bytes[0] << 8) | bytes[1];
}

static int AccurateEngima_Compress_SortComparison(const void* const a, const void* const b)
{
	const unsigned int a_value = AccurateEngima_Compress_BytesToWord((const unsigned char*)a);
	const unsigned int b_value = AccurateEngima_Compress_BytesToWord((const unsigned char*)b);

	if (a_value < b_value)
		return -1;
	else if (a_value > b_value)
		return 1;
	else
		return 0;
}

static unsigned int AccurateEngima_Compress_FindMostCommonWord(const unsigned char* const input_buffer, const size_t input_size)
{
	size_t i;
	unsigned int most_frequent_word, most_frequent_occurances, current_word, current_occurances;

	unsigned char* const sorted_buffer = (unsigned char*)malloc(input_size);

	if (sorted_buffer == NULL)
		return 0;

	memcpy(sorted_buffer, input_buffer, input_size);

	qsort(sorted_buffer, input_size / 2, 2, AccurateEngima_Compress_SortComparison);

	most_frequent_word = 0;
	most_frequent_occurances = 0;
	current_word = 0;
	current_occurances = 0;

	for (i = 0; i < input_size; i += 2)
	{
		const unsigned int new_word = AccurateEngima_Compress_BytesToWord(&sorted_buffer[i]);

		if (current_word == new_word)
		{
			++current_occurances;
		}
		else
		{
			if (current_occurances > most_frequent_occurances)
			{
				most_frequent_occurances = current_occurances;
				most_frequent_word = current_word;
			}

			current_word = new_word;
			current_occurances = 1;
		}
	}

	free(sorted_buffer);

	return most_frequent_word;
}

static void AccurateEngima_Compress_FlushBits(AccurateEngima_Compress_State* const state)
{
	state->write_byte(state->bits, state->write_byte_user_data);

	state->bits_remaining = 8;
	state->bits = 0;
}

static void AccurateEngima_Compress_WriteBit(AccurateEngima_Compress_State* const state, const unsigned int bit)
{
	if (state->bits_remaining == 0)
		AccurateEngima_Compress_FlushBits(state);

	--state->bits_remaining;
	state->bits |= bit << state->bits_remaining;
}

static void AccurateEngima_Compress_WriteBits(AccurateEngima_Compress_State* const state, const unsigned int bits, const unsigned int total_bits)
{
	unsigned int i;

	for (i = 0; i < total_bits; ++i)
		AccurateEngima_Compress_WriteBit(state, (bits & 1u << (total_bits - 1 - i)) != 0);
}

int AccurateEngima_Compress(const unsigned char* const input_buffer, const size_t input_size, void (*write_byte)(unsigned int byte, void *user_data), const void *write_byte_user_data)
{
	if (input_size % 2 != 0)
	{
		return 0; /* Failure: input data ends with partial tile. */
	}
	else
	{
		AccurateEngima_Compress_State state;
		size_t input_index;

		const unsigned int most_common_word = AccurateEngima_Compress_FindMostCommonWord(input_buffer, input_size);

		state.write_byte = write_byte;
		state.write_byte_user_data = (void*)write_byte_user_data;
		state.bits_remaining = 8;
		state.bits = 0;

		/* Header. */
		state.write_byte(16 - ACCURATE_ENIGMA_COMPRESS_MAXIMUM_RENDER_FLAGS_BITS, state.write_byte_user_data);
		state.write_byte((1u << ACCURATE_ENIGMA_COMPRESS_MAXIMUM_RENDER_FLAGS_BITS) - 1, state.write_byte_user_data);
		state.write_byte(0, state.write_byte_user_data);
		state.write_byte(0, state.write_byte_user_data);
		state.write_byte(most_common_word >> 8, state.write_byte_user_data);
		state.write_byte(most_common_word & 0xFF, state.write_byte_user_data);

		input_index = 0;

		while (input_index != input_size)
		{
			unsigned int match_length_same, match_length_increment, match_length_decrement, match_length;
			unsigned int j;

			const unsigned int word = AccurateEngima_Compress_BytesToWord(&input_buffer[input_index]);
			const unsigned int words_to_compare = ACCURATE_ENIGMA_COMPRESS_MIN(1u << ACCURATE_ENIGMA_COMPRESS_REPEAT_BITS, (input_size - input_index) / 2);

			for (match_length_same = 1; match_length_same < words_to_compare; ++match_length_same)
				if (word != AccurateEngima_Compress_BytesToWord(&input_buffer[input_index + match_length_same * 2]))
					break;

			for (match_length_increment = 1; match_length_increment < words_to_compare; ++match_length_increment)
				if (word + match_length_increment != AccurateEngima_Compress_BytesToWord(&input_buffer[input_index + match_length_increment * 2]))
					break;

			for (match_length_decrement = 1; match_length_decrement < words_to_compare; ++match_length_decrement)
				if (word - match_length_decrement != AccurateEngima_Compress_BytesToWord(&input_buffer[input_index + match_length_decrement * 2]))
					break;

			AccurateEngima_Compress_WriteBit(&state, 1);

			if (match_length_same > match_length_increment && match_length_same > match_length_decrement)
			{
				AccurateEngima_Compress_WriteBit(&state, 0);
				AccurateEngima_Compress_WriteBit(&state, 0);
				match_length = match_length_same;
			}
			else if (match_length_increment > match_length_same && match_length_increment > match_length_decrement)
			{
				AccurateEngima_Compress_WriteBit(&state, 0);
				AccurateEngima_Compress_WriteBit(&state, 1);
				match_length = match_length_increment;
			}
			else /*if (match_length_decrement > match_length_increment && match_length_decrement > match_length_same)*/
			{
				AccurateEngima_Compress_WriteBit(&state, 1);
				AccurateEngima_Compress_WriteBit(&state, 0);
				match_length = match_length_decrement;
			}

			AccurateEngima_Compress_WriteBits(&state, match_length - 1, ACCURATE_ENIGMA_COMPRESS_REPEAT_BITS);

			for (j = 0; j < 16; ++j)
				AccurateEngima_Compress_WriteBit(&state, (word & ((1u << 15) >> j)) != 0);

			input_index += match_length * 2;
		}

		/* Termination match. */
		AccurateEngima_Compress_WriteBit(&state, 1);
		AccurateEngima_Compress_WriteBit(&state, 1);
		AccurateEngima_Compress_WriteBit(&state, 1);

		AccurateEngima_Compress_WriteBit(&state, 1);
		AccurateEngima_Compress_WriteBit(&state, 1);
		AccurateEngima_Compress_WriteBit(&state, 1);
		AccurateEngima_Compress_WriteBit(&state, 1);

		AccurateEngima_Compress_FlushBits(&state);

		return 1;
	}
}
