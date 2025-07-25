#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spectrum.h"
#include "filetypes/filetypes.h"
#include "cpu/Z80/z80.h"

static int nsTime;
static int res2;
int res4 = 0;		// save last T synced with video (last is res2-res4)

unsigned char* comp_get_memcell_flag_ptr(Computer* comp, int adr) {
	unsigned char* res = NULL;
	xAdr xadr = mem_get_xadr(comp->mem, adr);
	switch (xadr.type) {
		case MEM_RAM: res = comp->brkRamMap + (xadr.abs & comp->mem->ramMask); break;
		case MEM_ROM: res = comp->brkRomMap + (xadr.abs & comp->mem->romMask); break;
		case MEM_SLOT: if (comp->slot->brkMap) {res = comp->slot->brkMap + (xadr.abs & comp->slot->memMask);} break;
	}
	return res;
}

bpChecker comp_check_bp(Computer* comp, int adr, int mask) {
	bpChecker ch;
	ch.t = -1;
	ch.ptr = NULL;
	if (comp->mem->busmask < 0x10000) {
		ch.ptr = comp->brkAdrMap + (adr & comp->mem->busmask);
		if (*ch.ptr & mask) {
			ch.t = BRK_CPUADR;
			ch.a = adr & comp->mem->busmask;
		}
	}
	if (ch.t < 0) {
		xAdr xadr = mem_get_xadr(comp->mem, adr);
		switch (xadr.type) {
			case MEM_RAM: xadr.abs &= comp->mem->ramMask; ch.ptr = comp->brkRamMap + xadr.abs; ch.t = BRK_MEMRAM; break;
			case MEM_ROM: xadr.abs &= comp->mem->romMask; ch.ptr = comp->brkRomMap + xadr.abs; ch.t = BRK_MEMROM; break;
			case MEM_SLOT: if (comp->slot->brkMap) {
					xadr.abs &= comp->slot->memMask;
					ch.ptr = comp->slot->brkMap + xadr.abs;
					ch.t = BRK_MEMSLT;
				} break;
		}
		if (ch.ptr) {
			if (*ch.ptr & mask) {
				// printf("xadr.abs = %X, page=%X, off=%X\n",xadr.abs,xadr.bank,xadr.adr & comp->mem->pgmask);
				ch.a = xadr.abs;
			} else {
				ch.t = -1;
			}
		}
	}
	return ch;
}

// video callbacks

int vid_mrd_cb(int adr, void* ptr) {
	Computer* comp = (Computer*)ptr;
	return comp->mem->ramData[adr & comp->mem->ramMask];
}

// mrw,irw

int memrd(int adr, int m1, void* ptr) {
	Computer* comp = (Computer*)ptr;
#ifdef HAVEZLIB
	if (m1 && comp->rzx.play && (comp->rzx.frm.fetches > 0)) {
		comp->rzx.frm.fetches--;
	}
#endif
	unsigned char* fptr = comp_get_memcell_flag_ptr(comp, adr);
	if (fptr) {
		unsigned char flag = *fptr;
		if (comp->maping) {
			if ((comp->cpu->regPC-1+comp->cpu->cs.base) == adr) {
				flag &= 0x0f;
				flag |= DBG_VIEW_EXEC;
				*fptr = flag;
			} else if (!(flag & 0xf0)) {
				flag |= DBG_VIEW_BYTE;
				*fptr = flag;
			}
		}
	}
	bpChecker ch = comp_check_bp(comp, adr, MEM_BRK_RD);
	if (ch.t >= 0) {
		comp->brk = 1;
		comp->brkt = ch.t;
		comp->brka = ch.a;
	}
	return comp->hw->mrd(comp,adr,m1);
}

void memwr(int adr, int val, void* ptr) {
	Computer* comp = (Computer*)ptr;
	unsigned char* fptr = comp_get_memcell_flag_ptr(comp, adr);
	if (fptr) {
		unsigned char flag = *fptr;
		if (comp->maping) {
			if (!(flag & 0xf0)) {
				flag |= DBG_VIEW_BYTE;
				*fptr = flag;
			}
		}
		bpChecker ch = comp_check_bp(comp, adr, MEM_BRK_WR);
		if (ch.t >= 0) {
			comp->brk = 1;
			comp->brkt = ch.t;
			comp->brka = ch.a;
		}
	}
/*
	if (flag & MEM_BRK_WR) {
		comp->brk = 1;
	} if (comp->mem->busmask < 0x10000) {
		flag |= comp->brkAdrMap[adr & 0xffff];
		if (flag & MEM_BRK_WR) {
			comp->brk = 1;
			comp->brkt = BRK_CPUADR;
			comp->brka = adr;
		}
	}
*/
	comp->hw->mwr(comp,adr,val);
}

