#include "1801vm1.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

void vm1_init(CPU* cpu) {cpu->gen = 0;}
void vm2_init(CPU* cpu) {cpu->gen = 1;}

void pdp_set_flag(CPU* cpu, int v) {
	cpu->flgC = (v & 1);
	cpu->flgV = !!(v & 2);
	cpu->flgZ = !!(v & 4);
	cpu->flgN = !!(v & 8);
	cpu->flgT = !!(v & 16);
	cpu->flgF7 = !!(v & 128);
	cpu->flgF10 = !!(v & 1024);
	cpu->flgF11 = !!(v & 2048);
}

int pdp_get_flag(CPU* cpu) {
	return cpu->flgC | (cpu->flgV << 1) | (cpu->flgZ << 2) | (cpu->flgN << 3) | (cpu->flgT << 4) | (cpu->flgF7 << 7) | (cpu->flgF10 << 10) | (cpu->flgF11 << 11);
}

// nod: b0:write MSB(1)/LSB(0)
void pdp_wrb(CPU* cpu, int adr, int val) {	// val is 00..FF
	if (adr & 1) {
		cpu->regNOD = 2;			// MSB
		val = (val << 8) & 0xff00;	// move LSB to MSB in val
	} else {
		cpu->regNOD = 1;			// LSB
		val &= 0xff;
	}
	adr &= ~1;
	cpu->mwr(adr, val, cpu->xptr);
}

// nod = 3: write both bytes
void pdp_wr(CPU* cpu, int adr, int val) {
	cpu->regNOD = 3;
	adr &= ~1;
//	if (adr == 0177130) printf("vm wr %.4X,%.4X\n",adr, val);
	cpu->mwr(adr, val, cpu->xptr);
}

int pdp_rd(CPU* cpu, int adr) {
	return cpu->mrd(adr & 0xfffe, 0, cpu->xptr) & 0xffff;
}

// read byte = read word and take high or low byte from there
int pdp_rdb(CPU* cpu, int adr) {
	int wrd = pdp_rd(cpu, adr & ~1);
	return ((adr & 1) ? (wrd >> 8) : wrd) & 0xff;
}

// reset
// R7 = (0xffce + Ncpu * 16) & 0xff00;
// F = 0xe0
void pdp11_reset(CPU* cpu) {
	for (int i = 0; i < 7; i++)
		cpu->regRN(i) = 0;
	cpu->t += 1016;
	cpu->regRN(7) = pdp_rd(cpu, 0xffce) & 0xff00;
	pdp_set_flag(cpu, 0x300); //cpu->f = 0x300;			// b8,9 = 11, cpu0
	cpu->intrq = 0;
	cpu->inten = PDP_INT_IRQ2 | PDP_INT_VIRQ;
	cpu->flgWAIT = 0;
	cpu->flgHALT = 0;
	cpu->timer.flag = 0x01;
}

void pdp_push(CPU* cpu, unsigned short val) {
	cpu->regRN(6) -= 2;
	pdp_wr(cpu, cpu->regRN(6), val);
}

unsigned short pdp_pop(CPU* cpu) {
	unsigned short res = pdp_rd(cpu, cpu->regRN(6));
	cpu->regRN(6) += 2;
	return res;
}

// TODO: some traps will decrease R7 by 2. depends on mcir
// mcir 010,100: decrease R7
// 000: waiting for INT
// 001: no INT
// 01x: halt, irq1, double error, b0:don't R7-=2
// 10x: user trap, b0: don't R7-=2
// 11x: reset cpu

void pdp_trap(CPU* cpu, unsigned short adr) {
	if ((cpu->regMCIR == 2) || (cpu->regMCIR == 4)) {
		cpu->regRN(7) -= 2;
	}
	if ((cpu->regMCIR & 6) == 2) {		// irq1, halt
		//pdp_wr(cpu, 0177676, pdp_get_flag(cpu));
		//pdp_wr(cpu, 0177674, cpu->regRN(7));
		cpu->reg177676 = pdp_get_flag(cpu);
		cpu->reg177674 = cpu->regRN(7);
	} else {
		pdp_push(cpu, pdp_get_flag(cpu));
		pdp_push(cpu, cpu->regRN(7));
	}
	cpu->regRN(7) = pdp_rd(cpu, adr);
	pdp_set_flag(cpu, pdp_rd(cpu, adr+2));
	cpu->regMCIR = 1;
}

int pdp11_int(CPU* cpu) {
	int res = 10;
	cpu->intrq &= cpu->inten;
	if (cpu->intrq && cpu->flgWAIT) {
		cpu->flgWAIT = 0;
		cpu->regRN(7) += 2;
	}
	if (cpu->intrq & PDP_INT_IRQ1) {
		cpu->intrq &= ~PDP_INT_IRQ1;
		if (!cpu->flgF10) {
			cpu->regMCIR = 3;
			// pdp_trap(cpu, 0160002);
		}
	} else if (cpu->intrq & PDP_INT_IRQ2) {
		cpu->intrq &= ~PDP_INT_IRQ2;
		if (!cpu->flgF7) {
			cpu->regMCIR = 5;
			pdp_trap(cpu, 0100);			// #40 = 100(8)
			//cpu->wait = 0;
		}
	} else if (cpu->intrq & PDP_INT_IRQ3) {
		cpu->intrq &= ~PDP_INT_IRQ3;
		if (!cpu->flgF7) {
			cpu->regMCIR = 5;
			pdp_trap(cpu, 0270);			// #b8 = 270(8)
			//cpu->wait = 0;
		}
	} else if (cpu->intrq & PDP_INT_VIRQ) {
		cpu->intrq &= ~PDP_INT_VIRQ;
		//if (!cpu->flgF7) {
			cpu->regMCIR = 5;
			pdp_trap(cpu, cpu->intvec);
			//cpu->wait = 0;
		//}
	} else if (cpu->intrq & PDP_INT_TIMER) {		// timer
		cpu->intrq &= ~PDP_INT_TIMER;
		if (!cpu->flgF7) {
			cpu->regMCIR = 5;
			//pdp_trap(cpu, 0270);
			//cpu->wait = 0;
		}
	} else {
		res = 0;
	}
	return res;
}

// addressation

// 0b = 001011 = @r3
// type: b3..5 addressation type, b0..2 register number

int pdp_adr(CPU* cpu, int type, int b) {
	int res = -1;
	if ((type & 7) == 6) b = 0;	// sp
	if ((type & 7) == 7) b = 0;	// pc
	switch (type & 0x38) {
		case 0x00: res = -1;			// Rn (no addr)
			break;
		case 0x08: res = cpu->regRN(type & 7);	// @Rn
			cpu->t += 13;
			break;
		case 0x10: res = cpu->regRN(type & 7);	// (Rn)+
			cpu->regRN(type & 7) += b ? 1 : 2;
			cpu->t += 12;
			break;
		case 0x18: res = cpu->regRN(type & 7);	// @(Rn)+
			cpu->regRN(type & 7) += 2;
			cpu->t += 12;
			res = pdp_rd(cpu, res & 0xffff);
			cpu->t += 7;
			break;
		case 0x20:				// -(Rn)
			cpu->regRN(type & 7) -= b ? 1 : 2;
			cpu->t += 13;
			res = cpu->regRN(type & 7);
			break;
		case 0x28:				// @-(Rn)
			cpu->regRN(type & 7) -= 2;
			res = cpu->regRN(type & 7);
			cpu->t += 13;
			res = pdp_rd(cpu, res & 0xffff);
			cpu->t += 7;
			break;
		case 0x30:				// E(Rn)
			cpu->t += 12;
			res = pdp_rd(cpu, cpu->regRN(7));	// offset
			cpu->t += 7;
			cpu->regRN(7) += 2;
			res += cpu->regRN(type & 7);		// Rn + offset
			res &= 0xffff;
			break;
		case 0x38:				// @E(Rn)
			cpu->t += 12;
			res = pdp_rd(cpu, cpu->regRN(7));	// offset
			cpu->t += 7;
			cpu->regRN(7) += 2;
			res += cpu->regRN(type & 7);		// Rn + offset
			res = pdp_rd(cpu, res & 0xffff);	// (Rn + offset)
			cpu->t += 7;
			break;
	}
	return res;
}

unsigned short pdp_src(CPU* cpu, int type, int b) {
	unsigned short res;
	int adr = pdp_adr(cpu, type, b);
	if (adr < 0) {
		res = cpu->regRN(type & 7);
		if (b) res &= 0xff;
	} else {
		cpu->regWZ = adr & 0xffff;
		if (b) {
			res = pdp_rdb(cpu, cpu->regWZ);
		} else {
			res = pdp_rd(cpu, cpu->regWZ);
		}
	}
	return res;
}

