#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#define ErrCreate 1
#define ErrParam 2
#define ErrOpening 3
#define LongName 4
#define ErrReading 5
#define ErrWriting 6
#define ErrDirectory 7
#define FileLength 1024
#define BufSize 1024
#define Pack 1
#define Unpack 2

int PackD(int FileDst, int Offset, char *Directory)
{
	DIR *dp;
	struct dirent *entry;
	struct stat statbuf;

	dp = opendir(Directory);
	if (!dp) {
		write(1, "Can't open directory.\n",
		 sizeof("Can't open directory.\n"));
		return ErrDirectory;
	}
	chdir(Directory);
	while ((entry = readdir(dp)) != NULL) {
		lstat(entry->d_name, &statbuf);
		if (S_ISDIR(statbuf.st_mode)) {
			if (!strcmp(".", entry->d_name) ||
				!strcmp("..", entry->d_name))
				continue;
			PackD(FileDst, Offset, entry->d_name);
		} else {
			long int FileSize;
			int i;
			int File;
			char FileName[FileLength] = {0};

			getcwd(FileName, FileLength);
			strcat(FileName, "/");
			strcat(FileName, entry->d_name);
			i = strlen(FileName+Offset);
			if (write(FileDst, FileName+Offset, i) == -1)
				return ErrWriting;
			while (i < FileLength) {
				write(FileDst, "\0", 1);
				i++;
			}
			if (i > FileLength) {
				write(1, "Filename was too long.\n",
				 sizeof("Filename was too long.\n"));
				return LongName;
			}
			File = open(entry->d_name, O_RDONLY);
			if (File != -1) {
				char FileCopyBuf[BufSize];
				int FileCopyed;

				FileSize = lseek(File, 0, SEEK_END);
				lseek(File, 0, SEEK_SET);
				if (write(FileDst, &FileSize,
						  sizeof(long int)) == -1) {
					close(File);
					return ErrWriting;
				}
				while (FileCopyed = read(File, FileCopyBuf,
					     BufSize))
					write(FileDst, FileCopyBuf, FileCopyed);
				close(File);
				} else {
				write(1, "Unable to open file.\n ",
				sizeof("Unable to open file.\n"));
				return ErrOpening;
				}
		}
	}
	chdir("..");
	closedir(dp);
}

char *CreateDirectory(char *Path)
{
	char *TokenSave;
	char *Last = strrchr(Path, '/');
	char *Token = strtok_r(Path, "/", &TokenSave);
	char DirectoryName[FileLength*2] = {0};

	while (Token) {
		if (Token == Last+1) {
			chdir(DirectoryName);
			return Token;
		}

		strcat(DirectoryName, Token);
		strcat(DirectoryName, "/");
		chdir("/");
		mkdir(DirectoryName, 0704);
		Token = strtok_r(NULL, "/", &TokenSave);
	}
}

int Unpck(char *PathSrc, char *Directory)
{
	int FileSrc = open(PathSrc, O_RDONLY);
	long int FileSize;
	int Offset = strlen(Directory);
	char FileName[FileLength*2];

	if (FileSrc == -1) {
		write(1, "Unable to open file.\n",
		sizeof("Unable to open file.\n"));
		return ErrOpening;
	}
	while (1) {
		int File;
		char *Alter;
		char FileCopyBuf[BufSize];

		if (read(FileSrc, FileName+Offset, FileLength) < 1)
			break;
		memcpy(FileName, Directory, Offset);
		Alter = CreateDirectory(FileName);
		File = open(Alter, O_WRONLY|O_CREAT, 0704);
		if (read(FileSrc, &FileSize, sizeof(long int)) == -1) {
			close(File);
			return ErrReading;
		}
		while (1) {
			if (FileSize > (BufSize-1)) {
				if (read(FileSrc, FileCopyBuf, BufSize) == -1)
					break;
				if (write(File, FileCopyBuf, BufSize) == -1)
					break;
				FileSize -= BufSize;
			} else {
				if (read(FileSrc, FileCopyBuf, FileSize) == -1)
					break;
				if (write(File, FileCopyBuf, FileSize) == -1)
					break;
				break;
			}
		}
		close(File);
	}
	write(1, "\n", 1);
	close(FileSrc);
}

int main(int argc, char *argv[])
{
	int i = 0;
	int Mode = 0;
	char PathSrc[256] = {0};
	char PathDst[256] = {0};

	for (i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-mode")) {
			if (i+1 < argc) {
				if (!strcmp(argv[i+1], "pack"))
					Mode = Pack;
				else if (!strcmp(argv[i+1], "unpack"))
					Mode = Unpack;
			}
		}
		if (!strcmp(argv[i], "-src")) {
			if (i+1 < argc)
				strcpy(PathSrc, argv[i+1]);
		}
		if (!strcmp(argv[i], "-dst")) {
			if (i+1 < argc)
				strcpy(PathDst, argv[i+1]);
		}
	}
	if (!Mode || !PathDst[0] || !PathSrc[0]) {
		write(1, "Usage: -mode <(un)pack> -src <path> -dst <path>\n",
		 sizeof("Usage: -mode <(un)pack> -src <path> -dst <path>\n"));
		return ErrParam;
	}
	if (Mode == Pack) {
		int FileDst = open(PathDst, 0704);

		if (FileDst != -1) {
			int Result;

			write(1, "Packing...\n", sizeof("Packing...\n"));
			Result = PackD(FileDst,
					strlen(PathSrc),
					PathSrc);
			close(FileDst);
			if (Result)
				return Result;
		} else {
			write(1, "Unable to open/create destination file.\n",
			   sizeof("Unable to open/create destination file.\n"));
			return ErrCreate;
		}
	} else {
		int Result;

		write(1, "Unpacking...\n", sizeof("Unpacking...\n"));
		Result = Unpck(PathSrc, PathDst);
		if (Result)
			return Result;
	}
	return 0;
}
