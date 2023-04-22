#include "compressor.h"

#include <stdlib.h>
#include <string.h>

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

static void AccurateEngima_Compress_WriteBit(AccurateEngima_Compress_State* const state, const unsigned int bit)
{
	if (state->bits_remaining == 0)
	{
		state->write_byte(state->bits, state->write_byte_user_data);

		state->bits_remaining = 8;
		state->bits = 0;
	}

	--state->bits_remaining;
	state->bits |= bit << state->bits_remaining;
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
		size_t i;

		state.write_byte = write_byte;
		state.write_byte_user_data = (void*)write_byte_user_data;
		state.bits_remaining = 8;
		state.bits = 0;

		/* Header. */
		state.write_byte(16 - 5, state.write_byte_user_data);
		state.write_byte((1u << 5) - 1, state.write_byte_user_data);
		state.write_byte(0, state.write_byte_user_data);
		state.write_byte(0, state.write_byte_user_data);
		state.write_byte(0, state.write_byte_user_data);
		state.write_byte(0, state.write_byte_user_data);

		for (i = 0; i < input_size; i += 2)
		{
			unsigned int j;

			const unsigned int word = AccurateEngima_Compress_BytesToWord(&input_buffer[i]);

			AccurateEngima_Compress_WriteBit(&state, 1);
			AccurateEngima_Compress_WriteBit(&state, 1);
			AccurateEngima_Compress_WriteBit(&state, 1);

			AccurateEngima_Compress_WriteBit(&state, 0);
			AccurateEngima_Compress_WriteBit(&state, 0);
			AccurateEngima_Compress_WriteBit(&state, 0);
			AccurateEngima_Compress_WriteBit(&state, 0);

			for (j = 0; j < 16; ++j)
				AccurateEngima_Compress_WriteBit(&state, (word & (0x8000u >> j)) != 0);
		}

		/* Termination match. */
		AccurateEngima_Compress_WriteBit(&state, 1);
		AccurateEngima_Compress_WriteBit(&state, 1);
		AccurateEngima_Compress_WriteBit(&state, 1);

		AccurateEngima_Compress_WriteBit(&state, 1);
		AccurateEngima_Compress_WriteBit(&state, 1);
		AccurateEngima_Compress_WriteBit(&state, 1);
		AccurateEngima_Compress_WriteBit(&state, 1);

		return 1;
	}
}
