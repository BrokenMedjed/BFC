#include "bfc.h"
#include <medjed/types.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>

extern "C" {
#include <unistd.h>
#include <libgen.h>
}

const I8 *usage_short = "usage: %s [-h|-s|-c|-i] [-SC SIZE] [-o OUTPUT] FILE\n";
const I8 *usage_long  = "\
usage: %s [-h|-s|-c|-i] [-SC] [-o OUTPUT] FILE\n\
\t-h\tPrint this message\n\
\t-s\tProduce 64-bit NASM code instead of an executable.\n\
\t-c\tProduce an object file instead of an executable (no linking)\n\
\t-i\tProduce a syntax tree (intermediary representation) for analysis\n\
\t-S\tSet cell size\n\
\t-C\tSet amount of cells allocated to program\n\
\t-o\tSet output file name\n";

#define FLAG_ONLYCOMPILE   1
#define FLAG_ONLYASSEMBLE (1 << 1)
#define FLAG_INTERMEDIATE (1 << 2)
#define FLAG_HASOUTFLAG   (1 << 3)

I32 main(I32 argc, I8 **argv) {
	if (argc < 2) {
		printf(usage_short, argv[0]);
		return EXIT_FAILURE;
	}
	I8  c;
	bfc b;
	U64 cc, cs = 0;
	U8  flags = 0;
	std::string out = "";
	while ((c = getopt(argc, argv, ":hcsiC:S:o:")) > -1) {
		switch(c) {
			case 'h': printf(usage_long, argv[0]); return EXIT_SUCCESS;
			case 'C':
				cc = strtoull(optarg, NULL, 0);
				if (cc > 0xFFFF) {
					printf("%s: argument to -C too big\n", argv[0]);
					return EXIT_FAILURE;
				}
				b.cellcount((U16) cc);
				break;
			case 'S':
				cs = strtoull(optarg, NULL, 0);
				if (cs > 0xFF) {
					printf("%s: argument to -S too big\n", argv[0]);
					return EXIT_FAILURE;
				}
				b.cellsize((U8) cs);
				break;
			case 'c': flags |= FLAG_ONLYCOMPILE;  break;
			case 's': flags |= FLAG_ONLYASSEMBLE; break;
			case 'i': flags |= FLAG_INTERMEDIATE; break;
			case 'o': out = optarg; flags |= FLAG_HASOUTFLAG; break;
			case '?':
				printf("%s: unknown option '-%c'\n", argv[0], optopt);
				return EXIT_FAILURE;
			case ':':
				printf("%s: missing argument to '-%c'", argv[0], optopt);
				return EXIT_FAILURE;
		}
	}
	if (optind > argc - 1) {
		printf(usage_short, argv[0]);
		return EXIT_FAILURE;
	}
	#define OUT_FN(ext) {\
		out = basename(argv[optind]);\
		U32 dotpos = out.find_last_of('.');\
		if (!dotpos) {\
			out += ext;\
		} else\
			out = out.substr(0, dotpos) + ext;\
	}
	b.file(argv[optind]);
	if (b.parse() < 0) {
		fprintf(stderr, "%s: %s @ %lu: %s\n",
			argv[0],
			b.error.where.c_str(),
			b.error.location,
			b.error.desc.c_str()
		);
		return EXIT_FAILURE;
	}
	if (flags & FLAG_INTERMEDIATE) {
		if (!(flags & FLAG_HASOUTFLAG))
			OUT_FN(".ir");
		FILE *f = fopen(out.c_str(), "w");
		if (!f) {
			fprintf(stderr, "%s: can't open '%s'\n", argv[0], out.c_str());
			return EXIT_FAILURE;
		}
		b.dump_ir(f);
		fclose(f);
		return EXIT_SUCCESS;
	}
	if (flags & FLAG_ONLYASSEMBLE) {
		if (!(flags & FLAG_HASOUTFLAG))
			OUT_FN(".asm");
		FILE *f = fopen(out.c_str(), "w");
		if (!f) {
			fprintf(stderr, "%s: can't open '%s'\n", argv[0], out.c_str());
			return EXIT_FAILURE;
		}
		b.nasm64(f);
		fclose(f);
		return EXIT_SUCCESS;
	}
	if (flags & FLAG_ONLYCOMPILE) {
		if (!(flags & FLAG_HASOUTFLAG))
			OUT_FN(".o");
	} else {
		if (!(flags & FLAG_HASOUTFLAG))
			OUT_FN("");
	}
	I8  asm_name[4096] = { 0 };
	I8  obj_name[4096] = { 0 };
	I8  templ[23] = "/tmp/bfc-asm-XXXXXX\0\0\0";
	I32 asm_fd = mkstemp(templ);
	memset(templ, 0, strlen(templ));
	FILE *f = fdopen(asm_fd, "w");
	if (!f) {
		fprintf(stderr, "%s: can't create temporary file\n", argv[0]);
		return EXIT_FAILURE;
	}
	b.nasm64(f);
	std::string path = "/proc/self/fd/";
	path += std::to_string(asm_fd);
	readlink(path.c_str(), asm_name, 4096);
	fclose(f);
	std::string comp_cmd = "nasm -f elf64 ";
	comp_cmd += asm_name;
	comp_cmd += " -o ";
	strncpy(templ, "/tmp/bfc-object-XXXXXX", 23);
	I32 obj_fd = mkstemp(templ);
	path  = "/proc/self/fd/";
	path += std::to_string(obj_fd);
	readlink(path.c_str(), obj_name, 4096);
	close(obj_fd);
	unlink(obj_name);
	std::string obj_path = obj_name;
	obj_path += ".o";
	if (flags & FLAG_ONLYCOMPILE)
		comp_cmd += out;
	else
		comp_cmd += obj_path;
	I8 sysret = system(comp_cmd.c_str());
	unlink(asm_name);
	if (sysret < 0) {
		fprintf(stderr, "%s: system(nasm): execution error\n", argv[0]);
		return EXIT_FAILURE;
	} else if (sysret) {
		fprintf(stderr, "%s: system(nasm): non-zero exit status [%hhd]\n", argv[0], sysret);
		return EXIT_FAILURE;
	}
	if (flags & FLAG_ONLYCOMPILE)
		return EXIT_SUCCESS;
	comp_cmd.clear();
	comp_cmd  = "gcc ";
	comp_cmd += obj_path;
	comp_cmd += " -o ";
	comp_cmd += out;
	sysret = system(comp_cmd.c_str());
	unlink(obj_path.c_str());
	if (sysret < 0) {
		fprintf(stderr, "%s: system(gcc): execution error\n", argv[0]);
		return EXIT_FAILURE;
	} else if (sysret) {
		fprintf(stderr, "%s: system(gcc): non-zero exit status [%hhd]\n", argv[0], sysret);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