void zx_cont_delay(Computer* comp) {
	int wns = vid_wait(comp->vid, 5 << 14);	// video is already at end of wait cycle
	int tns = 0;
	while (wns > 0) {
		comp->cpu->t++;
		wns -= comp->nsPerTick;
		tns += comp->nsPerTick;
	}
	vid_sync(comp->vid, tns);
	res4 = comp->cpu->t;
}

void zx_free_ticks(Computer* comp, int t) {
	comp->cpu->t += t;
	vid_sync(comp->vid, t * comp->nsPerTick);
	res4 = comp->cpu->t;
}

// Contention on T1
void zx_cont_t1(Computer* comp, int port) {
	if ((port & 0xc000) == 0x4000)
		zx_cont_delay(comp);
	zx_free_ticks(comp, 1);
}

// Contention on T2-T4
void zx_cont_tn(Computer* comp, int port) {
	if ((port & 0xc000) == 0x4000) {
		if (port & 1) {			// C:1 C:1 C:1 C:1
//			zx_cont_delay(comp);
//			zx_free_ticks(comp, 1);
			zx_cont_delay(comp);
			zx_free_ticks(comp, 1);
			zx_cont_delay(comp);
			zx_free_ticks(comp, 1);
			zx_cont_delay(comp);
			zx_free_ticks(comp, 1);
		} else {			// C:1 C:3
//			zx_cont_delay(comp);
//			zx_free_ticks(comp, 1);
			zx_cont_delay(comp);
			zx_free_ticks(comp, 3);
		}
	} else if (port & 1) {			// N:4
		zx_free_ticks(comp, 4-1);
	} else {				// N:1 C:3
//		zx_free_ticks(comp, 1);
		zx_cont_delay(comp);
		zx_free_ticks(comp, 3);
	}
}

static unsigned char ula_levs[8] = {0x00, 0x24, 0x49, 0x6d, 0x92, 0xb6, 0xdb, 0xff};

void zxSetUlaPalete(Computer* comp) {
	int i;
	int col;
	xColor xc;
	for (i = 0; i < 64; i++) {
		col = (comp->vid->ula->pal[i] << 1) & 7;	// blue
		if (col & 2) col |= 1;
		xc.b = ula_levs[col];
		col = (comp->vid->ula->pal[i] >> 2) & 7;	// red
		xc.r = ula_levs[col];
		col = (comp->vid->ula->pal[i] >> 5) & 7;	// green
		xc.g = ula_levs[col];
		vid_set_col(comp->vid, i, xc);
	}
}

int iord(int port, void* ptr) {
	Computer* comp = (Computer*)ptr;
// TODO: zx only
	if (comp->hw->grp == HWG_ZX) {
		if (comp->contIO && 0) {
			zx_cont_t1(comp, port);
			zx_cont_tn(comp, port);
		} else {
			vid_sync(comp->vid,(comp->cpu->t + 3 - res4) * comp->nsPerTick);
			res4 = comp->cpu->t + 3;
		}
	}
// play rzx
#ifdef HAVEZLIB
	int res = 0xff;
	if (comp->rzx.play) {
		if (comp->rzx.frm.pos < comp->rzx.frm.size) {
			res = comp->rzx.frm.data[comp->rzx.frm.pos];
			comp->rzx.frm.pos++;
			return res;
		} else {
			rzxStop(comp);
			printf("overIO\n");
			comp->rzx.overio = 1;
			return 0xff;
		}
	}
#endif
	comp->bdiz = (comp->dos && (comp->dif->type == DIF_BDI)) ? 1 : 0;
// brk
	if (comp->brkIOMap[port] & MEM_BRK_RD) {
		comp->brk = 1;
		comp->brkt = BRK_IOPORT;
		comp->brka = port;
	}

	return comp->hw->in ? comp->hw->in(comp, port) : 0xff;
}

