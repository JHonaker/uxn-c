#include "uxn.h"
#include "common.h"

#ifndef ops_h
#define ops_h

Short op_jmi(Uxn *uxn, Short pc);
Short op_jsi(Uxn *uxn, Short pc);
Short op_jci(Uxn *uxn, Short pc);
Short op_lit(Uxn *uxn, Short pc, bool return_mode, bool short_mode);
Short op_inc(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_pop(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_nip(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_swp(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_rot(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_dup(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_ovr(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_equ(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_neq(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_gth(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_lth(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_jmp(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_jcn(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_jsr(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_sth(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_ldz(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_stz(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode); 
Short op_ldr(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_str(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_lda(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_sta(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_dei(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_deo(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_add(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_sub(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_mul(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_div(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_and(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode); 
Short op_ora(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_eor(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);
Short op_sft(Uxn *uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode);

#endif // ops_h