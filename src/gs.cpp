#include <stdlib.h>
#include <string.h>
#include "z80ex.h"
#include "memory.h"

#define GS_SELF_INCLUDE
#include "gs.h"
#define GS_FRQ		12.0

struct GSound {
	int flags;
	Z80EX_CONTEXT* cpu;
	Memory* mem;
	uint8_t pb3_gs;	// gs -> zx
	uint8_t pb3_zx;	// zx -> gs
	uint8_t pbb_zx;
	uint8_t rp0;
	uint8_t pstate;	// state register (d0, d7)
	int vol1,vol2,vol3,vol4;
	int ch1,ch2,ch3,ch4;
	int cnt;
	int stereo;
	double counter;
};

Z80EX_BYTE gsmemrd(Z80EX_CONTEXT*,Z80EX_WORD adr,int,void* ptr) {
	GSound* gs = (GSound*)ptr;
	Z80EX_BYTE res = memRd(gs->mem,adr);
	switch (adr & 0xe300) {
		case 0x6000: gs->ch1 = res; break;
		case 0x6100: gs->ch2 = res; break;
		case 0x6200: gs->ch3 = res; break;
		case 0x6300: gs->ch4 = res; break;
	}
	return res;
}

void gsmemwr(Z80EX_CONTEXT*,Z80EX_WORD adr,Z80EX_BYTE val,void* ptr) {
	GSound* gs = (GSound*)ptr;
	memWr(gs->mem,adr,val);
}

// internal IN
Z80EX_BYTE gsiord(Z80EX_CONTEXT*,Z80EX_WORD port,void* ptr) {
	GSound* gs = (GSound*)ptr;
	Z80EX_BYTE res = 0xff;
	port &= 0x0f;
	switch (port) {
		case 0: break;
		case 1: res = gs->pbb_zx; break;
		case 2: gs->pstate &= 0x7f; res = gs->pb3_zx; break;
		case 3: gs->pstate |= 0x80; break;
		case 4: res = gs->pstate; break;
		case 5: gs->pstate &= 0xfe; break;
		case 6: break;
		case 7: break;
		case 8: break;
		case 9: break;
		case 10: if (gs->rp0 & 0x01) gs->pstate &= 0x7f; else gs->pstate |= 0x80; break;
		case 11: if (gs->vol1 & 0x20) gs->pstate |= 1; else gs->pstate &= 0xfe; break;
	}
	return res;
}

void gsiowr(Z80EX_CONTEXT*,Z80EX_WORD port,Z80EX_BYTE val,void* ptr) {
	GSound* gs = (GSound*)ptr;
	port &= 0x0f;
	switch (port) {
		case 0: gs->rp0 = val;
			val &= 7;
			if (val == 0) {
				memSetBank(gs->mem,MEM_BANK2,MEM_ROM,0);	// gs->mem->pt2 = &gs->mem->rom[0][0];
				memSetBank(gs->mem,MEM_BANK3,MEM_ROM,1);	// gs->mem->pt3 = &gs->mem->rom[1][0];
			} else {
				memSetBank(gs->mem,MEM_BANK2,MEM_RAM,(val << 1) - 2);	// gs->mem->pt2 = &gs->mem->ram[(val << 1) - 2][0];
				memSetBank(gs->mem,MEM_BANK3,MEM_RAM,(val << 1) - 1);	// gs->mem->pt3 = &gs->mem->ram[(val << 1) - 1][0];
			}
			break;
		case 1: break;
		case 2: gs->pstate &= 0x7f; break;
		case 3: gs->pstate |= 0x80; gs->pb3_gs = val; break;
		case 4: break;
		case 5: gs->pstate &= 0xfe; break;
		case 6: gs->vol1 = val & 0x3f; break;
		case 7: gs->vol2 = val & 0x3f; break;
		case 8: gs->vol3 = val & 0x3f; break;
		case 9: gs->vol4 = val & 0x3f; break;
		case 10: if (gs->rp0 & 1) gs->pstate &= 0x7f; else gs->pstate |= 0x80; break;
		case 11: if (gs->vol1 & 64) gs->pstate |= 1; else gs->pstate &= 0xfe; break;
	}
}

Z80EX_BYTE gsintrq(Z80EX_CONTEXT*,void*) {
	return 0xff;
}

int gsGetFlag(GSound* gs) {
	return gs->flags;
}

void gsSetFlag(GSound* gs, int fl) {
	gs->flags = fl;
}

