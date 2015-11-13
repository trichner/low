/* Copyright (c) 2015 Fabian Schuiki */
#include "ast.h"
#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>


static void
parse_options (int *argc, char **argv) {
	char **argi = argv+1;
	char **argo = argv+1;
	char **arge = argv+*argc;
	for (; argi != arge; ++argi) {
		if (*argi[0] == '-') {
			if (strcmp(*argi, "--") == 0) {
				++argi;
				for (; argi != arge; ++argi)
					*argo++ = *argi;
				break;
			} else {
				// TODO: Parse option
				fprintf(stderr, "unknown option %s\n", *argi);
				exit(1);
			}
		} else {
			*argo++ = *argi;
		}
	}
	*argc = argo-argv;
}


static int
compile (const char *filename) {
	int fd = open(filename, O_RDONLY);
	if (fd == -1) {
		perror("open");
		return 1;
	}

	struct stat fs;
	if (fstat(fd, &fs) == -1) {
		perror("fstat");
		close(fd);
		return 1;
	}

	if (!S_ISREG(fs.st_mode)) {
		fprintf(stderr, "not a regular file\n");
		close(fd);
		return 1;
	}

	char *p = mmap(0, fs.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED) {
		perror("mmap");
		close(fd);
		return 1;
	}

	if (close(fd) == -1) {
		perror("close");
		return 1;
	}

	// printf("compiling %s\n", filename);
	lexer_t lex;
	lexer_init(&lex, p, fs.st_size);
	lexer_next(&lex);

	const char *last_slash = strrchr(filename, '/');
	const char *last_dot = strrchr(last_slash, '.');
	unsigned basename_len = (last_dot ? last_dot-filename : strlen(filename));
	char out_name[basename_len+4];
	strncpy(out_name, filename, basename_len);
	strcpy(out_name+basename_len, ".ll");

	array_t *units = parse(&lex);
	if (units) {
		LLVMModuleRef mod = LLVMModuleCreateWithName(filename);
		codegen(mod, units);

		char *error = NULL;
		LLVMVerifyModule(mod, LLVMAbortProcessAction, &error);
		LLVMDisposeMessage(error);

		// LLVMWriteBitcodeToFile(mod, "parsed.bc");
		error = NULL;
		LLVMPrintModuleToFile(mod, out_name, &error);
		LLVMDisposeMessage(error);

		LLVMDisposeModule(mod);

		unsigned i;
		for (i = 0; i < units->size; ++i)
			unit_dispose(array_get(units,i));
		array_dispose(units);
		free(units);
	}

	if (munmap(p, fs.st_size) == -1) {
		perror("munmap");
		return 1;
	}

	return !units || lex.token != TKN_EOF;
}


int
main (int argc, char **argv) {
	/// TODO: Open the file using mmap and perform the lexical analysis.
	/// TODO: Parse the input into an AST.
	/// TODO: Iteratively elaborate the code, executing compile-time code where necessary.
	/// TODO: Generate code.

	parse_options(&argc, argv);

	if (argc == 1) {
		fprintf(stderr, "no input files\n");
		return(1);
	}

	char **arg;
	int any_failed = 0;
	for (arg = argv+1; arg != argv+argc; ++arg) {
		int err = compile(*arg);
		if (err) {
			printf("unable to compile %s\n", *arg);
			any_failed = 1;
		}
	}

	return any_failed;
}
