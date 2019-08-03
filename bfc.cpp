#include <medjed/types.h>
#include <medjed/debug.h>
#include <exception>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include "bfc.h"

extern "C" {
#include <fcntl.h>
#include <unistd.h>
}

#define DO_DEBUG 0

const I8 *nasm_table[] = {
	// sections, part 1
	"SECTION .data\n"
	"	err_fdmode DB \"w\", 0\n"
	"	err_nomem  DB \"Not enough memory for %d bytes\", 10, 0\n"
	"SECTION .bss\n"
	"	cell_array RESQ 1\n",
	// sections, .text
	"SECTION .text\n"
	"USE64\n"
	"DEFAULT REL\n",
	// _bfc_read for the IN instruction
	"_bfc_read:\n"
	"	PUSH RBX\n"
	"	PUSH RCX\n"
	"	PUSH RDX\n"
	"	PUSH RDI\n"
	"	LEA  RDI, [read_buf]\n"
	"	XOR  RSI, RSI\n"
	"	MOV  RDX, %lu\n"
	"	CALL memset WRT ..plt\n"
	"	POP  RDI\n"
	"	XOR  RAX, RAX\n"
	"	LEA  RSI, [read_buf]\n"
	"	MOV  RDX, RDI\n"
	"	XOR  RDI, RDI\n"
	"	SYSCALL\n"
	"	LEA  RDI, [read_buf]\n"
	"	CALL strlen WRT ..plt\n"
	"	XOR  RDI, RDI\n"
	".l:	MOV  %s,  [RSI]\n"
	"	MOV  [RBX], %s\n"
	"	ADD  RBX, %hhu\n"
	"	ADD  RSI, 1\n"
	"	ADD  RDI, 1\n"
	"	CMP  RDI, RAX\n"
	"	JL   .l\n"
	"	POP  RDX\n"
	"	POP  RCX\n"
	"	POP  RBX\n"
	"	RET\n\n",
	// same but for cell size 1
	"_bfc_read:\n"
	"	PUSH RDX\n"
	"	XOR  RAX, RAX\n"
	"	MOV  RSI, RBX\n"
	"	MOV  RDX, RDI\n"
	"	XOR  RDI, RDI\n"
	"	SYSCALL\n"
	"	POP  RDX\n"
	"	RET\n\n",
	// init
	"GLOBAL main\n"
	"main:\n"
	"	PUSH RBP\n"
	"	MOV  RBP, RSP\n"
	"	SUB  RSP, %d\n"
	"	MOV  RDI, %hhu\n"
	"	MOV  RSI, %hu\n"
	"	CALL calloc WRT ..plt\n"
	"	TEST RAX, RAX\n"
	"	JNZ  .code\n"
	"	MOV  RDI, 2\n"
	"	LEA  RSI, [err_fdmode]\n"
	"	CALL fdopen WRT ..plt\n"
	"	LEA  RSI, [err_nomem]\n"
	"	MOV  RDI, RAX\n"
	"	CALL fputs WRT ..plt\n"
	"	MOV  RAX, 60\n"
	"	MOV  RDI, 1\n"
	"	ADD  RSP, %d\n"
	"	POP  RBP\n"
	"	SYSCALL\n"
	".code:\n"
	"	MOV  [cell_array], RAX\n"
	"	MOV  RBX, RAX\n"
	"	XOR  RAX, RAX\n",
	// exit
	".exit:\n"
	"	MOV  RDI, [cell_array]\n" // reset pointer
	"	CALL free WRT ..plt\n"
	"	MOV  RAX, 60\n"
	"	XOR  RDI, RDI\n"
	"	ADD  RSP, %d\n"
	"	POP  RBP\n"
	"	SYSCALL\n",
	// LST
	"	TEST %s, %s\n"
	"	JZ   .loop_end_%lu\n"
	".loop_%lu:\n",
	// LEN
	"	TEST %s, %s\n"
	"	JNZ  .loop_%lu\n"
	".loop_end_%lu:\n"
};

const char *bfc_cell_size_exception::what() const throw() {
	std::string r = "Invalid cell size: ";
	r += std::to_string(_size);
	return r.c_str();
}

U0 bfc::add_insn(I8 type) {
	insn i = { type, 0 };
	insns.push_back(i);
}

I8 bfc::get_insn_id(I8 op) {
	switch(op) {
		case insn_e::OP_INC: return insn_e::INC;
		case insn_e::OP_DEC: return insn_e::DEC;
		case insn_e::OP_PTI: return insn_e::PTI;
		case insn_e::OP_PTD: return insn_e::PTD;
		case insn_e::OP_OUT: return insn_e::OUT;
		case insn_e::OP_IN : return insn_e::IN;
		case insn_e::OP_LST: return insn_e::LST;
		case insn_e::OP_LEN: return insn_e::LEN;
	}
	return 0;
}