// write result back (w/o calculating address again), addr is in cpu->regWZ
void pdp_wres(CPU* cpu, int t, unsigned short v) {
	if (t & 0x38) {
		pdp_wr(cpu, cpu->regWZ, v);
	} else {
		cpu->regRN(t & 7) = v;
	}
}

void pdp_wresb(CPU* cpu, int t, unsigned char v) {
	if (t & 0x38) {
		pdp_wrb(cpu, cpu->regWZ, v);
	} else {
		cpu->regRN(t & 7) &= 0xff00;
		cpu->regRN(t & 7) |= (v & 0xff);
	}
}

void pdp_dst(CPU* cpu, unsigned short wrd, int type, int b) {
	cpu->regWZ = pdp_adr(cpu, type, b);
	if (b) {
		pdp_wresb(cpu, type, wrd & 0xff);
	} else {
		pdp_wres(cpu, type, wrd);
	}
}

// commands

static unsigned short twsrc;
static unsigned short twdst;
static int twres;

void pdp_undef(CPU* cpu) {
	printf("undef command %.4X : %.4X\n", cpu->regRN(7) - 2, cpu->com);
//	cpu->xirq(IRQ_BRK, cpu->xptr);
	cpu->regMCIR = 5;
	cpu->regVCEL = 2;
	pdp_trap(cpu, 010);
}

// 000x

// 0000:halt
void pdp_halt(CPU* cpu) {
//	cpu->mcir = 3;	// 011		// 01x - store pc/f @ 0177674
//	cpu->vsel = 11;	// 1011
//	printf("halt\n");
	// cpu->halt = 1;
//	pdp_trap(cpu, 0160002);
}

// 0001:flgWAIT
void pdp_wait(CPU* cpu) {
	// cpu->mcir = 0;
	cpu->flgWAIT = 1;
	cpu->regRN(7) -= 2;
}

// 0002:rti
void pdp_rti(CPU* cpu) {
	cpu->t += 3;
	cpu->regRN(7) = pdp_rd(cpu, cpu->regRN(6)) & 0xffff;
	cpu->t += 8;
	cpu->regRN(6) += 2;
	pdp_set_flag(cpu, pdp_rd(cpu, cpu->regRN(6)) & 0xff);	// ffff ?
	cpu->t += 8;
	cpu->regRN(6) += 2;
//	cpu->f &= 0xff;
	if (cpu->flgT) {
		cpu->flgT = 0;		// RTI/RTT clears T-flag
		cpu->regMCIR = 5;
		cpu->regVCEL = 3;
		pdp_trap(cpu, 014);
	}
}

// 0003:bpt
void pdp_bpt(CPU* cpu) {
	cpu->regMCIR = 5;
	cpu->regVCEL = 3;
	pdp_trap(cpu, 014);		// 14(8) = 12(10)
}

// 0004:iot
void pdp_iot(CPU* cpu) {
	cpu->regMCIR = 5;
	cpu->regVCEL = 1;
	pdp_trap(cpu, 020);		// 20(8) = 16(10)
}

// 0005:reset
void pdp_res(CPU* cpu) {
	cpu->t += 448;
	cpu->timer.val = 0xffff;
	cpu->timer.ival = 0xffff;
	cpu->timer.flag = 0xff;
	cpu->xirq(PDP11_INIT, cpu->xptr);	// cpu->iwr(0, PDP11_INIT, cpu->xptr);
}

// 0006:rtt
void pdp_rtt(CPU* cpu) {
	cpu->regRN(7) = pdp_rd(cpu, cpu->regRN(6)) & 0xffff;
	cpu->regRN(6) += 2;
	pdp_set_flag(cpu, pdp_rd(cpu, cpu->regRN(6)) & 0xff);	// ffff
	cpu->regRN(6) += 2;
	// cpu->f &= 0xff;
	cpu->flgT = 0;
}

// 0007..000A : start

void pdp_start(CPU* cpu) {
	cpu->regRN(7) = pdp_rd(cpu, 0177674) & 0xffff;
	pdp_set_flag(cpu, pdp_rd(cpu, 0177676) & 0xffff);
	cpu->regWZ = pdp_rd(cpu, 0177716) & 0xffff;
	cpu->regWZ &= ~8;
	pdp_wr(cpu, 0177716, cpu->regWZ);
}

// 000B..000F : step

void pdp_step(CPU* cpu) {
	cpu->regRN(7) = pdp_rd(cpu, 0177674) & 0xffff;
	pdp_set_flag(cpu, pdp_rd(cpu, 0177676) & 0xffff);
	cpu->regWZ = pdp_rd(cpu, 0177716) & 0xffff;
	cpu->regWZ &= ~8;
	pdp_wr(cpu, 0177716, cpu->regWZ);
}

static cbcpu pdp_000n_tab[16] = {
	pdp_halt, pdp_wait, pdp_rti, pdp_bpt,
	pdp_iot, pdp_res, pdp_rtt, pdp_undef,
	pdp_start, pdp_start, pdp_start, pdp_start,
	pdp_step, pdp_step, pdp_step, pdp_step
};

void pdp_000x(CPU* cpu) {
	pdp_000n_tab[cpu->com & 0x0f](cpu);
}

//0000 0000 01dd dddd	jmp		r7 = [dd]
void pdp_jmp(CPU* cpu) {
	twres = pdp_adr(cpu, cpu->com, 0);
	if (twres < 0) {
		cpu->regMCIR = 5;
		cpu->regVCEL = 4;
		pdp_trap(cpu, 4);
	} else {
		cpu->regRN(7) = twres & 0xffff;
	}
}

//0000 0000 1000 0rrr	rts		r7=reg:pop reg
//0000 0000 1000 1xxx	?
void pdp_008x(CPU* cpu) {
	cpu->t += 8;
	if (cpu->com & 8) {
		pdp_undef(cpu);
	} else {
		// cpu->mcir = 3;
		cpu->regRN(7) = cpu->regRN(cpu->com & 7);
		cpu->regRN(cpu->com & 7) = pdp_rd(cpu, cpu->regRN(6)) & 0xffff;
		cpu->t += 8;
		cpu->regRN(6) += 2;
	}
}

// 0000 0000 1001 xxxx	?

// 0000 0000 1010 nzvc	clear flags
void pdp_cl(CPU* cpu) {
	int f = pdp_get_flag(cpu);
	f &= ~(cpu->com & 0x0f);
	pdp_set_flag(cpu, f);
}

// 0000 0000 1011 nzvc	set flags
void pdp_se(CPU* cpu) {
	int f = pdp_get_flag(cpu);
	f |= (cpu->com & 0x0f);
	pdp_set_flag(cpu, f);
}

// 0000 0000 11dd dddd	swab		swap hi/lo bytes in [dd]
void pdp_swab(CPU* cpu) {
	// cpu->mcir = 7;
	twsrc = pdp_src(cpu, cpu->com, 0);
	twdst = ((twsrc << 8) & 0xff00) | ((twsrc >> 8) & 0xff);
	//cpu->f &= ~(PDP_FC | PDP_FV | PDP_FN | PDP_FZ);	// reset c,v
	cpu->flgC = 0;
	cpu->flgV = 0;
	cpu->flgN = !!(twdst & 0x80);
	cpu->flgZ = !(twdst & 0xff);
	pdp_wres(cpu, cpu->com, twdst);
}

static cbcpu pdp_00nx_tab[16] = {
	pdp_000x, pdp_undef, pdp_undef, pdp_undef,
	pdp_jmp, pdp_jmp, pdp_jmp, pdp_jmp,
	pdp_008x, pdp_undef, pdp_cl, pdp_se,
	pdp_swab, pdp_swab, pdp_swab, pdp_swab
};

void pdp_00xx(CPU* cpu) {
	pdp_00nx_tab[(cpu->com >> 4) & 0x0f](cpu);
}

// conditional jumps

void pdp_jr(CPU* cpu) {
	twsrc = (cpu->com << 1) & 0x1fe;
	if (twsrc & 0x100)
		twsrc |= 0xff00;
	cpu->regRN(7) += twsrc;
}

// br
void pdp_01xx(CPU* cpu) {
	cpu->t += 12;
	pdp_jr(cpu);
}

// bne
void pdp_02xx(CPU* cpu) {
	cpu->t += 12;
	if (!cpu->flgZ)
		pdp_jr(cpu);
}