void iowr(int port, int val, void* ptr) {
	Computer* comp = (Computer*)ptr;
	comp->bdiz = (comp->dos && (comp->dif->type == DIF_BDI)) ? 1 : 0;
	if (comp->hw->grp == HWG_ZX) {
		// sync video to current T
		vid_sync(comp->vid, (comp->cpu->t - res4) * comp->nsPerTick);
		res4 = comp->cpu->t;
		if (comp->contIO) {
			zx_cont_t1(comp, port);
			comp->hw->out(comp, port, val);
			zx_cont_tn(comp, port);
			comp->cpu->t -= 4;
		} else {
			vid_sync(comp->vid, comp->nsPerTick);
			res4++;
			comp->hw->out(comp, port, val);
		}
	} else {
		comp->hw->out(comp, port, val);
	}
	if (comp->vid->ula->palchan) {
		zxSetUlaPalete(comp);
		comp->vid->ula->palchan = 0;
	}
// brk
	if (comp->brkIOMap[port] & MEM_BRK_WR) {
		comp->brk = 1;
		comp->brkt = BRK_IOPORT;
		comp->brka = port;
	}
}

int intrq(void* ptr) {
	Computer* comp = (Computer*)ptr;
	return comp->hw->ack ? comp->hw->ack(comp) : 0xff;
}

void comp_irq(int t, void* ptr) {
	Computer* comp = (Computer*)ptr;
	switch (t) {
		case IRQ_BRK:
			comp->brk = 1;
			comp->brkt = -1;
			break;
		case IRQ_CPU_HALT:
			comp->hCount = comp->frmtCount;			// fix T counter from INT to start of HALT
			break;
		case IRQ_VID_INT:
			comp->fCount = comp->vid->nsPerFrame / comp->nsPerTick;		// fixed each frame (but depends on cpu freq @ int)
			comp->frmtCount = 0;
			if (!comp->cpu->flgHALT) {
				comp->hCount = comp->fCount;		// if not HALT-ed during frame, count all ticks
			}
			break;
		case IRQ_CPU_SYNC:
			vid_sync(comp->vid, (comp->cpu->t - res4) * comp->nsPerTick);
			res4 = comp->cpu->t;
			break;
	}
	if (comp->hw->irq) comp->hw->irq(comp, t);
}

// new (for future use)

/*
int comp_rom_rd(Computer* comp, int adr) {
	adr &= comp->mem->romMask;
	if (comp->brkRomMap[adr] & MEM_BRK_RD)
		comp->brk = 1;
	return comp->mem->romData[adr] & 0xff;
}

void comp_rom_wr(Computer* comp, int adr, int val) {
	adr &= comp->mem->romMask;
	if (comp->brkRomMap[adr] & MEM_BRK_WR)
		comp->brk = 1;
	comp->mem->romData[adr] = val & 0xff;
}

int comp_ram_rd(Computer* comp, int adr) {
	adr &= comp->mem->ramMask;
	if (comp->brkRamMap[adr] & MEM_BRK_RD)
		comp->brk = 1;
	return comp->mem->ramData[adr] & 0xff;
}

void comp_ram_wr(Computer* comp, int adr, int val) {
	adr &= comp->mem->ramMask;
	if (comp->brkRamMap[adr] & MEM_BRK_WR)
		comp->brk = 1;
	comp->mem->ramData[adr] = val & 0xff;
}

int comp_slt_rd(Computer* comp, int adr) {
	if (!comp->slot->data) return 0xff;
	adr &= comp->slot->memMask;
	if (comp->slot->brkMap[adr] & MEM_BRK_RD)
		comp->brk = 1;
	return comp->slot->data[adr] & 0xff;
}

void comp_slt_wr(Computer* comp, int adr, int val) {
	if (!comp->slot->data) return;
	adr &= comp->slot->memMask;
	if (comp->slot->brkMap[adr] & MEM_BRK_WR)
		comp->brk = 1;
	comp->slot->data[adr] = val & 0xff;
}
*/

// rzx

void rzxStop(Computer* zx) {
#ifdef HAVEZLIB
	zx->rzx.play = 0;
	if (zx->rzx.file) fclose(zx->rzx.file);
	zx->rzx.file = NULL;
	zx->rzx.fCount = 0;
	zx->rzx.frm.size = 0;
	zx->rzx.stop = 1;
#endif
}