std::string bfc::get_insn_name(I8 type) {
	switch(type) {
		case insn_e::INC: return "INC";
		case insn_e::DEC: return "DEC";
		case insn_e::PTI: return "PTI";
		case insn_e::PTD: return "PTD";
		case insn_e::OUT: return "OUT";
		case insn_e::IN : return "IN ";
		case insn_e::LST: return "LST";
		case insn_e::LEN: return "LEN";
	}
	return "UNK";
}

U0 bfc::_string(const std::string& str) {
	if (str.empty() || fd)
		return;
	s          = str;
	spos       = 0;
	loaded_str = true;
}

U0 bfc::cellsize(U8 cell_size) {
	if (cell_size != 1 &&
	    cell_size != 2 &&
	    cell_size != 4 &&
	    cell_size != 8)
		throw new bfc_cell_size_exception(cell_size);
	csize = cell_size;
}

U0 bfc::cellcount(U16 cell_count) {
	ccount = cell_count;
}

U0 bfc::rbsize(U64 size) {
	if (!size) {
		nasm_rdbuf = size;
		return;
	}
	nasm_rdbuf = size;
}

U0 bfc::string(const I8 *str) {
	_string(std::string(str));
}

U0 bfc::string(const std::string& str) {
	_string(str);
}

U0 bfc::string(const std::string *str) {
	_string(*str);
}

I8 bfc::file(const I8 *path) {
	return (fd = ::open(path, O_RDONLY));
}

I8 bfc::file(const std::string& path) {
	return (fd = ::open(path.c_str(), O_RDONLY));
}

I8 bfc::file(const std::string *path) {
	return (fd = ::open(path->c_str(), O_RDONLY));
}

I8 bfc::next() {
	I8  c;
	U64 pos;
	if (loaded_str) {
		if (spos >= s.size()) {
			finished = true;
			loopd    = 0;
			return 0;
		}
		c   = s[spos++];
		pos = spos;
		goto parse;
	}
	fpos++;
	switch(::read(fd, &c, 1)) {
		case -1:
			return -1;
		case 0:
			if (!loops.empty()) {
				error.location = fpos;
				error.where    = "bfc::next()";
				error.desc     = "unmatched token '";
				error.desc    += insn_e::OP_LST;
				error.desc    += '\'';
				clear();
				return -1;
			}
			finished = true;
			loopd    = 0;
			return 0;
	}
	pos = fpos;
parse:
	switch(c) {
		case ' ': case '\n': case '\r': case '\t': return c;
		case insn_e::OP_LST:
			add_insn(insn_e::LST);
			insns.back().arg = loopd;
			loops.push(loopd++);
			return c;
		case insn_e::OP_LEN:
			if (loops.empty()) {
				error.location = pos;
				error.where    = "bfc::next()";
				error.desc     = "unmatched token '";
				error.desc    += insn_e::OP_LEN;
				error.desc    += '\'';
				if (!loaded_str)
					::close(fd);
				clear();
				return -1;
			}
			add_insn(insn_e::LEN);
			insns.back().arg = loops.top();
			loops.pop();
			return c;
		case insn_e::OP_IN:
			has_in_insn = true;
	}
	if (!insns.empty() && (get_insn_id(c) == insns.back().type)) {
		insns.back().arg++;
		return c;
	}
	if (!get_insn_id(c)) {
		error.location = pos;
		error.where    = "bfc::next()";
		error.desc     = "unknown token '";
		error.desc    += c;
		error.desc    += '\'';
		if (!loaded_str)
			::close(fd);
		clear();
		return -1;
	}
	add_insn(get_insn_id(c));
	insns.back().arg++;
	return c;
}

I8 bfc::parse() {
	while (!finished) {
		if (next() < 0)
			return -1;
	}
	return 0;
}

U0 bfc::clear() {
	finished = 0;
	insns.clear();
	while (!loops.empty())
		loops.pop();
	if (loaded_str) {
		s.clear();
		spos       = 0;
		loaded_str = false;
	} else {
		::close(fd);
		fd   = 0;
		fpos = 0;
	}
}

U0 bfc::dump_ir(FILE *f) {
	std::string n;
	U8 indent = 0;
	fprintf(f, "// BFC IR, %lu instructions\n", insns.size());
	for (insn i:insns) {
		for (U8 i = 0; i < indent; i++)
			fputc(' ', f);
		n = get_insn_name(i.type);
		switch(i.type) {
			case insn_e::LST:
				indent += 2;
				break;
			case insn_e::LEN:
				indent -= 2;
				break;
		}
		fprintf(f, "%s %lu\n", n.c_str(), i.arg);
	}
}