// beq
void pdp_03xx(CPU* cpu) {
	cpu->t += 12;
	if (cpu->flgZ)
		pdp_jr(cpu);
}

// bge
void pdp_04xx(CPU* cpu) {
	cpu->t += 12;
//	twres = cpu->flgN;
//	if (cpu->flgV) twres ^= 1;
	if (!(cpu->flgN ^ cpu->flgV))
		pdp_jr(cpu);
}

// blt
void pdp_05xx(CPU* cpu) {
	cpu->t += 12;
//	twres = (cpu->f & PDP_FN) ? 1 : 0;
//	if (cpu->f & PDP_FV) twres ^= 1;
	if (cpu->flgN ^ cpu->flgV)
		pdp_jr(cpu);
}

// bgt
void pdp_06xx(CPU* cpu) {
	cpu->t += 12;
//	twres = (cpu->f & PDP_FN) ? 1 : 0;
//	if (cpu->f & PDP_FV) twres ^= 1;
//	if (cpu->f & PDP_FZ) twres |= 1;
	if (!(cpu->flgZ | (cpu->flgN ^ cpu->flgV)))
		pdp_jr(cpu);
}

// ble
void pdp_07xx(CPU* cpu) {
	cpu->t += 12;
//	twres = (cpu->f & PDP_FN) ? 1 : 0;
//	if (cpu->f & PDP_FV) twres ^= 1;
//	if (cpu->f & PDP_FZ) twres |= 1;
	if (cpu->flgZ | (cpu->flgN ^ cpu->flgV))
		pdp_jr(cpu);
}

// 0000 100r rrdd dddd	jsr		push reg:reg=r7:r7=[dd]
// !!! if addressation method = 0, exception (4)
void pdp_jsr(CPU* cpu) {
	twres = pdp_adr(cpu, cpu->com, 0);
	if (twres < 0) {		// addr type 0: exception
		cpu->regMCIR = 5;
		cpu->regVCEL = 4;
		pdp_trap(cpu, 4);
	} else {
		// cpu->mcir = 4;
		twsrc = (cpu->com >> 6) & 7;
		cpu->regRN(6) -= 2;
		pdp_wr(cpu, cpu->regRN(6), cpu->regRN(twsrc));
		cpu->regRN(twsrc) = cpu->regRN(7);
		cpu->regRN(7) = twres & 0xffff;
	}
}

//0000 1010 00dd dddd	clr		dd = 0
void pdp_clr(CPU* cpu) {
//	cpu->mcir = (cpu->com & 0x38) ? 7 : 5;
	pdp_dst(cpu, 0, cpu->com, 0);
//	cpu->f &= ~(PDP_FC | PDP_FN | PDP_FV);
//	cpu->f |= PDP_FZ;
	cpu->flgC = 0;
	cpu->flgN = 0;
	cpu->flgV = 0;
	cpu->flgZ = 1;
}

//0000 1010 01dd dddd	com		invert all bits (cpl)
void pdp_com(CPU* cpu) {
//	cpu->mcir = (cpu->com & 0x38) ? 7 : 5;
	twsrc = pdp_src(cpu, cpu->com, 0);
	twsrc ^= 0xffff;
	pdp_wres(cpu, cpu->com, twsrc);
//	cpu->f |= PDP_FC;
	cpu->flgZ = !twsrc;
	cpu->flgN = !!(twsrc & 0x8000);
	cpu->flgC = 1;
	cpu->flgV = 0;
//	cpu->f |= PDP_FC;
}

//0000 1010 10dd dddd	inc
void pdp_inc(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 0);
	twsrc++;
	pdp_wres(cpu, cpu->com, twsrc);
//	cpu->f &= ~(PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgZ = !twsrc;
	cpu->flgN = !!(twsrc & 0x8000);
	cpu->flgV = !!(twsrc == 0x8000);
}

//0000 1010 11dd dddd	dec
void pdp_dec(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 0);
	twsrc--;
	pdp_wres(cpu, cpu->com, twsrc);
//	cpu->f &= ~(PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgZ = !twsrc;
	cpu->flgN = !!(twsrc & 0x8000);
	cpu->flgV = !!(twsrc == 0x7fff);
}

static cbcpu pdp_0axx_tab[4] = {pdp_clr, pdp_com, pdp_inc, pdp_dec};

void pdp_0axx(CPU* cpu) {
	pdp_0axx_tab[(cpu->com >> 6) & 3](cpu);
}

//0000 1011 00dd dddd	neg		[dd] = -[dd]
//0000 1011 01dd dddd	adc		[dd] = [dd] + C
//0000 1011 10dd dddd	sbc		[dd] = [dd] - C
//0000 1011 11dd dddd	tst
void pdp_neg(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 0);
	cpu->flgV = !!(twsrc == 0x8000);
	twsrc = ~twsrc + 1;
	pdp_wres(cpu, cpu->com, twsrc);
//	cpu->f &= ~(PDP_FN | PDP_FV | PDP_FZ | PDP_FC);
	cpu->flgZ = !twsrc;			// TODO: check flags
	cpu->flgN = !!(twsrc & 0x8000);
	// cpu->flgC = !!(twsrc != 0);
}

void pdp_adc(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 0);
	if (cpu->flgC)
		twsrc++;
	pdp_wres(cpu, cpu->com, twsrc);
	//cpu->f &= ~(PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgZ = !twsrc;
	cpu->flgN = !!(twsrc & 0x8000);
	cpu->flgV = !!(twsrc == 0x8000);
	if (cpu->flgC) {
		if (twsrc) cpu->flgC = 0;
	}
}

void pdp_sbc(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 0);
	if (cpu->flgC)
		twsrc--;
	pdp_wres(cpu, cpu->com, twsrc);
	//cpu->f &= ~(PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgZ = !twsrc;
	cpu->flgN = !!(twsrc & 0x8000);
	cpu->flgV = 0;
	if (cpu->flgC) {
		if (twsrc != 0xffff) cpu->flgC = 0;
		if (twsrc == 0x7fff) cpu->flgV = 1;
	}
}

void pdp_tst(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 0);
	//cpu->f &= ~(PDP_FN | PDP_FV | PDP_FC | PDP_FZ);
	cpu->flgC = 0;
	cpu->flgV = 0;
	cpu->flgZ = !twsrc;
	cpu->flgN = !!(twsrc & 0x8000);
}

static cbcpu pdp_0bxx_tab[4] = {pdp_neg, pdp_adc, pdp_sbc, pdp_tst};

void pdp_0bxx(CPU* cpu) {
	pdp_0bxx_tab[(cpu->com >> 6) & 3](cpu);
}

//0000 1100 00dd dddd	ror
void pdp_ror(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 0);
	cpu->tmpw = cpu->flgC;
	//cpu->f &= ~(PDP_FC | PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgC = twsrc & 1;
	twsrc >>= 1;
	if (cpu->tmpw) twsrc |= 0x8000;
	cpu->flgN = !!(twsrc & 0x8000);
	cpu->flgZ = !twsrc;
	cpu->flgV = cpu->flgC ^ cpu->flgN;
	pdp_wres(cpu, cpu->com, twsrc);
}

//0000 1100 01dd dddd	rol
void pdp_rol(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 0);
	cpu->tmpw = cpu->flgC;
	//cpu->f &= ~(PDP_FC | PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgC = !!(twsrc & 0x8000);
	twsrc <<= 1;
	if (cpu->tmpw) twsrc |= 1;
	cpu->flgN = !!(twsrc & 0x8000);
	cpu->flgZ = !twsrc;
	cpu->flgV = cpu->flgC ^ cpu->flgN;
	pdp_wres(cpu, cpu->com, twsrc);
}

//0000 1100 10dd dddd	asr
void pdp_asr(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 0);
	//cpu->f &= ~(PDP_FC | PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgC = twsrc & 1;
	twsrc >>= 1;
	if (twsrc & 0x4000) twsrc |= 0x8000;
	cpu->flgN = !!(twsrc & 0x8000);
	cpu->flgZ = !twsrc;
	cpu->flgV = cpu->flgC ^ cpu->flgN;
	pdp_wres(cpu, cpu->com, twsrc);
}

//0000 1100 11dd dddd	arl
void pdp_asl(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 0);
	//cpu->f &= ~(PDP_FC | PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgC = !!(twsrc & 0x8000);
	twsrc <<= 1;
	cpu->flgN = !!(twsrc & 0x8000);
	cpu->flgZ = !twsrc;
	cpu->flgV = cpu->flgC ^ cpu->flgN;
	pdp_wres(cpu, cpu->com, twsrc);
}

