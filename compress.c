#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "compressor.h"

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
		FILE* const input_file = fopen(input_file_path, "rb");

		if (input_file == NULL)
		{
			fprintf(stderr, "Error: file '%s' could not be opened for reading.\n", input_file_path);			
		}
		else
		{
			const char* const output_file_path = argv[2];
			FILE* const output_file = fopen(output_file_path, "wb");

			if (output_file == NULL)
			{
				fprintf(stderr, "Error: file '%s' could not be opened for writing.\n", output_file_path);			
			}
			else
			{
				size_t input_file_size;
				unsigned char *input_file_buffer;

				fseek(input_file, 0, SEEK_END);
				input_file_size = ftell(input_file);
				rewind(input_file);
				input_file_buffer = (unsigned char*)malloc(input_file_size);

				if (input_file_buffer == NULL)
				{
					fputs("Error: could not allocate memory for input file buffer.\n", stderr);			
				}
				else
				{
					if (fread(input_file_buffer, input_file_size, 1, input_file) != 1)
					{
						fprintf(stderr, "Error: could not read file '%s'.\n", input_file_path);									
					}
					else
					{
						if (!AccurateEngima_Compress(input_file_buffer, input_file_size, WriteByte, output_file))
							fprintf(stderr, "Error: file '%s' is not a valid Enigma archive.\n", input_file_path);			
						else
							exit_code = EXIT_SUCCESS;
					}

					free(input_file_buffer);
				}

				fclose(output_file);
			}

			fclose(input_file);
		}
	}

	return exit_code;
}