// 64-bit NASM assembly generator
U0 bfc::nasm64(FILE *f) {
	auto getreg = [&](I8 r) {
		std::string rn = "";
		switch(csize) {
			case 1:
				rn += r;
				rn += 'L';
				break;
			case 2:
				rn += r;
				rn += 'X';
				break;
			case 4:
				rn += 'E';
				rn += r;
				rn += 'X';
				break;
			case 8:
				rn += 'R';
				rn += r;
				rn += 'X';
				break;
		}
		return rn.c_str();
	};
	#define WR_INSN(s, ...)\
		fprintf(f, "\t" s "\n", ##__VA_ARGS__);
	#define WRITE_MEM()\
		WR_INSN("MOV  [RBX], %s", getreg('A'))
	#define READ_MEM()\
		WR_INSN("MOV  %s, [RBX]", getreg('A'))
	#define PREV_INS_PTR()\
		(i && (p_ins.type == insn_e::PTI || p_ins.type == insn_e::PTD))
	#define PREV_INS_ARITH()\
		(i && (p_ins.type == insn_e::INC || p_ins.type == insn_e::DEC))
	#define PREV_INS_IO()\
		(i && (p_ins.type == insn_e::OUT || p_ins.type == insn_e::IN))
	#define INS_COMMENT()\
		WR_INSN("; %s %lu", get_insn_name(ins.type).c_str(), ins.arg)
	fprintf(f, "EXTERN ");
	if (csize > 1)
		fprintf(f, "strlen, memset, ");
	fprintf(f, "putchar, free, calloc, fdopen, fputs\n\n");
	fprintf(f, "; BFC x86_64 NASM assembly, compile with '-f elf64'\n");
	fprintf(f, "; %hu cells * %hhu bytes = %d bytes\n\n", ccount, csize, ccount * csize);
	fprintf(f, nasm_table[insn_e::NASM_SECT_DATA], ccount * csize);
	if (has_in_insn) {
		if (csize > 1) {
			WR_INSN("read_buf   RESB %lu", nasm_rdbuf);
			fprintf(f, nasm_table[insn_e::NASM_SECT_TEXT]);
			fprintf(f, nasm_table[insn_e::NASM_READFUNC], nasm_rdbuf, getreg('D'), getreg('D'), csize);
		} else {
			fprintf(f, nasm_table[insn_e::NASM_SECT_TEXT]);
			fprintf(f, nasm_table[insn_e::NASM_READFUNC_CS1]);
		}
	} else
		fprintf(f, nasm_table[insn_e::NASM_SECT_TEXT]);
	fprintf(f, nasm_table[insn_e::NASM_INIT], 32, csize, ccount, 32);
	bool first   = true;
	for (U64 i = 0; i < insns.size(); i++) {
		insn ins = insns[i];
		insn p_ins = { 0, 0 };
		if (i) p_ins = insns[i - 1];
		switch(ins.type) {
			case insn_e::INC:
				INS_COMMENT();
				if (!PREV_INS_PTR() && !PREV_INS_ARITH())
					READ_MEM();
				WR_INSN("ADD  %s, %lu", getreg('A'), ins.arg);
				WRITE_MEM();
				break;
			case insn_e::DEC:
				INS_COMMENT();
				if (!PREV_INS_PTR() && !PREV_INS_ARITH())
					READ_MEM();
				WR_INSN("SUB  %s, %lu", getreg('A'), ins.arg);
				WRITE_MEM();
				break;
			case insn_e::PTI:
				INS_COMMENT();
				if (!PREV_INS_PTR() && !PREV_INS_ARITH())
					WRITE_MEM();
				WR_INSN("ADD  RBX, %lu", ins.arg * csize);
				READ_MEM();
				break;
			case insn_e::PTD:
				INS_COMMENT();
				if (!PREV_INS_PTR() && !PREV_INS_ARITH())
					WRITE_MEM();
				WR_INSN("SUB  RBX, %lu", ins.arg * csize);
				READ_MEM();
				break;
			case insn_e::OUT:
				INS_COMMENT();
				WRITE_MEM();
				for (U64 p = 0; p < ins.arg; p++) {
					WR_INSN("MOV  RDI, [RBX]");
					WR_INSN("CALL putchar WRT ..plt");
				}
				READ_MEM(); // prevent rax clobber
				break;
			case insn_e::IN:
				INS_COMMENT();
				WRITE_MEM();
				WR_INSN("MOV  RDI, %lu", ins.arg + 1);
				WR_INSN("CALL _bfc_read");
				READ_MEM();
				break;
			case insn_e::LST:
				if (!PREV_INS_IO())
					READ_MEM();
				fprintf(f, nasm_table[insn_e::NASM_LST], getreg('A'), getreg('A'), ins.arg, ins.arg);
				break;
			case insn_e::LEN:
				if (!PREV_INS_IO())
					READ_MEM();
				fprintf(f, nasm_table[insn_e::NASM_LEN], getreg('A'), getreg('A'), ins.arg, ins.arg);
				break;
		}
		if (first) first = false;
	}
	fprintf(f, nasm_table[insn_e::NASM_EXIT], 32);
}