static cbcpu pdp_0cxx_tab[4] = {pdp_ror, pdp_rol, pdp_asr, pdp_asl};

void pdp_0cxx(CPU* cpu) {
	pdp_0cxx_tab[(cpu->com >> 6) & 3](cpu);
}

//0000 1101 00nn nnnn	mark nn
//0000 1101 01ss ssss	mfpi		push [ss]
//0000 1101 10dd dddd	mtpi		pop [dd]
//0000 1101 11dd dddd	sxt		dst = N ? -1 : 0

void pdp_mark(CPU* cpu) {
	cpu->regRN(6) = cpu->regRN(7) + (cpu->com & 0x3f) * 2;
	cpu->regRN(7) = cpu->regRN(5);
	cpu->regRN(5) = pdp_pop(cpu);
}

void pdp_mfpi(CPU* cpu) {
	pdp_undef(cpu);
}

void pdp_mtpi(CPU* cpu) {
	pdp_undef(cpu);
}

void pdp_sxt(CPU* cpu) {
	twdst = cpu->flgN ? 0xffff : 0x0000;
	//cpu->f &= ~(PDP_FZ | PDP_FV);
	cpu->flgZ = !cpu->flgN;
	// cpu->flgV = 0;
	pdp_dst(cpu, twdst, cpu->com, 0);
}

static cbcpu pdp_0dxx_tab[4] = {pdp_mark, pdp_mfpi, pdp_mtpi, pdp_sxt};

void pdp_0dxx(CPU* cpu) {
	pdp_0dxx_tab[(cpu->com >> 6) & 3](cpu);
}

static cbcpu pdp_tab_0nxx[16] = {
	pdp_00xx, pdp_01xx, pdp_02xx, pdp_03xx,
	pdp_04xx, pdp_05xx, pdp_06xx, pdp_07xx,
	pdp_jsr, pdp_jsr, pdp_0axx, pdp_0bxx,
	pdp_0cxx, pdp_0dxx, pdp_undef, pdp_undef
};

// 00xxxx
void pdp_0xxx(CPU* cpu) {
	pdp_tab_0nxx[(cpu->com >> 8) & 0x0f](cpu);
}

// bpl
void pdp_80xx(CPU* cpu) {
	cpu->t += 12;
	if (!cpu->flgN)
		pdp_jr(cpu);
}

// bmi
void pdp_81xx(CPU* cpu) {
	cpu->t += 12;
	if (cpu->flgN)
		pdp_jr(cpu);
}

// bhi
void pdp_82xx(CPU* cpu) {
	cpu->t += 12;
	if (!(cpu->flgC | cpu->flgZ)) // (cpu->f & PDP_FC) || (cpu->f & PDP_FZ)))
		pdp_jr(cpu);
}

// blos
void pdp_83xx(CPU* cpu) {
	cpu->t += 12;
	if (cpu->flgC | cpu->flgZ) // (cpu->f & PDP_FC) || (cpu->f & PDP_FZ))
		pdp_jr(cpu);
}

// bvc
void pdp_84xx(CPU* cpu) {
	cpu->t += 12;
	if (!cpu->flgV)
		pdp_jr(cpu);
}

// bvs
void pdp_85xx(CPU* cpu) {
	cpu->t += 12;
	if (cpu->flgV)
		pdp_jr(cpu);
}

// bcc (bhis)
void pdp_86xx(CPU* cpu) {
	cpu->t += 12;
	if (!cpu->flgC)
		pdp_jr(cpu);
}

// bcs (blo)
void pdp_87xx(CPU* cpu) {
	cpu->t += 12;
	if (cpu->flgC)
		pdp_jr(cpu);
}

// 10xxxx

// emt
void pdp_88xx(CPU* cpu) {
	cpu->regMCIR = 5;
	cpu->regVCEL = 6;
	pdp_trap(cpu, 24);	// 30(8) = 24(10)
}

// trap
void pdp_89xx(CPU* cpu) {
	cpu->regMCIR = 5;
	cpu->regVCEL = 12;
	pdp_trap(cpu, 28);	// 34(8) = 28(10)
}

//1000 1010 00dd dddd	clrb
//1000 1010 01dd dddd	comb
//1000 1010 10dd dddd	incb
//1000 1010 11dd dddd	decb

void pdp_clrb(CPU* cpu) {
	pdp_dst(cpu, 0, cpu->com, 1);
	cpu->flgN = 0;
	cpu->flgC = 0;
	cpu->flgV = 0;
	cpu->flgZ = 1;
}

// FC = 1 !!!
void pdp_comb(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 1);
	twdst = twsrc ^ 0xff;
	//cpu->f &= ~(PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgV = 0;
	cpu->flgC = 1;
	cpu->flgN = !!(twdst & 0x80);
	cpu->flgZ = !(twdst & 0xff);
	pdp_wresb(cpu, cpu->com, twdst);
}

void pdp_incb(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 1);
	twdst = (twsrc + 1) & 0xff;
	// cpu->f &= ~(PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgN = !!(twdst & 0x80);
	cpu->flgZ = !twdst;
	cpu->flgV = !!(twdst == 0x80);	// 7f->80
	pdp_wresb(cpu, cpu->com, twdst);
}

void pdp_decb(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 1);
	twdst = (twsrc - 1) & 0xff;
	//cpu->f &= ~(PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgN = !!(twdst & 0x80);
	cpu->flgZ = !twdst;
	cpu->flgV = !!(twdst == 0x7f);	// 80->7f
	pdp_wresb(cpu, cpu->com, twdst);
}

static cbcpu pdp_8axx_tab[4] = {pdp_clrb, pdp_comb, pdp_incb, pdp_decb};

void pdp_8axx(CPU* cpu) {
	pdp_8axx_tab[(cpu->com >> 6) & 3](cpu);
}

//1000 1011 00dd dddd	negb
//1000 1011 01dd dddd	adcb
//1000 1011 10dd dddd	sbcb
//1000 1011 11dd dddd	tstb

void pdp_negb(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 1);
	twdst = (0 - twsrc) & 0xff;
	// cpu->f &= ~(PDP_FC | PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgZ = !twdst;
	cpu->flgC = !!twdst;
	cpu->flgN = !!(twdst & 0x80);
	cpu->flgV = !!(twdst == 0x80);
	pdp_wresb(cpu, cpu->com, twdst);
}

void pdp_adcb(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 1);
	twdst = twsrc;
	if (cpu->flgC) twdst++;
	twdst &= 0xff;
	//cpu->f &= ~(PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgZ = !twdst;
	cpu->flgN = !!(twdst & 0x80);
	cpu->flgV = 0;
	if (cpu->flgC) {
		if (twdst) cpu->flgC = 0;
		if (twdst == 0x80) cpu->flgV = 1;
	}
	pdp_wresb(cpu, cpu->com, twdst);
}

void pdp_sbcb(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 1);
	twdst = twsrc;
	if (cpu->flgC) twdst--;
	twdst &= 0xff;
//	twsrc &= 0xff00;
//	twsrc |= twdst;
	//cpu->f &= ~(PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgZ = !twdst;
	cpu->flgN = !!(twdst & 0x80);
	cpu->flgV = 0;
	if (cpu->flgC) {
		if (twdst != 0xff) cpu->flgC = 0;
		if (twdst == 0x7f) cpu->flgV = 1;
	}
	pdp_wresb(cpu, cpu->com, twdst);
//	if (cpu->com & 070) {
//		pdp_wrb(cpu, cpu->mptr, twsrc & 0xff);
//	} else {
//		cpu->preg[cpu->com & 7] = twsrc;
//	}
}

void pdp_tstb(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 1);
	//cpu->f &= ~(PDP_FC | PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgV = 0;
	cpu->flgC = 0;
	cpu->flgZ = !twsrc;
	cpu->flgN = !!(twsrc & 0x80);
}

static cbcpu pdp_8bxx_tab[4] = {pdp_negb, pdp_adcb, pdp_sbcb, pdp_tstb};

void pdp_8bxx(CPU* cpu) {
	pdp_8bxx_tab[(cpu->com >> 6) & 3](cpu);
}

//1000 1100 00dd dddd	rorb
//1000 1100 01dd dddd	rolb
//1000 1100 10dd dddd	asrb
//1000 1100 11dd dddd	aslb