int gsGetParam(GSound* gs, int tp) {
	int res = 0;
	switch (tp) {
		case GS_STEREO: res = gs->stereo; break;
	}
	return res;
}

void gsSetParam(GSound* gs, int tp, int val) {
	switch(tp) {
		case GS_STEREO: gs->stereo = val; break;
	}
}

void gsSetRom(GSound* gs, int part, char* buf) {
	memSetPage(gs->mem,MEM_ROM,part,buf);
}

GSound* gsCreate() {
	GSound* res = (GSound*)malloc(sizeof(GSound));
	void* ptr = (void*)res;
	res->cpu = z80ex_create(&gsmemrd,ptr,&gsmemwr,ptr,&gsiord,ptr,&gsiowr,ptr,&gsintrq,ptr);
	res->mem = memCreate();	// new Memory();
	memSet(res->mem,MEM_MEMSIZE,512);
	memSetBank(res->mem,MEM_BANK0,MEM_ROM,0);
	memSetBank(res->mem,MEM_BANK1,MEM_RAM,0);
	memSetBank(res->mem,MEM_BANK2,MEM_RAM,0);
	memSetBank(res->mem,MEM_BANK3,MEM_RAM,1);
	res->cnt = 0;
	res->pstate = 0x7e;
	res->flags = GS_ENABLE;
	res->stereo = GS_12_34;
	res->counter = 0;
	res->ch1 = 0;
	res->ch2 = 0;
	res->ch3 = 0;
	res->ch4 = 0;
	res->vol1 = 0;
	res->vol2 = 0;
	res->vol3 = 0;
	res->vol4 = 0;
	return res;
}

void gsDestroy(GSound* gs) {
	free(gs);
}

void gsReset(GSound* gs) {
	z80ex_reset(gs->cpu);
}

void gsFlush(GSound* gs) {
	int res;
//	double ln = tk * GS_FRQ / 7.0;		// scale to GS ticks;
//	gs->counter += ln;
//	gs->t = tk;
	if (~gs->flags & GS_ENABLE) return;
	while (gs->counter > 0) {
		res = 0;
		do {
			res += z80ex_step(gs->cpu);
		} while (z80ex_last_op_type(gs->cpu) != 0);
		gs->counter -= res;
		gs->cnt += res;
		if (gs->cnt > 320) {	// 12MHz CLK, 37.5KHz INT -> int in each 320 ticks
			gs->cnt -= 320;
			res = z80ex_int(gs->cpu);
			gs->cnt += res;
			gs->counter -= res;
		}
	}
}

std::pair<uint8_t,uint8_t> gsGetVolume(GSound* gs) {
	std::pair<uint8_t,uint8_t> res;
	res.first = 0;
	res.second = 0;
	if (~gs->flags & GS_ENABLE) return res;
	gsFlush(gs);
	switch (gs->stereo) {
		case GS_MONO:
			res.first = ((gs->ch1 * gs->vol1 + \
				gs->ch2 * gs->vol2 + \
				gs->ch3 * gs->vol3 + \
				gs->ch4 * gs->vol4) >> 9);
			res.second = res.first;
			break;
		case GS_12_34:
			res.first = ((gs->ch1 * gs->vol1 + gs->ch2 * gs->vol2) >> 8);
			res.second = ((gs->ch3 * gs->vol3 + gs->ch4 * gs->vol4) >> 8);
			break;
	}
	return res;
}

void gsSync(GSound* gs, uint32_t tk) {
	if (~gs->flags & GS_ENABLE) return;
	gs->counter += tk * GS_FRQ / 7.0;
}

// external in/out

int gsIn(GSound* gs, int prt, uint8_t* val) {
	if (~gs->flags & GS_ENABLE) return GS_ERR;	// gs disabled
	if ((prt & 0x44) != 0) return GS_ERR;		// port don't catched
	gsFlush(gs);
	if (prt & 8) {
		*val = gs->pstate;
	} else {
		*val = gs->pb3_gs;
		gs->pstate &= 0x7f;		// reset b7,state
	}
	return GS_OK;
}

int gsOut(GSound* gs, int prt,uint8_t val) {
	if (~gs->flags & GS_ENABLE) return GS_ERR;
	if ((prt & 0x44) != 0) return GS_ERR;
	gsFlush(gs);
	if (prt & 8) {
		gs->pbb_zx = val;
		gs->pstate |= 1;
	} else {
		gs->pb3_zx = val;
		gs->pstate |= 0x80;	// set b7,state
	}
	return GS_OK;
}
