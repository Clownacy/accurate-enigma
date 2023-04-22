#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "decompressor.h"

static unsigned int ReadByte(void* const user_data)
{
	const int byte = fgetc((FILE*)user_data);

	return byte == EOF ? -1 : byte;
}

static void WriteByte(const unsigned int byte, void* const user_data)
{
	fputc(byte, (FILE*)user_data);	
}

int main(const int argc, char** const argv)
{
	int exit_code = EXIT_FAILURE;

	if (argc < 3)
	{
		fputs("Usage: [path to executable] [input filename] [output filename]\n", stderr);
	}
	else
	{
		const char* const input_file_path = argv[1];
		FILE* const in_file = fopen(input_file_path, "rb");

		if (in_file == NULL)
		{
			fprintf(stderr, "Error: file '%s' could not be opened for reading.\n", input_file_path);			
		}
		else
		{
			const char* const output_file_path = argv[2];
			FILE* const out_file = fopen(output_file_path, "wb");

			if (out_file == NULL)
			{
				fprintf(stderr, "Error: file '%s' could not be opened for writing.\n", output_file_path);			
			}
			else
			{
				if (!AccurateEngima_Decompress(ReadByte, in_file, WriteByte, out_file))
					fprintf(stderr, "Error: file '%s' is not a valid Enigma archive.\n", input_file_path);			
				else
					exit_code = EXIT_SUCCESS;

				fclose(out_file);
			}

			fclose(in_file);
		}
	}

	return exit_code;
}
