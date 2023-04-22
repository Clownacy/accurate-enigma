#include "decompressor.h"

#include <assert.h>
#include <setjmp.h>

#define ACCURATE_ENIGMA_DECOMPRESS_MAXIMUM_RENDER_FLAGS_BITS 5
#define ACCURATE_ENIGMA_DECOMPRESS_MAXIMUM_RENDER_FLAGS_VALUES (1u << ACCURATE_ENIGMA_DECOMPRESS_MAXIMUM_RENDER_FLAGS_BITS)

typedef struct AccurateEngima_Decompress_State
{
	unsigned int (*read_byte)(void *user_data);
	void *read_byte_user_data;
	void (*write_byte)(unsigned int byte, void *user_data);
	void *write_byte_user_data;
	unsigned int bits_remaining;
	unsigned int bits;
	jmp_buf jump_buffer;
	unsigned int bitmasks[ACCURATE_ENIGMA_DECOMPRESS_MAXIMUM_RENDER_FLAGS_VALUES];
} AccurateEngima_Decompress_State;

static unsigned int AccurateEnigma_Decompress_CountBits(unsigned int value)
{
	unsigned int total_bits;

	for (total_bits = 0; value != 0; ++total_bits)
		value &= value - 1;

	return total_bits;
}

static void AccurateEngima_Decompress_WriteWord(AccurateEngima_Decompress_State* const state, const unsigned int word)
{
	state->write_byte(word >> 8, state->write_byte_user_data);
	state->write_byte(word & 0xFF, state->write_byte_user_data);
}

static unsigned int AccurateEngima_Decompress_ReadByte(AccurateEngima_Decompress_State* const state)
{
	unsigned int byte = state->read_byte(state->read_byte_user_data);

	if (byte == (unsigned int)-1)
		longjmp(state->jump_buffer, 1);

	return byte;
}

static unsigned int AccurateEngima_Decompress_ReadBit(AccurateEngima_Decompress_State* const state)
{
	if (state->bits_remaining == 0)
	{
		state->bits = AccurateEngima_Decompress_ReadByte(state);
		state->bits_remaining = 8;
	}

	--state->bits_remaining;

	return (state->bits & (1u << state->bits_remaining)) != 0;
}

static unsigned int AccurateEngima_Decompress_ReadBits(AccurateEngima_Decompress_State* const state, const unsigned int total_bits)
{
	unsigned int accumulator;
	unsigned int i;

	assert(total_bits <= 16);

	accumulator = 0;

	for (i = 0; i < total_bits; ++i)
	{
		accumulator <<= 1;
		accumulator |= AccurateEngima_Decompress_ReadBit(state);
	}

	return accumulator;
}

static unsigned int AccurateEngima_Decompress_ReadInlineCopyValue(AccurateEngima_Decompress_State* const state, const unsigned int total_inline_render_flags_bits, const unsigned int total_inline_copy_value_bits)
{
	const unsigned int inline_render_flags = AccurateEngima_Decompress_ReadBits(state, total_inline_render_flags_bits);
	const unsigned int inline_copy_value = AccurateEngima_Decompress_ReadBits(state, total_inline_copy_value_bits);
	const unsigned int complete_inline_copy_value = state->bitmasks[inline_render_flags] | inline_copy_value;

	return complete_inline_copy_value;
}

int AccurateEngima_Decompress(unsigned int (* const read_byte)(void *user_data), const void* const read_byte_user_data, void (* const write_byte)(unsigned int byte, void *user_data), const void* const write_byte_user_data)
{
	AccurateEngima_Decompress_State state;

	state.read_byte = read_byte;
	state.read_byte_user_data = (void*)read_byte_user_data;
	state.write_byte = write_byte;
	state.write_byte_user_data = (void*)write_byte_user_data;
	state.bits_remaining = 0;

	if (setjmp(state.jump_buffer))
	{
		/* Failure: the input data ended prematurely. */
		return 0;
	}
	else
	{
		const unsigned int total_inline_copy_value_bits = AccurateEngima_Decompress_ReadByte(&state);
		const unsigned int inline_render_flags_bitfield = AccurateEngima_Decompress_ReadByte(&state) % ACCURATE_ENIGMA_DECOMPRESS_MAXIMUM_RENDER_FLAGS_VALUES;
		const unsigned int incremental_copy_word_upper_byte = AccurateEngima_Decompress_ReadByte(&state);
		const unsigned int incremental_copy_word_lower_byte = AccurateEngima_Decompress_ReadByte(&state);
		const unsigned int literal_copy_word_upper_byte = AccurateEngima_Decompress_ReadByte(&state);
		const unsigned int literal_copy_word_lower_byte = AccurateEngima_Decompress_ReadByte(&state);

		const unsigned int total_inline_render_flags_bits = AccurateEnigma_Decompress_CountBits(inline_render_flags_bitfield);
		unsigned int incremental_copy_word = incremental_copy_word_upper_byte << 8 | incremental_copy_word_lower_byte;
		const unsigned int literal_copy_word = literal_copy_word_upper_byte << 8 | literal_copy_word_lower_byte;

		if (total_inline_copy_value_bits >= 12)
		{
			/* Failure: invalid inline copy size. */
			return 0;
		}

		/* Produce bitmasks. */
		{
			unsigned int current_index = 0;
			unsigned int i;

			for (i = 0; i < ACCURATE_ENIGMA_DECOMPRESS_MAXIMUM_RENDER_FLAGS_VALUES; ++i)
				if ((i & ~inline_render_flags_bitfield) == 0)
					state.bitmasks[current_index++] = i << (16 - ACCURATE_ENIGMA_DECOMPRESS_MAXIMUM_RENDER_FLAGS_BITS);
		}

		for (;;)
		{
			unsigned int format;
			unsigned int repeat_count;

			if (AccurateEngima_Decompress_ReadBit(&state) == 0)
			{
				format = AccurateEngima_Decompress_ReadBit(&state);
			}
			else
			{
				format = 2;
				format += AccurateEngima_Decompress_ReadBit(&state) << 1;
				format += AccurateEngima_Decompress_ReadBit(&state) << 0;
			}

			repeat_count = AccurateEngima_Decompress_ReadBits(&state, 4);

			switch (format)
			{
				case 0:
					do
					{
						AccurateEngima_Decompress_WriteWord(&state, incremental_copy_word);
						incremental_copy_word = (incremental_copy_word + 1) & 0xFFFF;
					} while (repeat_count-- != 0);

					break;

				case 1:
					do
					{
						AccurateEngima_Decompress_WriteWord(&state, literal_copy_word);
					} while (repeat_count-- != 0);

					break;

				case 2:
				case 3:
				case 4:
				{
					static const unsigned int deltas[3] = {0, 1, -1};
					const unsigned int delta = deltas[format - 2];
					unsigned int inline_copy_value = AccurateEngima_Decompress_ReadInlineCopyValue(&state, total_inline_render_flags_bits, total_inline_copy_value_bits);

					do
					{
						AccurateEngima_Decompress_WriteWord(&state, inline_copy_value);
						inline_copy_value = (inline_copy_value + delta) & 0xFFFF;
					} while (repeat_count-- != 0);

					break;
				}

				case 5:
					if (repeat_count == (1u << 4) - 1)
						return 1; /* Success. */

					do
					{
						AccurateEngima_Decompress_WriteWord(&state, AccurateEngima_Decompress_ReadInlineCopyValue(&state, total_inline_render_flags_bits, total_inline_copy_value_bits));
					} while (repeat_count-- != 0);

					break;
			}
		}
	}
}