//				0   1   2   3   4   5   6   7   8   9   A   B   C    D    E    F
const unsigned char blnm[] = {'x','B','o','o','t',000,000,000,000,000,000,000,0x38,0x98,0x00,0x00};
const unsigned char bcnm[] = {'x','E','v','o',' ',000,000,000,000,000,000,000,0x89,0x99,0x00,0x00};

Computer* compCreate() {
	Computer* comp = (Computer*)malloc(sizeof(Computer));
	memset(comp, 0x00, sizeof(Computer));
	comp->resbank = RES_48;
	comp->firstRun = 1;
	comp->debug = 0;

	comp->cpu = cpuCreate(CPU_Z80,memrd,memwr,iord,iowr,intrq,comp_irq,comp);
	comp->mem = memCreate();
	comp->vid = vidCreate(vid_mrd_cb, comp_irq, comp);
	vid_set_mode(comp->vid, VID_NORMAL);

// input
	comp->keyb = keyCreate(comp_irq, comp);
	// comp->cmos.kbuf = &comp->keyb->kbuf;
	comp->joy = joyCreate();
	comp->mouse = mouseCreate(comp_irq, comp);
	comp->ppi = ppi_create();
	comp->ps2c = ps2c_create(comp->keyb, comp->mouse, comp_irq, comp);
// storage
	comp->tape = tape_create(comp_irq, comp);
	comp->dif = difCreate(DIF_NONE, comp_irq, comp);
	comp->ide = ideCreate(IDE_NONE, comp_irq, comp);
	comp->ide->smuc.cmos = &comp->cmos;
	comp->sdc = sdcCreate();
	comp->slot = sltCreate();
// sound
	comp->ts = tsCreate(TS_NONE,SND_AY,SND_NONE);
	comp->gs = gsCreate();
	comp->sdrv = sdrvCreate(SDRV_NONE);
	comp->gbsnd = gbsCreate();
	comp->saa = saaCreate();
	comp->beep = bcCreate();
	comp->nesapu = apuCreate(nes_apu_ext_rd, comp_irq, comp);
// c64
	comp->c64.cia1 = cia_create(IRQ_CIA1, comp_irq, comp);
	comp->c64.cia2 = cia_create(IRQ_CIA2, comp_irq, comp);
// ibm
	comp->dma1 = dma_create(comp, 0);
	comp->dma2 = dma_create(comp, 1);
	comp->mpic = pic_create(1, comp_irq, comp);
	comp->spic = pic_create(0, comp_irq, comp);
	comp->pit = pit_create(comp_irq, comp);
	comp->com1 = uart_create(IRQ_COM1, comp_irq, comp);
//	pit_reset(&comp->pit);
//	comp->mpic.master = 1;
//	comp->spic.master = 0;
// baseconf
	memcpy(comp->evo.blVer,blnm,16);
	memcpy(comp->evo.bcVer,bcnm,16);
//tsconf
	comp->tsconf.pwr_up = 1;
// rzx
#ifdef HAVEZLIB
	comp->rzx.file = NULL;
#endif
	compSetHardware(comp, "Dummy");
	gsReset(comp->gs);
	comp->cmos.data[17] = 0xaa;	// 0a?
	comp->frqMul = 1;
	compSetBaseFrq(comp, 3.5);
//	compReset(comp, RES_DEFAULT);		// Can't reset here, cuz comp->resbank not defined yet
	return comp;
}

void compDestroy(Computer* comp) {
	rzxStop(comp);
	cpuDestroy(comp->cpu);
	memDestroy(comp->mem);
	vidDestroy(comp->vid);
	keyDestroy(comp->keyb);
	joyDestroy(comp->joy);
	mouseDestroy(comp->mouse);
	tape_destroy(comp->tape);
	difDestroy(comp->dif);
	ideDestroy(comp->ide);
	tsDestroy(comp->ts);
	gsDestroy(comp->gs);
	gbsDestroy(comp->gbsnd);
	sdrvDestroy(comp->sdrv);
	saaDestroy(comp->saa);
	bcDestroy(comp->beep);
	apuDestroy(comp->nesapu);
	sltDestroy(comp->slot);
	ppi_destroy(comp->ppi);
	ps2c_destroy(comp->ps2c);
	dma_destroy(comp->dma1);
	dma_destroy(comp->dma2);
	pit_destroy(comp->pit);
	cia_destroy(comp->c64.cia1);
	cia_destroy(comp->c64.cia2);
	free(comp);
}

