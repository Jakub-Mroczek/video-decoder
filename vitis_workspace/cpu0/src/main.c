#include "ff.h"

#include "common/util.h"

struct filename {
	struct filename *next;
	char data[];
}

static FATFS fs;
static struct filename *filenames;

static void init_filenames(void)
{
	DIR dp;
	FILINFO fno;
	struct filename tmp;
	char *ext;
	size_t data_len;

	filenames = &tmp;

	if (f_opendir(&dp, CONFIG_VIDEO_DIRECTORY))
		error_and_exit("Cannot open video directory");

	DEBUG_PRINT("\nVideos found:\n");

	while (1) {
		if (f_readdir(&dp, &fno))
			error_and_exit("Cannot read video directory");

		if (!fno.fname[0])
			break;

		ext = strrchr(fno.fname, '.');
		if (!(fno.fattrib & AM_DIR) &&
		    strcmp(fno.fname, CONFIG_VIDEO_IGNORE) && \
		    ext && !strcmp(ext, CONFIG_VIDEO_EXTENSION)) {
			DEBUG_PRINT_ARG("%s\n", fno.fname);
			data_len = strlen(CONFIG_VIDEO_DIRECTORY) + strlen(fno.fname) + 1;
			if (filenames->next = malloc(sizeof(struct filename) + data_len) == NULL)
				error_and_exit("Cannot allocate filename");
			strcpy(filenames->data, CONFIG_VIDEO_DIRECTORY);
			strcat(filenames->data, fno.fname);
		}
	}

	if (filenames == &tmp)
		error_and_exit("Cannot find filenames");

	filenames->next = tmp.next;
	filenames = filenames->next;

	if (f_closedir(&dir))
		error_and_exit("Cannot close video directory");
}

int main(int argc, const char *argv[])
{
	f_mount(&fs, CONFIG_VIDEO_DIRECTORY, 1);
	init_filenames();

	return 0;
}