void pdp_rorb(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 1);
	twdst = twsrc & 0xff;
	cpu->tmpw = cpu->flgC;
	//cpu->f &= ~(PDP_FC | PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgC = twdst & 1;
	twdst >>= 1;
	if (cpu->tmpw) twdst |= 0x80;
	twdst &= 0xff;
	cpu->flgZ = !twdst;
	cpu->flgN = !!(twdst & 0x80);
	cpu->flgV = cpu->flgC ^ cpu->flgN; //if (((cpu->f & PDP_FC) ? 1 : 0) ^ ((cpu->f & PDP_FN) ? 1 : 0)) cpu->f |= PDP_FV;
	pdp_wresb(cpu, cpu->com, twdst);
}

void pdp_rolb(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 1);
	twdst = twsrc & 0xff;
	cpu->tmpw = cpu->flgC;
	//cpu->f &= ~(PDP_FC | PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgC = !!(twdst & 0x80);
	twdst <<= 1;
	twdst &= 0xfe;
	if (cpu->tmpw) twdst |= 1;
	cpu->flgZ = !twdst;
	cpu->flgN = !!(twdst & 0x80);
	cpu->flgV = cpu->flgC ^ cpu->flgN; //if (((cpu->f & PDP_FC) ? 1 : 0) ^ ((cpu->f & PDP_FN) ? 1 : 0)) cpu->f |= PDP_FV;
	pdp_wresb(cpu, cpu->com, twdst);
}

void pdp_asrb(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 1);
	twdst = twsrc & 0xff;
	cpu->tmpw = cpu->flgC;
	//cpu->f &= ~(PDP_FC | PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgC = twdst & 1;
	twdst >>= 1;
	if (twdst & 0x40) twdst |= 0x80;
	twdst &= 0xff;
	cpu->flgZ = !twdst;
	cpu->flgN = !!(twdst & 0x80);
	cpu->flgV = cpu->flgC ^ cpu->flgN; // if (((cpu->f & PDP_FC) ? 1 : 0) ^ ((cpu->f & PDP_FN) ? 1 : 0) cpu->f |= PDP_FV;
	pdp_wresb(cpu, cpu->com, twdst);
}

void pdp_aslb(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 1);
	twdst = twsrc & 0xff;
	cpu->tmpw = cpu->flgC;
	//cpu->f &= ~(PDP_FC | PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgC = !!(twdst & 0x80);
	twdst <<= 1;
	twdst &= 0xfe;
	cpu->flgZ = !twdst;
	cpu->flgN = !!(twdst & 0x80);
	cpu->flgV = cpu->flgC ^ cpu->flgN; //if (((cpu->f & PDP_FC) ? 1 : 0) ^ ((cpu->f & PDP_FN) ? 1 : 0)) cpu->f |= PDP_FV;
	pdp_wresb(cpu, cpu->com, twdst);
}

static cbcpu pdp_8cxx_tab[4] = {pdp_rorb, pdp_rolb, pdp_asrb, pdp_aslb};

void pdp_8cxx(CPU* cpu) {
	pdp_8cxx_tab[(cpu->com >> 6) & 3](cpu);
}

// 8dxx
// MOVB _DST, F (except bit T)

void pdp_mtps(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 1);
	twsrc &= 0xef;		// T flag, b8..15 is not affected
	int f = pdp_get_flag(cpu);
	f &= 0xff10;
	f |= twsrc;
	pdp_set_flag(cpu, f);
}

// MOVB F, _DST
// N: negative
// Z: zero
// V: 0
// C: not affected
void pdp_mfps(CPU* cpu) {
	cpu->regWZ = pdp_adr(cpu, cpu->com, 1);
	twsrc = pdp_get_flag(cpu) & 0xff;
	if (twsrc & 0x80) twsrc |= 0xff00;
	if (cpu->com & 0x38) {
		pdp_wrb(cpu, cpu->regWZ, twsrc & 0xff);
	} else {
		cpu->regRN(cpu->com & 7) = twsrc;
	}
	//cpu->f &= ~(PDP_FZ | PDP_FN | PDP_FV);
	cpu->flgV = 0;
	cpu->flgZ = !(twsrc & 0xff);
	cpu->flgN = !!(twsrc & 0x80);
}

static cbcpu pdp_8dxx_tab[4] = {pdp_mtps, pdp_undef, pdp_undef, pdp_mfps};

void pdp_8dxx(CPU* cpu) {
	pdp_8dxx_tab[(cpu->com >> 6) & 3](cpu);
}

static cbcpu pdp_8nxx_tab[16] = {
	pdp_80xx, pdp_81xx, pdp_82xx, pdp_83xx,
	pdp_84xx, pdp_85xx, pdp_86xx, pdp_87xx,
	pdp_88xx, pdp_89xx, pdp_8axx, pdp_8bxx,
	pdp_8cxx, pdp_8dxx, pdp_undef, pdp_undef
};

void pdp_8xxx(CPU* cpu) {
	pdp_8nxx_tab[(cpu->com >> 8) & 0x0f](cpu);
}

// 07xxxx

// 070rss	mul ss,Rn	Rn,Rn+1 = Rn * ss
void pdp_mul(CPU* cpu) {
	if (cpu->gen < 1) {
		pdp_undef(cpu);
	} else {
		signed short src = pdp_src(cpu, cpu->com, 0);
		int rn = (cpu->com >> 6) & 7;		// reg number
		signed int res = src * (signed short)cpu->regRN(rn);
		cpu->regRN(rn) = (res >> 16) & 0xffff;
		cpu->regRN(rn | 1) = res & 0xffff;
		cpu->flgN = !!(res < 0);
		cpu->flgZ = !res;
		cpu->flgV = 0;
		cpu->flgC = !!((res < -(2 << 15)) || (res >= ((2 << 15) - 1)));
	}
}

// 071rss	div
void pdp_div(CPU* cpu) {
	if (cpu->gen < 1) {
		pdp_undef(cpu);
	} else {
		int rn = (cpu->com >> 6) & 6;	// & 7 ?
		signed int src = (cpu->regRN(rn) << 16) | cpu->regRN(rn | 1);
		unsigned short dst = pdp_src(cpu, cpu->com, 0);
		if (dst == 0) {
			cpu->flgC = 1;
		} else {
			int res = src / dst;
			int mod = src % dst;
			cpu->regRN(rn) = res & 0xffff;
			cpu->regRN(rn | 1) = mod & 0xffff;
			cpu->flgN = !!(res < 0);
			cpu->flgZ = !res;
			cpu->flgV = !!((src == 0) || (res < -(2 << 15)) || (res > (2 << 15) - 1));
			cpu->flgC = 0;
		}
	}
}

// 072rss	ash
void pdp_ash(CPU* cpu) {
	if (cpu->gen < 1) {
		pdp_undef(cpu);
	} else {
		twsrc = pdp_src(cpu, cpu->com, 0) & 0x3f;
		int rn = (cpu->com >> 6) & 7;
		twdst = cpu->regRN(rn);
		twres = twdst;
		if (twsrc & 0x20) {	// shift right
			twsrc = 0x40 - twsrc;
			while (twsrc) {
				cpu->flgC = twdst & 1;
				twdst = (twdst & 0x8000) | (twdst >> 1);
				twsrc--;
			}
		} else {		// shift left
			while (twsrc) {
				cpu->flgC = !!(twdst & 0x8000);
				twdst <<= 1;
				twsrc--;
			}
		}
		cpu->regRN(rn) = twdst;
		cpu->flgN = !!(twdst & 0x8000);
		cpu->flgZ = !twdst;
		cpu->flgV = !!((twres ^ twdst) & 0x8000);	// sign changed?
	}
}

// 073rss	ashc
void pdp_ashc(CPU* cpu) {
	if (cpu->gen < 1) {
		pdp_undef(cpu);
	} else {
		twsrc = pdp_src(cpu, cpu->com, 0) & 0x3f;
		int rn = (cpu->com >> 6) & 7;
		twres = (cpu->regRN(rn) << 16) | (cpu->regRN(rn | 1));
		twdst = cpu->regRN(rn);
		if (twsrc & 0x20) {
			twsrc = 0x40 - twsrc;
			while (twsrc) {
				cpu->flgC = twres & 1;
				twres >>= 1;		// sign?
				twsrc--;
			}
		} else {
			while (twsrc) {
				cpu->flgC = !!(twres & (1 << 31));
				twres <<= 1;
				twsrc--;
			}
		}
		cpu->regRN(rn) = (twres >> 16) & 0xffff;
		cpu->regRN(rn | 1) = twres & 0xffff;
		cpu->flgN = !!(twres & (1 << 31));
		cpu->flgZ = !twres;
		cpu->flgV = !!((cpu->regRN(rn) ^ twdst) & 0x8000);	// sign changed?
	}
}

