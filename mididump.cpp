// Copyright (C) 2025 Theo Niessink <theo@taletn.com>
// This work is free. You can redistribute it and/or modify it under the
// terms of the Do What The Fuck You Want To Public License, Version 2,
// as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.

// Source: https://www.music.mcgill.ca/~ich/classes/mumt306/StandardMIDIfileformat.html

#include <assert.h>
#include <stdio.h>
#include <string.h>

bool parse_chunk(const unsigned char* const buf, const char* const chunk_type, unsigned int* const chunk_size, const int track_idx = -1)
{
	assert(!strcmp(chunk_type, "MThd") || !strcmp(chunk_type, "MTrk"));

	for (int i = 0; i < 4; ++i)
	{
		const int c = buf[i];
		putchar(c >= 0x20 && c < 0x7F ? c : '?');
	}

	if (track_idx >= 0 && !memcmp(buf, "MTrk", 4))
	{
		printf(" [%d]", track_idx);
	}

	const unsigned int n = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
	*chunk_size = n;

	printf(" (%u bytes)\n\n", n);

	return !memcmp(buf, chunk_type, 4);
}

int read_var_len(int* const value, FILE* const fp, const bool dump = false)
{
	int n = 0, len;

	for (len = 1; len <= 4; ++len)
	{
		const int c = fgetc(fp);

		if (c == EOF)
		{
			len = 0;
			break;
		}

		if (dump) printf(" %02X", c);

		n = (n << 7) | (c & 0x7F);
		if (!(c & 0x80)) break;
	}

	*value = n;
	len = len <= 4 ? len : 4;

	return len;
}

int error(const int ret, FILE* const fp)
{
	if (fp) fclose(fp);
	return ret;
}

int read_error(const char* const filename, FILE* const fp)
{
	fprintf(stderr, "Can't read %s\n", filename);
	return error(3, fp);
}

int main(const int argc, const char* const* const argv)
{
	if (argc != 2 || !strcmp(argv[1], "/?") || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
	{
		printf("Usage: %s <MIDI file>\n", argv[0]);
		return 1;
	}

	FILE* const fp = fopen(argv[1], "rb");

	if (!fp)
	{
		fprintf(stderr, "Can't open %s\n", argv[1]);
		return 2;
	}

	unsigned char buf[14];
	unsigned int size;

	if (fread(buf, 1, 14, fp) != 14)
	{
		return read_error(argv[1], fp);
	}

	if (!parse_chunk(buf, "MThd", &size) || size < 6)
	{
		fprintf(stderr, "Invalid MThd chunk\n");
		return error(4, fp);
	}

	const int format = (buf[8] << 8) | buf[9];
	const int num_tracks = (buf[10] << 8) | buf[11];
	const int division = (short)((buf[12] << 8) | buf[13]);

	printf("File format: %d\n"
	       "Number of tracks: %d\n", format, num_tracks);

	if (division >= 0)
		printf("Ticks per quarter note: %d\n", division);
	else
		printf("SMPTE format: %d\n"
		       "Ticks per frame: %d\n", -division >> 8, division & 0xFF);

	if (size > 6)
	{
		size -= 6;
		printf("\n[Skipping %u bytes]\n", size);

		if (fseek(fp, size, SEEK_CUR))
		{
			return read_error(argv[1], fp);
		}
	}

	for (int i = 0; i < num_tracks;)
	{
		putchar('\n');

		if (fread(buf, 1, 8, fp) != 8)
		{
			return read_error(argv[1], fp);
		}

		const bool track_chunk = parse_chunk(buf, "MTrk", &size, i);
		if (track_chunk) i++;

		unsigned long long abs_time = 0;
		bool end_of_track = !track_chunk;

		while (!end_of_track)
		{
			unsigned int n;
			int delta_time;

			n = read_var_len(&delta_time, fp);
			if (!n || size < n) return read_error(argv[1], fp);
			size -= n;

			abs_time += delta_time;
			printf("%+6d %7llu:  ", delta_time, abs_time);

			const int event = fgetc(fp);
			if (event == EOF || !size--) return read_error(argv[1], fp);

			printf(" %02X", event);

			int len = 0;
			bool text = false;

			switch (event >> 4)
			{
				case 0x8: case 0x9: case 0xA: case 0xB: case 0xE:
				{
					len = 2;
					break;
				}

				case 0xC: case 0xD:
				{
					len = 1;
					break;
				}

				default:
				{
					switch (event)
					{
						case 0xF0: case 0xF7:
						{
							n = read_var_len(&len, fp);
							if (!n || size < n) return read_error(argv[1], fp);
							size -= n;
							break;
						}
						
						case 0xF2:
						{
							len = 2;
							break;
						}

						case 0xF3:
						{
							len = 1;
							break;
						}

						case 0xFF:
						{
							const int meta = fgetc(fp);
							if (meta == EOF || !size--) return read_error(argv[1], fp);

							printf(" %02X", meta);
							text = meta >= 0x01 && meta <= 0x07;

							n = read_var_len(&len, fp, !text);
							if (!n || size < n) return read_error(argv[1], fp);
							size -= n;

							end_of_track = meta == 0x2F /* && len == 0 */;
							break;
						}
					}
				}
			}

			bool quote = false;

			for (int j = 0; j < len; ++j)
			{
				const int c = fgetc(fp);
				if (c == EOF || !size--) return read_error(argv[1], fp);

				if (text && c >= 0x20 && c != '"' && c != '\\' && c < 0x7F)
				{
					if (!quote)
					{
						quote = true;
						printf(" \"");
					}

					putchar(c);
				}
				else
				{
					if (quote)
					{
						quote = false;
						putchar('"');
					}

					printf(" %02X", c);
				}
			}

			if (quote) putchar('"');
			putchar('\n');
		}

		if (size)
		{
			if (track_chunk) putchar('\n');
			printf("[Skipping %u bytes]\n", size);

			if (fseek(fp, size, SEEK_CUR))
			{
				return read_error(argv[1], fp);
			}
		}
	}

	if (fgetc(fp) != EOF || !feof(fp))
	{
		puts("\n[End of file expected]");
	}

	fclose(fp);
	return 0;
}