void compReset(Computer* comp,int res) {
#ifdef HAVEZLIB
	if (comp->rzx.play)
		rzxStop(comp);
#endif

	if (res == RES_DEFAULT)
		res = comp->resbank;
	comp->p7FFD = ((res == RES_DOS) || (res == RES_48)) ? 0x10 : 0x00;
	comp->dos = ((res == RES_DOS) || (res == RES_SHADOW)) ? 1 : 0;
	comp->rom = (comp->p7FFD & 0x10) ? 1 : 0;
	comp->cpm = 0;
	comp->ext = 0;
	comp->prt2 = 0;
	comp->p1FFD = 0;
	comp->pEFF7 = 0;

	vid_reset(comp->vid);
	// kbdReleaseAll(comp->keyb);
	kbdSetMode(comp->keyb, KBD_SPECTRUM);
	ps2c_reset(comp->ps2c);
	difReset(comp->dif);
	if (comp->gs->reset)
		gsReset(comp->gs);
	tsReset(comp->ts);
	ideReset(comp->ide);
	saaReset(comp->saa);
	sdcReset(comp->sdc);
	dma_reset(comp->dma1);
	dma_reset(comp->dma2);
	if (comp->hw->reset)
		comp->hw->reset(comp);
	comp->hw->mapMem(comp);
	comp->cpu->cs.base = 0;		// for all except i80286
	comp->cpu->ss.base = 0;
	comp->cpu->cs.limit = 0xffff;
	cpu_reset(comp->cpu);
}

// cpu freq

void comp_update_timings(Computer* comp) {
	comp->nsPerTick = 1e3 / comp->cpuFrq;
	if (comp->hw->init)
		comp->hw->init(comp);
	comp->nsPerTick /= comp->frqMul;
}

void compSetBaseFrq(Computer* comp, double frq) {
	if (frq > 0)
		comp->cpuFrq = frq;
	comp_update_timings(comp);
}

void compSetTurbo(Computer* comp, double mult) {
	comp->frqMul = mult;
	comp->speed = (mult > 1) ? 1 : 0;
	comp_update_timings(comp);
}

void comp_set_layout(Computer* comp, vLayout* lay) {
	if (comp->hw->lay)
		lay = comp->hw->lay;
	vid_set_layout(comp->vid, lay);
}

void comp_kbd_release(Computer* comp) {
	kbdReleaseAll(comp->keyb);
	ps2c_clear(comp->ps2c);
}

// hardware

int compSetHardware(Computer* comp, const char* name) {
	HardWare* hw;
	if (name == NULL) {
		hw = comp->hw;
	} else {
		hw = findHardware(name);
	}
	if (hw == NULL) return 0;
	comp->hw = hw;
//	comp->cpu->nod = 0;
	comp->vid->mrd = vid_mrd_cb;
	comp->tape->xen = 0;
	mem_set_bus(comp->mem, hw->adrbus);
	compSetBaseFrq(comp, 0);	// recalculations
	return 1;
}

// exec 1 opcode, sync devices, return eated ns