// 074rss	xor Rn,ss	ss ^= Rn
void pdp_xor(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com, 0);
	twsrc ^= cpu->regRN((cpu->com >> 6) & 7);
	cpu->flgV = 0;
	cpu->flgZ = !twsrc;
	cpu->flgN = !!(twsrc & 0x8000);
	pdp_wres(cpu, cpu->com, twsrc);
}

// for VM2 only:
// 07500r	fadd
// 07501r	fsub
// 07502r	fmul
// 07503r	fdiv

void pdp_075x(CPU* cpu) {
	pdp_undef(cpu);
}

// 07ruu
void pdp_sob(CPU* cpu) {
	cpu->t += 8;
	twsrc = (cpu->com >> 6) & 7;
	cpu->regRN(twsrc)--;
	if (cpu->regRN(twsrc)) {
		cpu->regRN(7) -= (cpu->com & 0x3f) * 2;
	}
}

static cbcpu pdp_7nxx_tab[8] = {
	pdp_mul, pdp_div, pdp_ash, pdp_ashc,
	pdp_xor, pdp_075x, pdp_undef, pdp_sob
};

void pdp_7xxx(CPU* cpu) {
	pdp_7nxx_tab[(cpu->com >> 9) & 0x07](cpu);
}

// B1SSDD:mov
// DD = SS
// Z: src = 0
// N: src < 0
// C: not changed
// V: 0

// example: 016737 071324 177716
// src = 67	(E+R7)		R7+71324 (1st)
// dst = 37	@(R7)+		177716 (2nd)

void pdp_mov(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com >> 6, 0);
	pdp_dst(cpu, twsrc, cpu->com, 0);
	//cpu->f &= ~(PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgV = 0;
	cpu->flgZ = !twsrc;
	cpu->flgN = !!(twsrc & 0x8000);
}

// movb works as RMW (read-modify-write)
// movb (R1)+, @R3 : R1+=1
// movb (R0)+, (R1)+ : R0+=1, R1+=1
void pdp_movb(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com >> 6, 1);
	if (cpu->com & 0x38) {
		twdst = pdp_src(cpu, cpu->com, 1);
		pdp_wrb(cpu, cpu->regWZ, twsrc & 0xff);		// write low byte only
	} else {
		twsrc &= 0xff;					// extend sign
		if (twsrc & 0x80)
			twsrc |= 0xff00;
		cpu->regRN(cpu->com & 7) = twsrc;
	}
	//cpu->f &= ~(PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgV = 0;
	cpu->flgZ = !(twsrc & 0xff);
	cpu->flgN = !!(twsrc & 0x80);
}

// B2SSDD:cmp
// SS - DD >> /dev/null
// Z: dst = src
// N: dst < src
// C: high byte carry
// V: overflow
void pdp_cmp(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com >> 6, 0);
	twdst = pdp_src(cpu, cpu->com, 0);
	twres = twsrc - twdst;
	//cpu->f &= ~(PDP_FC | PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgZ = !(twres & 0xffff);
	cpu->flgN = !!(twres & 0x8000);
	cpu->flgC = !!(twres & ~0xffff);
	// V: neg - pos = pos || pos - neg = neg
	cpu->flgV = !!(((twsrc ^ twdst) & 0x8000) && ((twsrc ^ twres) & 0x8000));
}

void pdp_cmpb(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com >> 6, 1) & 0xff;
	twdst = pdp_src(cpu, cpu->com, 1) & 0xff;
	twres = twsrc - twdst;
	//cpu->f &= ~(PDP_FC | PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgZ = !(twres & 0xff);
	cpu->flgN = !!(twres & 0x80);
	cpu->flgC = !!(twres & 0x100);
	cpu->flgV = !!(((twsrc ^ twdst) & 0x80) && ((twsrc ^ twres) & 0x80));
}

// B3SSDD:bit (and)
// check (DD & SS). don't write it back
// Z: res = 0
// N: res.b15
// C: not affected
// V: 0
void pdp_bit(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com >> 6, 0);
	twdst = pdp_src(cpu, cpu->com, 0);
	twdst &= twsrc;
	//cpu->f &= ~(PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgV = 0;
	cpu->flgZ = !twdst;
	cpu->flgN = !!(twdst & 0x8000);
}

void pdp_bitb(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com >> 6, 1) & 0xff;
	twdst = pdp_src(cpu, cpu->com, 1) & 0xff;
	twdst &= twsrc;
	//cpu->f &= ~(PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgV = 0;
	cpu->flgZ = !twdst;
	cpu->flgN = !!(twdst & 0x80);
}

// B4SSDD:bic (and not)
// DD = DD & ~SS;
// Z: res = 0
// N: res.b15
// C: not affected
// V: 0
void pdp_bic(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com >> 6, 0);
	twdst = pdp_src(cpu, cpu->com, 0);
	twdst &= ~twsrc;
	//cpu->f &= ~(PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgV = 0;
	cpu->flgZ = !twdst;
	cpu->flgN = !!(twdst & 0x8000);
	pdp_wres(cpu, cpu->com, twdst);
}

void pdp_bicb(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com >> 6, 1) & 0xff;
	twdst = pdp_src(cpu, cpu->com, 1) & 0xff;
	twdst &= ~twsrc;			// src = 00xx; ~src = FFzz; keep high byte of dst
	//cpu->f &= ~(PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgV = 0;
	cpu->flgZ = !(twdst & 0xff);
	cpu->flgN = !!(twdst & 0x80);
	pdp_wresb(cpu, cpu->com, twdst & 0xff);
}

// B5SSDD:bis (or)
// DD = DD | SS
// Z: res = 0
// N: res.b15
// C: not affected
// V: 0
void pdp_bis(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com >> 6, 0);
	twdst = pdp_src(cpu, cpu->com, 0);
	twdst |= twsrc;
	//cpu->f &= ~(PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgV = 0;
	cpu->flgZ = !twdst;
	cpu->flgN = !!(twdst & 0x8000);
	pdp_wres(cpu, cpu->com, twdst);
}

void pdp_bisb(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com >> 6, 1) & 0xff;
	twdst = pdp_src(cpu, cpu->com, 1);
	twdst |= twsrc;
	//cpu->f &= ~(PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgV = 0;
	cpu->flgZ = !(twdst & 0xff);
	cpu->flgN = !!(twdst & 0x80);
	pdp_wresb(cpu, cpu->com, twdst);
}

// 06SSDD:add
// DD = DD + SS
// Z: result = 0
// N: result < 0
// C: high byte carry
// V: overflow (if both op is same sign, but res is opposite sign)		TODO: V = b15 overflow ^ b14 overflow

unsigned short pdp_op_add(CPU* cpu, int src, int dst) {
	twres = src + dst;
	cpu->flgZ = !(twres & 0xffff);
	cpu->flgN = !!(twres & 0x8000);
	cpu->flgC = !!(twres > 0xffff);
	twsrc = (twsrc ^ twdst) & 0x8000;	// src/dst sign is different (!twsrc - same)
	twdst = (twsrc ^ twres) & 0x8000;	// src/res sign is different
	cpu->flgV = !twsrc && twdst;		// neg + neg = pos || pos + pos = neg
	// cpu->flgV = cpu->flgC ^ !!(((twsrc & 0x7fff) + (twdst & 0x7fff)) & 0x8000);
	return twres & 0xffff;
}

void pdp_add(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com >> 6, 0);		// SS
	twdst = pdp_src(cpu, cpu->com, 0);		// DD (cpu->mptr is DD address)
	twres = pdp_op_add(cpu, twsrc, twdst);
	pdp_wres(cpu, cpu->com, twres);
}

// 16SSDD:sub
// DD = DD - SS
// TODO: DD = DD + ~SS + 1
// Z: result = 0
// N: result < 0
// C: high byte carry
// V: overflow
void pdp_sub(CPU* cpu) {
	twsrc = pdp_src(cpu, cpu->com >> 6, 0);
	twdst = pdp_src(cpu, cpu->com, 0);
//	twres = pdp_op_add(cpu, ~twsrc + 1, twdst);
	twres = twdst - twsrc;
	//cpu->f &= ~(PDP_FC | PDP_FN | PDP_FV | PDP_FZ);
	cpu->flgZ = !(twres & 0xffff);
	cpu->flgN = !!(twres & 0x8000);
	cpu->flgC = !!(twsrc > twdst);
	// flag V set if: pos - neg = neg || neg - pos = pos
	twsrc = (twsrc ^ twdst) & 0x8000;	// src/dst sign is different (!twsrc - same)
	twdst = (twsrc ^ twres) & 0x8000;	// src/res sign is different
	cpu->flgV = twsrc && twdst;		// neg - pos = pos || pos - neg = neg
	pdp_wres(cpu, cpu->com, twres);
}

