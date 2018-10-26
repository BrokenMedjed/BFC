#pragma once

#include <medjed/types.h>
#include <exception>
#include <vector>
#include <string>
#include <stack>

typedef struct {
	I8  type;
	U64 arg; // BF is simple :3
} insn;

class bfc_cell_size_exception: public std::exception {
	U8 _size;
public:
	bfc_cell_size_exception(U8 size) {
		_size = size;
	}
	virtual const char *what() const throw();
};

enum insn_e: I8 {
	INC = 1, DEC,
	PTI,     PTD,
	OUT,     IN,
	LST,     LEN,

	OP_INC = '+', OP_DEC = '-',
	OP_PTI = '>', OP_PTD = '<',
	OP_OUT = '.', OP_IN  = ',',
	OP_LST = '[', OP_LEN = ']',

	NASM_SECT_DATA = 0, NASM_SECT_TEXT,
	NASM_READFUNC, NASM_READFUNC_CS1,
	NASM_INIT, NASM_EXIT,
	NASM_LST, NASM_LEN
};

extern const I8 *nasm_table[];

class bfc {
	std::stack<U64>   loops;
	std::vector<insn> insns;
	U16  loopd;
	U8   csize;
	U16  ccount; // max 64K cells
	I32  fd;
	U64  fpos;
	std::string s;
	U64  spos;
	bool loaded_str;
	bool has_in_insn;
	U64  nasm_rdbuf;
	U0 _string(const std::string& str);
	U0 add_insn(I8 type);
	I8 get_insn_id(I8 op);
	std::string get_insn_name(I8 type);
public:
	bool finished;
	struct {
		std::string desc;
		std::string where;
		U64 location;
	} error;
	U0 cellsize(U8 cell_size);
	U0 cellcount(U16 cell_count);
	U0 rbsize(U64 size);
	bfc(U16 cell_count = 0x8000, U8 cell_size = 1) {
		cellsize(cell_size);
		ccount      = cell_count;
		nasm_rdbuf  = 512;
		has_in_insn = false;
		fd          = 0;
		loaded_str  = false;
		fpos        = 0;
		spos        = 0;
		finished    = false;
		loopd       = 0;
	}
	U0 string(const I8 *str);
	U0 string(const std::string& str);
	U0 string(const std::string *str);
	I8 file(const I8 *path);
	I8 file(const std::string& path);
	I8 file(const std::string *path);
	I8 next();
	I8 parse();
	U0 clear();
	U0 dump_ir(FILE *f = stdout);
	I8 interp(FILE *f = stdout);
	U0 nasm64(FILE *f = stdout);
};