int compExec(Computer* comp) {
	comp->vid->time = 0;
// breakpoints
	if (!comp->debug) {
		bpChecker ch = comp_check_bp(comp, comp->cpu->regPC + comp->cpu->cs.base, MEM_BRK_FETCH | MEM_BRK_TFETCH);
		if (ch.t >= 0) {
			comp->brk = 1;
			comp->brkt = ch.t;
			comp->brka = ch.a;
			if (*ch.ptr & MEM_BRK_TFETCH) {
				*ch.ptr &= ~MEM_BRK_TFETCH;
				comp->brkt = -1;		// temp (not in list)
			}
			return 0;
		}
		if (comp->cpu->intrq && comp->brkirq) {
			comp->brk = 1;
			comp->brkt = BRK_IRQ;
			return 0;
		}
	}
// start
	res4 = 0;
// exec cpu opcode OR handle interrupt. get T states back
	res2 = cpu_exec(comp->cpu);
// scorpion WAIT: add 1T to odd-T command
	if (comp->evenM1 && (res2 & 1))
		res2++;
#ifdef HAVEZLIB
	if (comp->rzx.play) {
		if (comp->rzx.frm.fetches == 0) {
			if (comp->hw->irq)
				comp->hw->irq(comp, IRQ_RZX_INT);
		}
	}
#endif
	if (res2 > res4) {
		if (comp->hw->grp == HWG_ZX) {
			if (res2 > res4 + 1)
				vid_sync(comp->vid, (res2 - res4 - 1) * comp->nsPerTick);
			comp->cpu->flgACK = comp->vid->intFRAME ? 1 : 0;
			vid_sync(comp->vid, comp->nsPerTick);
		} else {
			vid_sync(comp->vid, (res2 - res4) * comp->nsPerTick);
		}
	}
// execution completed : get eated time & translate signals
	nsTime = comp->vid->time;
	comp->tickCount += res2;
	comp->frmtCount += res2;
// sync hardware
	if (comp->hw->sync)
		comp->hw->sync(comp, nsTime);
// new frame
	if (comp->vid->newFrame) {
		comp->vid->newFrame = 0;
		comp->frmStrobe = 1;
	}
// return ns eated @ this step
	return nsTime;
}

// cmos

unsigned char cmsRd(Computer* comp) {
	unsigned char res = 0xff;
	if (comp->cmos.adr >= 0x70) {
		switch(comp->cmos.mode) {
			case 0: res = comp->evo.bcVer[comp->cmos.adr & 0x0f]; break;
			case 1: res = comp->evo.blVer[comp->cmos.adr & 0x0f]; break;
			case 2: res = xt_read(comp->keyb); break; //keyReadCode(comp->keyb); break;		// read PC keyboard keycode
		}
	} else {
		res = cmos_rd(&comp->cmos, CMOS_DATA);
	}
	return res & 0xff;
}

void cmsWr(Computer* comp, int val) {
	switch (comp->cmos.adr) {
		case 0x0c:
			if (val & 1) {
				comp->keyb->outbuf = 0;
			}
			break;
		default:
			if (comp->cmos.adr > 0x6f) {
				comp->cmos.mode = val;	// write to F0..FF : set F0..FF reading mode
				//printf("cmos mode %i\n",val);
			} else {
				cmos_wr(&comp->cmos, CMOS_DATA, val);
			}
			break;
	}
}

// breaks

// activate breakpoint w/o type (exit to debuga)
void comp_brk(Computer* comp) {
	comp->brk = 1;
	comp->brkt = -1;
}

static unsigned char dumBrk = 0x00;

unsigned char* getBrkPtr(Computer* comp, int madr) {
	xAdr xadr = mem_get_xadr(comp->mem, madr);
	unsigned char* ptr = NULL;
	switch (xadr.type) {
		case MEM_RAM: ptr = comp->brkRamMap + (xadr.abs & comp->mem->ramMask); break;
		case MEM_ROM: ptr = comp->brkRomMap + (xadr.abs & comp->mem->romMask); break;
		case MEM_SLOT:
			if (comp->slot->brkMap)
				ptr = comp->slot->brkMap + (xadr.abs & comp->slot->memMask);
			break;
	}
	if (!ptr) {
		dumBrk = 0;
		ptr = &dumBrk;
	/*
	} else {
		if (!comp->brk && (*ptr & 0x0f)) {
			comp->brka = xadr.abs;
			switch (xadr.type) {
				case MEM_RAM: comp->brkt = BRK_MEMRAM; break;
				case MEM_ROM: comp->brkt = BRK_MEMROM; break;
				case MEM_SLOT: comp->brkt = BRK_MEMSLT; break;
				default: comp->brkt = BRK_MEMEXT; break;
			}
		}
	*/
	}
	return ptr;
}

void setBrk(Computer* comp, int adr, unsigned char val) {
	unsigned char* ptr = getBrkPtr(comp, adr);
	if (ptr == NULL) return;
	*ptr = (*ptr & 0xf0) | (val & 0x0f);
}

unsigned char getBrk(Computer* comp, int adr) {
	unsigned char* ptr = getBrkPtr(comp, adr);
	unsigned char res = ptr ? *ptr : 0x00;
	if (comp->mem->busmask < 0x10000) {
		res |= (comp->brkAdrMap[adr & comp->mem->busmask] & 0x0f);
	}
	return res;
}