// tables

// xNNN xxxx xxxx xxxx
static cbcpu pdp_tab_a[16] = {
	pdp_0xxx, pdp_mov, pdp_cmp, pdp_bit, pdp_bic, pdp_bis, pdp_add, pdp_7xxx,
	pdp_8xxx, pdp_movb, pdp_cmpb, pdp_bitb, pdp_bicb, pdp_bisb, pdp_sub, pdp_undef
};

void pdp_timer(CPU* cpu, int t) {
	if (cpu->timer.flag & 1) return;		// stopped
	cpu->timer.cnt -= t;
	while (cpu->timer.cnt < 0) {
		cpu->timer.cnt += cpu->timer.per;
		if (cpu->timer.flag & 0x10) {
			cpu->timer.val--;
			if (cpu->timer.val == 0xffff) {
				if (cpu->timer.flag & 0x04) {
					cpu->timer.flag |= 0x80;
					cpu->intrq |= PDP_INT_TIMER;
				}
				if (cpu->timer.flag & 0x02) {
					cpu->timer.val = 0xffff;
				} else if (cpu->timer.flag & 0x08) {
					cpu->timer.flag &= ~0x10;
				} else {
					cpu->timer.val = cpu->timer.ival;
				}
			}
		}
	}
}

int pdp11_exec(CPU* cpu) {
#ifdef ISDEBUG
//	printf("%.4X : %.4X\n", cpu->regRN(7), pdp_rd(cpu, cpu->regRN(7)));
#endif
//	cpu->regRN(7) = cpu->regPC;
	cpu->oldpc = cpu->regRN(7);
	//if (cpu->halt) return 4;
	//cpu->t = cpu->wait ? 8 : 0;
	cpu->t = 0;
	if (cpu->inten & cpu->intrq)
		cpu->t += pdp11_int(cpu);
	if (cpu->t == 0) {
		cpu->com = pdp_rd(cpu, cpu->regRN(7));
		cpu->regRN(7) += 2;
		cpu->t += 8;
		// exec 1st tab #Nxxx
		pdp_tab_a[(cpu->com >> 12) & 0x0f](cpu);
//		if (cpu->sta) {			// stack goes trough 0x400 : trap 4
//			cpu->sta = 0;
//			pdp_trap(cpu, 4);
//		}
	}
//	cpu->regPC = cpu->regRN(7);
//	cpu->regSP = cpu->regRN(6);		// for correct deBUGa stack dump
	pdp_timer(cpu, cpu->t);
	return cpu->t;
}

// registers

xRegDsc pdp11RegTab[] = {
	{PDP11_REG0, "R0", REG_WORD, REG_RDMP, offsetof(CPU, regRN(0))},
	{PDP11_REG1, "R1", REG_WORD, REG_RDMP, offsetof(CPU, regRN(1))},
	{PDP11_REG2, "R2", REG_WORD, REG_RDMP, offsetof(CPU, regRN(2))},
	{PDP11_REG3, "R3", REG_WORD, REG_RDMP, offsetof(CPU, regRN(3))},
	{PDP11_REG4, "R4", REG_WORD, REG_RDMP, offsetof(CPU, regRN(4))},
	{PDP11_REG5, "R5", REG_WORD, REG_RDMP, offsetof(CPU, regRN(5))},
	{PDP11_REG6, "SP", REG_WORD, REG_RDMP | REG_SP, offsetof(CPU, regRN(6))},
	{PDP11_REG7, "PC", REG_WORD, REG_RDMP | REG_PC, offsetof(CPU, regRN(7))},
	{PDP11_REGF, "PSW", REG_32, 0, 0},
	{REG_NONE, "", 0, 0, 0}
};

static char* regNames[8] = {"R0","R1","R2","R3","R4","R5","SP","PC"};
static char* pdpFlags = "I--TNZVC";

unsigned short pdp_get_reg(CPU* cpu, int id) {
	unsigned short res = 0;
	switch(id) {
		case PDP11_REG0: res = cpu->regRN(0); break;
		case PDP11_REG1: res = cpu->regRN(1); break;
		case PDP11_REG2: res = cpu->regRN(2); break;
		case PDP11_REG3: res = cpu->regRN(3); break;
		case PDP11_REG4: res = cpu->regRN(4); break;
		case PDP11_REG5: res = cpu->regRN(5); break;
		case PDP11_REG6: res = cpu->regRN(6); break;
		case PDP11_REG7: res = cpu->regRN(7); break;
		case PDP11_REGF: res = pdp_get_flag(cpu); break;
	}
	return res;
}

void pdp11_get_regs(CPU* cpu, xRegBunch* bunch) {
	int idx = 0;
	while (pdp11RegTab[idx].id != REG_NONE) {
		bunch->regs[idx].id = pdp11RegTab[idx].id;
		bunch->regs[idx].name = pdp11RegTab[idx].name;
		bunch->regs[idx].type = pdp11RegTab[idx].type;
		bunch->regs[idx].flag = pdp11RegTab[idx].flag;
		bunch->regs[idx].value = pdp_get_reg(cpu, pdp11RegTab[idx].id);
		idx++;
	}
	bunch->regs[idx].id = REG_NONE;
	//memcpy(bunch->flags, "---TNZVC", 8);
	bunch->flags = pdpFlags;
	//int f = pdp_get_flag(cpu);
	//f &= 0xff;
	//pdp_set_flag(cpu, f);
}

void pdp11_set_regs(CPU* cpu, xRegBunch bunch) {
	int idx = 0;
	while (bunch.regs[idx].id != REG_NONE) {
		switch (bunch.regs[idx].id) {
			case PDP11_REG0: cpu->regRN(0) = bunch.regs[idx].value; break;
			case PDP11_REG1: cpu->regRN(1) = bunch.regs[idx].value; break;
			case PDP11_REG2: cpu->regRN(2) = bunch.regs[idx].value; break;
			case PDP11_REG3: cpu->regRN(3) = bunch.regs[idx].value; break;
			case PDP11_REG4: cpu->regRN(4) = bunch.regs[idx].value; break;
			case PDP11_REG5: cpu->regRN(5) = bunch.regs[idx].value; break;
			case PDP11_REG6: cpu->regRN(6) = bunch.regs[idx].value; break;
			case PDP11_REG7: cpu->regRN(7) = bunch.regs[idx].value; /*cpu->regPC = cpu->regRN(7);*/ break;
			case PDP11_REGF: pdp_set_flag(cpu, bunch.regs[idx].value); break;
		}
		idx++;
	}
}

// disasm

// TODO: maybe mov Rn,-(sp) -> push Rn | mov (sp)+, Rn -> pop Rn | rts r7 -> ret

xPdpDasm pdp11_dasm_tab[] = {
	{0xffff, 0x0000, 0, "halt"},
	{0xffff, 0x0001, 0, "wait"},
	{0xffff, 0x0002, 0, "rti"},
	{0xffff, 0x0003, OF_SKIPABLE, "bpt"},
	{0xffff, 0x0004, OF_SKIPABLE, "iot"},
	{0xffff, 0x0005, 0, "reset"},
	{0xffff, 0x0006, 0, "rtt"},
	{0xffff, 0x0007, 0, "start"},
	{0xffff, 0x0008, 0, "stop"},
	{0xffc0, 0x0040, 0, "jmp :d"},	// :d lower 6 bits dst(src)
	{0xffff, 0x0087, 0, "ret"},	// special rts r7 = ret
	{0xfff8, 0x0080, 0, "rts r:0"},	// :0 bits 0,1,2 number
	{0xfff0, 0x00a0, 0, "cl :f"},	// :f lower 4 bits flags
	{0xfff0, 0x00b0, 0, "se :f"},
	{0xffc0, 0x00c0, 0, "swab :d"},
	{0xff00, 0x0100, 0, "br :e"},	// :e relative jump
	{0xff00, 0x0200, 0, "bne :e"},
	{0xff00, 0x0300, 0, "beq :e"},
	{0xff00, 0x0400, 0, "bge :e"},
	{0xff00, 0x0500, 0, "blt :e"},
	{0xff00, 0x0600, 0, "bgt :e"},
	{0xff00, 0x0700, 0, "ble :e"},
	{0xffc0, 0x09c0, OF_SKIPABLE, "call :d"},		// special jsr r7,nn = call nn
	{0xfe00, 0x0800, OF_SKIPABLE, "jsr r:6,:d"},	// :6 bits 6,7,8 number
	{0xffc0, 0x0a00, 0, "clr :d"},
	{0xffc0, 0x0a40, 0, "com :d"},
	{0xffc0, 0x0a80, 0, "inc :d"},
	{0xffc0, 0x0ac0, 0, "dec :d"},
	{0xffc0, 0x0b00, 0, "neg :d"},
	{0xffc0, 0x0b40, 0, "adc :d"},
	{0xffc0, 0x0b80, 0, "sbc :d"},
	{0xffc0, 0x0bc0, 0, "tst :d"},
	{0xffc0, 0x0c00, 0, "ror :d"},
	{0xffc0, 0x0c40, 0, "rol :d"},
	{0xffc0, 0x0c80, 0, "asr :d"},
	{0xffc0, 0x0cc0, 0, "arl :d"},
	{0xffc0, 0x0d40, 0, "mfpi :d"},
	{0xffc0, 0x0d80, 0, "mtpi :d"},
	{0xffc0, 0x0dc0, 0, "sxt :d"},
	{0xf000, 0x1000, 0, "mov :s,:d"},	// :s = :d from bits 6-11
	{0xf000, 0x2000, 0, "cmp :s,:d"},
	{0xf000, 0x3000, 0, "bit :s,:d"},
	{0xf000, 0x4000, 0, "bic :s,:d"},
	{0xf000, 0x5000, 0, "bis :s,:d"},
	{0xf000, 0x6000, 0, "add :s,:d"},
	{0xfe00, 0x7000, 0, "mul :d,r:6"},	// vm1g+
	{0xfe00, 0x7200, 0, "div :d,r:6"},	// vm2+
	{0xfe00, 0x7400, 0, "ash :d,r:6"},	// vm2+
	{0xfe00, 0x7600, 0, "ashc :d,r:6"},	// vm2+
	{0xfe00, 0x7800, 0, "xor r:6,:d"},
	{0xfe00, 0x7e00, OF_SKIPABLE, "sob r:6,:j"},		// :j = lower 6 bits, back relative adr
	{0xff00, 0x8000, 0, "bpl :e"},
	{0xff00, 0x8100, 0, "bmi :e"},
	{0xff00, 0x8200, 0, "bhi :e"},
	{0xff00, 0x8300, 0, "blos :e"},
	{0xff00, 0x8400, 0, "bvc :e"},
	{0xff00, 0x8500, 0, "bvs :e"},
	{0xff00, 0x8600, 0, "bcc :e"},
	{0xff00, 0x8700, 0, "bcs :e"},
	{0xff00, 0x8800, OF_SKIPABLE, "emt :x"},	// x: lower 8 bits, number
	{0xff00, 0x8900, OF_SKIPABLE, "trap :x"},
	{0xffc0, 0x8a00, 0, "clrb :d"},
	{0xffc0, 0x8a40, 0, "comb :d"},
	{0xffc0, 0x8a80, 0, "incb :d"},
	{0xffc0, 0x8ac0, 0, "decb :d"},
	{0xffc0, 0x8b00, 0, "negb :d"},
	{0xffc0, 0x8b40, 0, "adcb :d"},
	{0xffc0, 0x8b80, 0, "sbcb :d"},
	{0xffc0, 0x8bc0, 0, "tstb :d"},
	{0xffc0, 0x8c00, 0, "rorb :d"},
	{0xffc0, 0x8c40, 0, "rolb :d"},
	{0xffc0, 0x8c80, 0, "asrb :d"},
	{0xffc0, 0x8cc0, 0, "aslb :d"},
	{0xffc0, 0x8d00, 0, "mtps :d"},
	{0xffc0, 0x8dc0, 0, "mfps :d"},
	{0xf000, 0x9000, 0, "movb :s,:d"},
	{0xf000, 0xa000, 0, "cmpb :s,:d"},
	{0xf000, 0xb000, 0, "bitb :s,:d"},
	{0xf000, 0xc000, 0, "bicb :s,:d"},
	{0xf000, 0xd000, 0, "bisb :s,:d"},
	{0xf000, 0xe000, 0, "sub :s,:d"},
	{0x0000, 0x0000, 0, "undef"}
};

static char mnbuf[128];
static char num7[8] = "01234567";

// TODO: chose between - E(Rn), (Rn + E(unsigned)) or (Rn +- E(signed))

char* put_addressation(char* dst, unsigned short type) {
	switch ((type >> 3) & 7) {
		case 1: *(dst++) = '@';				// @Rn
		case 0:	strcpy(dst, regNames[type & 7]);	// Rn
			dst += strlen(regNames[type & 7]);
			break;
		case 3: *(dst++) = '@';				// @(Rn)+
		case 2: if ((type & 7) == 7) {			// (Rn)+
				if (type & 8) {
					dst--;
					*dst++ = '(';
					*dst++ = ':';
					*dst++ = '8';
					*dst++ = ')';
				} else {
					*(dst++) = ':';		// :8 will be replaced with next word (oct)
					*(dst++) = '8';
				}
			} else {
				*(dst++) = '(';
				strcpy(dst, regNames[type & 7]);
				dst += strlen(regNames[type & 7]);
				*(dst++) = ')';
				*(dst++) = '+';
			}
			break;
		case 5: *(dst++) = '@';				// @-(Rn)
		case 4: *(dst++) = '-';				// -(Rn)
			*(dst++) = '(';
			strcpy(dst, regNames[type & 7]);
			dst += strlen(regNames[type & 7]);
			*(dst++) = ')';
			break;
		case 7: *(dst++) = '@';				// @E(Rn)
		case 6: if ((type & 7) == 7) {			// E(Rn)
				*(dst++) = '(';
				*(dst++) = ':';			// (E + PC)
				*(dst++) = '6';			// :6 will be replaced with adr+imm.word
				*(dst++) = ')';
			} else {
				*(dst++) = ':';			// :8 will be replaced with imm.word
				*(dst++) = '8';
				*(dst++) = '(';
				strcpy(dst, regNames[type & 7]);
				dst += strlen(regNames[type & 7]);
				*(dst++) = ')';
			}
			break;
	}
	return dst;
}

xMnem pdp11_mnem(CPU* cpu, int qadr, cbdmr mrd, void* dat) {
	xMnem res;
	res.oadr = -1;
	res.len = 2;
	int idx = 0;
	unsigned short dtw;
	unsigned short adr = qadr & 0xffff;
	unsigned short com = mrd(adr++, dat);
	com |= (mrd(adr++, dat) << 8);
	while ((pdp11_dasm_tab[idx].mask != 0) && ((com & pdp11_dasm_tab[idx].mask) != pdp11_dasm_tab[idx].code))
		idx++;
	const char* src = pdp11_dasm_tab[idx].mnem;
	char* dst = mnbuf;
	while (*src != 0) {
		if (*src == ':') {
			src++;
			switch (*src) {
				case '0':
					*dst = num7[com & 7];
					dst++;
					break;
				case '6':
					*dst = num7[(com >> 6) & 7];
					dst++;
					break;
				case 'x':
					dst += sprintf(dst, "%o", com & 0xff);
					break;
				case 'e':
					dtw = (com & 0xff);
					if (com & 0x80) dtw |= 0xff00;
					dtw <<= 1;
					dtw += adr;
					res.oadr = adr;
					dst += sprintf(dst, "%o", dtw);
					break;
				case 'j':
					dtw = com & 0x3f;
					dtw <<= 1;
					dtw = adr - dtw;
					dst += sprintf(dst, "%o", dtw);
					break;
				case 'f':
					if (com & 1) *(dst++) = 'C';
					if (com & 2) *(dst++) = 'V';
					if (com & 4) *(dst++) = 'Z';
					if (com & 8) *(dst++) = 'N';
					if (!(com & 15)) *(dst++) = '0';
					break;
				// TODO: set res.oadr for :s and :d
				case 's':
					dst = put_addressation(dst, com >> 6);
					break;
				case 'd':
					dst = put_addressation(dst, com);
					break;
			}
			src++;
		} else {
			*dst = *src;
			src++;
			dst++;
		}
	}
	*dst = 0;
	res.mnem = mnbuf;
	res.flag = pdp11_dasm_tab[idx].flag;
	res.cond = 0;
	res.met = 0;
	res.mem = 0;
	res.mop = 0;
	res.oadr = 0;
	return res;
}
