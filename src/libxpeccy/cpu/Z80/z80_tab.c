#include "../cpu.h"

const unsigned char FHaddTab[] = {0, 1, 1, 1, 0, 0, 0, 1};
const unsigned char FHsubTab[] = {0, 0, 1, 0, 1, 0, 1, 1};
const unsigned char FVaddTab[] = {0, 0, 0, 1, 1, 0, 0, 0};
const unsigned char FVsubTab[] = {0, 1, 0, 0, 0, 0, 1, 0};

/*table for daa, contains af*/

const unsigned char daaTab[0x1000] = {
        0x44,0x00,0x00,0x01,0x00,0x02,0x04,0x03
       ,0x00,0x04,0x04,0x05,0x04,0x06,0x00,0x07
       ,0x08,0x08,0x0c,0x09,0x10,0x10,0x14,0x11
       ,0x14,0x12,0x10,0x13,0x14,0x14,0x10,0x15
       ,0x00,0x10,0x04,0x11,0x04,0x12,0x00,0x13
       ,0x04,0x14,0x00,0x15,0x00,0x16,0x04,0x17
       ,0x0c,0x18,0x08,0x19,0x30,0x20,0x34,0x21
       ,0x34,0x22,0x30,0x23,0x34,0x24,0x30,0x25
       ,0x20,0x20,0x24,0x21,0x24,0x22,0x20,0x23
       ,0x24,0x24,0x20,0x25,0x20,0x26,0x24,0x27
       ,0x2c,0x28,0x28,0x29,0x34,0x30,0x30,0x31
       ,0x30,0x32,0x34,0x33,0x30,0x34,0x34,0x35
       ,0x24,0x30,0x20,0x31,0x20,0x32,0x24,0x33
       ,0x20,0x34,0x24,0x35,0x24,0x36,0x20,0x37
       ,0x28,0x38,0x2c,0x39,0x10,0x40,0x14,0x41
       ,0x14,0x42,0x10,0x43,0x14,0x44,0x10,0x45
       ,0x00,0x40,0x04,0x41,0x04,0x42,0x00,0x43
       ,0x04,0x44,0x00,0x45,0x00,0x46,0x04,0x47
       ,0x0c,0x48,0x08,0x49,0x14,0x50,0x10,0x51
       ,0x10,0x52,0x14,0x53,0x10,0x54,0x14,0x55
       ,0x04,0x50,0x00,0x51,0x00,0x52,0x04,0x53
       ,0x00,0x54,0x04,0x55,0x04,0x56,0x00,0x57
       ,0x08,0x58,0x0c,0x59,0x34,0x60,0x30,0x61
       ,0x30,0x62,0x34,0x63,0x30,0x64,0x34,0x65
       ,0x24,0x60,0x20,0x61,0x20,0x62,0x24,0x63
       ,0x20,0x64,0x24,0x65,0x24,0x66,0x20,0x67
       ,0x28,0x68,0x2c,0x69,0x30,0x70,0x34,0x71
       ,0x34,0x72,0x30,0x73,0x34,0x74,0x30,0x75
       ,0x20,0x70,0x24,0x71,0x24,0x72,0x20,0x73
       ,0x24,0x74,0x20,0x75,0x20,0x76,0x24,0x77
       ,0x2c,0x78,0x28,0x79,0x90,0x80,0x94,0x81
       ,0x94,0x82,0x90,0x83,0x94,0x84,0x90,0x85
       ,0x80,0x80,0x84,0x81,0x84,0x82,0x80,0x83
       ,0x84,0x84,0x80,0x85,0x80,0x86,0x84,0x87
       ,0x8c,0x88,0x88,0x89,0x94,0x90,0x90,0x91
       ,0x90,0x92,0x94,0x93,0x90,0x94,0x94,0x95
       ,0x84,0x90,0x80,0x91,0x80,0x92,0x84,0x93
       ,0x80,0x94,0x84,0x95,0x84,0x96,0x80,0x97
       ,0x88,0x98,0x8c,0x99,0x55,0x00,0x11,0x01
       ,0x11,0x02,0x15,0x03,0x11,0x04,0x15,0x05
       ,0x45,0x00,0x01,0x01,0x01,0x02,0x05,0x03
       ,0x01,0x04,0x05,0x05,0x05,0x06,0x01,0x07
       ,0x09,0x08,0x0d,0x09,0x11,0x10,0x15,0x11
       ,0x15,0x12,0x11,0x13,0x15,0x14,0x11,0x15
       ,0x01,0x10,0x05,0x11,0x05,0x12,0x01,0x13
       ,0x05,0x14,0x01,0x15,0x01,0x16,0x05,0x17
       ,0x0d,0x18,0x09,0x19,0x31,0x20,0x35,0x21
       ,0x35,0x22,0x31,0x23,0x35,0x24,0x31,0x25
       ,0x21,0x20,0x25,0x21,0x25,0x22,0x21,0x23
       ,0x25,0x24,0x21,0x25,0x21,0x26,0x25,0x27
       ,0x2d,0x28,0x29,0x29,0x35,0x30,0x31,0x31
       ,0x31,0x32,0x35,0x33,0x31,0x34,0x35,0x35
       ,0x25,0x30,0x21,0x31,0x21,0x32,0x25,0x33
       ,0x21,0x34,0x25,0x35,0x25,0x36,0x21,0x37
       ,0x29,0x38,0x2d,0x39,0x11,0x40,0x15,0x41
       ,0x15,0x42,0x11,0x43,0x15,0x44,0x11,0x45
       ,0x01,0x40,0x05,0x41,0x05,0x42,0x01,0x43
       ,0x05,0x44,0x01,0x45,0x01,0x46,0x05,0x47
       ,0x0d,0x48,0x09,0x49,0x15,0x50,0x11,0x51
       ,0x11,0x52,0x15,0x53,0x11,0x54,0x15,0x55
       ,0x05,0x50,0x01,0x51,0x01,0x52,0x05,0x53
       ,0x01,0x54,0x05,0x55,0x05,0x56,0x01,0x57
       ,0x09,0x58,0x0d,0x59,0x35,0x60,0x31,0x61
       ,0x31,0x62,0x35,0x63,0x31,0x64,0x35,0x65
       ,0x25,0x60,0x21,0x61,0x21,0x62,0x25,0x63
       ,0x21,0x64,0x25,0x65,0x25,0x66,0x21,0x67
       ,0x29,0x68,0x2d,0x69,0x31,0x70,0x35,0x71
       ,0x35,0x72,0x31,0x73,0x35,0x74,0x31,0x75
       ,0x21,0x70,0x25,0x71,0x25,0x72,0x21,0x73
       ,0x25,0x74,0x21,0x75,0x21,0x76,0x25,0x77
       ,0x2d,0x78,0x29,0x79,0x91,0x80,0x95,0x81
       ,0x95,0x82,0x91,0x83,0x95,0x84,0x91,0x85
       ,0x81,0x80,0x85,0x81,0x85,0x82,0x81,0x83
       ,0x85,0x84,0x81,0x85,0x81,0x86,0x85,0x87
       ,0x8d,0x88,0x89,0x89,0x95,0x90,0x91,0x91
       ,0x91,0x92,0x95,0x93,0x91,0x94,0x95,0x95
       ,0x85,0x90,0x81,0x91,0x81,0x92,0x85,0x93
       ,0x81,0x94,0x85,0x95,0x85,0x96,0x81,0x97
       ,0x89,0x98,0x8d,0x99,0xb5,0xa0,0xb1,0xa1
       ,0xb1,0xa2,0xb5,0xa3,0xb1,0xa4,0xb5,0xa5
       ,0xa5,0xa0,0xa1,0xa1,0xa1,0xa2,0xa5,0xa3
       ,0xa1,0xa4,0xa5,0xa5,0xa5,0xa6,0xa1,0xa7
       ,0xa9,0xa8,0xad,0xa9,0xb1,0xb0,0xb5,0xb1
       ,0xb5,0xb2,0xb1,0xb3,0xb5,0xb4,0xb1,0xb5
       ,0xa1,0xb0,0xa5,0xb1,0xa5,0xb2,0xa1,0xb3
       ,0xa5,0xb4,0xa1,0xb5,0xa1,0xb6,0xa5,0xb7
       ,0xad,0xb8,0xa9,0xb9,0x95,0xc0,0x91,0xc1
       ,0x91,0xc2,0x95,0xc3,0x91,0xc4,0x95,0xc5
       ,0x85,0xc0,0x81,0xc1,0x81,0xc2,0x85,0xc3
       ,0x81,0xc4,0x85,0xc5,0x85,0xc6,0x81,0xc7
       ,0x89,0xc8,0x8d,0xc9,0x91,0xd0,0x95,0xd1
       ,0x95,0xd2,0x91,0xd3,0x95,0xd4,0x91,0xd5
       ,0x81,0xd0,0x85,0xd1,0x85,0xd2,0x81,0xd3
       ,0x85,0xd4,0x81,0xd5,0x81,0xd6,0x85,0xd7
       ,0x8d,0xd8,0x89,0xd9,0xb1,0xe0,0xb5,0xe1
       ,0xb5,0xe2,0xb1,0xe3,0xb5,0xe4,0xb1,0xe5
       ,0xa1,0xe0,0xa5,0xe1,0xa5,0xe2,0xa1,0xe3
       ,0xa5,0xe4,0xa1,0xe5,0xa1,0xe6,0xa5,0xe7
       ,0xad,0xe8,0xa9,0xe9,0xb5,0xf0,0xb1,0xf1
       ,0xb1,0xf2,0xb5,0xf3,0xb1,0xf4,0xb5,0xf5
       ,0xa5,0xf0,0xa1,0xf1,0xa1,0xf2,0xa5,0xf3
       ,0xa1,0xf4,0xa5,0xf5,0xa5,0xf6,0xa1,0xf7
       ,0xa9,0xf8,0xad,0xf9,0x55,0x00,0x11,0x01
       ,0x11,0x02,0x15,0x03,0x11,0x04,0x15,0x05
       ,0x45,0x00,0x01,0x01,0x01,0x02,0x05,0x03
       ,0x01,0x04,0x05,0x05,0x05,0x06,0x01,0x07
       ,0x09,0x08,0x0d,0x09,0x11,0x10,0x15,0x11
       ,0x15,0x12,0x11,0x13,0x15,0x14,0x11,0x15
       ,0x01,0x10,0x05,0x11,0x05,0x12,0x01,0x13
       ,0x05,0x14,0x01,0x15,0x01,0x16,0x05,0x17
       ,0x0d,0x18,0x09,0x19,0x31,0x20,0x35,0x21
       ,0x35,0x22,0x31,0x23,0x35,0x24,0x31,0x25
       ,0x21,0x20,0x25,0x21,0x25,0x22,0x21,0x23
       ,0x25,0x24,0x21,0x25,0x21,0x26,0x25,0x27
       ,0x2d,0x28,0x29,0x29,0x35,0x30,0x31,0x31
       ,0x31,0x32,0x35,0x33,0x31,0x34,0x35,0x35
       ,0x25,0x30,0x21,0x31,0x21,0x32,0x25,0x33
       ,0x21,0x34,0x25,0x35,0x25,0x36,0x21,0x37
       ,0x29,0x38,0x2d,0x39,0x11,0x40,0x15,0x41
       ,0x15,0x42,0x11,0x43,0x15,0x44,0x11,0x45
       ,0x01,0x40,0x05,0x41,0x05,0x42,0x01,0x43
       ,0x05,0x44,0x01,0x45,0x01,0x46,0x05,0x47
       ,0x0d,0x48,0x09,0x49,0x15,0x50,0x11,0x51
       ,0x11,0x52,0x15,0x53,0x11,0x54,0x15,0x55
       ,0x05,0x50,0x01,0x51,0x01,0x52,0x05,0x53
       ,0x01,0x54,0x05,0x55,0x05,0x56,0x01,0x57
       ,0x09,0x58,0x0d,0x59,0x35,0x60,0x31,0x61
       ,0x31,0x62,0x35,0x63,0x31,0x64,0x35,0x65
       ,0x46,0x00,0x02,0x01,0x02,0x02,0x06,0x03
       ,0x02,0x04,0x06,0x05,0x06,0x06,0x02,0x07
       ,0x0a,0x08,0x0e,0x09,0x02,0x04,0x06,0x05
       ,0x06,0x06,0x02,0x07,0x0a,0x08,0x0e,0x09
       ,0x02,0x10,0x06,0x11,0x06,0x12,0x02,0x13
       ,0x06,0x14,0x02,0x15,0x02,0x16,0x06,0x17
       ,0x0e,0x18,0x0a,0x19,0x06,0x14,0x02,0x15
       ,0x02,0x16,0x06,0x17,0x0e,0x18,0x0a,0x19
       ,0x22,0x20,0x26,0x21,0x26,0x22,0x22,0x23
       ,0x26,0x24,0x22,0x25,0x22,0x26,0x26,0x27
       ,0x2e,0x28,0x2a,0x29,0x26,0x24,0x22,0x25
       ,0x22,0x26,0x26,0x27,0x2e,0x28,0x2a,0x29
       ,0x26,0x30,0x22,0x31,0x22,0x32,0x26,0x33
       ,0x22,0x34,0x26,0x35,0x26,0x36,0x22,0x37
       ,0x2a,0x38,0x2e,0x39,0x22,0x34,0x26,0x35
       ,0x26,0x36,0x22,0x37,0x2a,0x38,0x2e,0x39
       ,0x02,0x40,0x06,0x41,0x06,0x42,0x02,0x43
       ,0x06,0x44,0x02,0x45,0x02,0x46,0x06,0x47
       ,0x0e,0x48,0x0a,0x49,0x06,0x44,0x02,0x45
       ,0x02,0x46,0x06,0x47,0x0e,0x48,0x0a,0x49
       ,0x06,0x50,0x02,0x51,0x02,0x52,0x06,0x53
       ,0x02,0x54,0x06,0x55,0x06,0x56,0x02,0x57
       ,0x0a,0x58,0x0e,0x59,0x02,0x54,0x06,0x55
       ,0x06,0x56,0x02,0x57,0x0a,0x58,0x0e,0x59
       ,0x26,0x60,0x22,0x61,0x22,0x62,0x26,0x63
       ,0x22,0x64,0x26,0x65,0x26,0x66,0x22,0x67
       ,0x2a,0x68,0x2e,0x69,0x22,0x64,0x26,0x65
       ,0x26,0x66,0x22,0x67,0x2a,0x68,0x2e,0x69
       ,0x22,0x70,0x26,0x71,0x26,0x72,0x22,0x73
       ,0x26,0x74,0x22,0x75,0x22,0x76,0x26,0x77
       ,0x2e,0x78,0x2a,0x79,0x26,0x74,0x22,0x75
       ,0x22,0x76,0x26,0x77,0x2e,0x78,0x2a,0x79
       ,0x82,0x80,0x86,0x81,0x86,0x82,0x82,0x83
       ,0x86,0x84,0x82,0x85,0x82,0x86,0x86,0x87
       ,0x8e,0x88,0x8a,0x89,0x86,0x84,0x82,0x85
       ,0x82,0x86,0x86,0x87,0x8e,0x88,0x8a,0x89
       ,0x86,0x90,0x82,0x91,0x82,0x92,0x86,0x93
       ,0x82,0x94,0x86,0x95,0x86,0x96,0x82,0x97
       ,0x8a,0x98,0x8e,0x99,0x23,0x34,0x27,0x35
       ,0x27,0x36,0x23,0x37,0x2b,0x38,0x2f,0x39
       ,0x03,0x40,0x07,0x41,0x07,0x42,0x03,0x43
       ,0x07,0x44,0x03,0x45,0x03,0x46,0x07,0x47
       ,0x0f,0x48,0x0b,0x49,0x07,0x44,0x03,0x45
       ,0x03,0x46,0x07,0x47,0x0f,0x48,0x0b,0x49
       ,0x07,0x50,0x03,0x51,0x03,0x52,0x07,0x53
       ,0x03,0x54,0x07,0x55,0x07,0x56,0x03,0x57
       ,0x0b,0x58,0x0f,0x59,0x03,0x54,0x07,0x55
       ,0x07,0x56,0x03,0x57,0x0b,0x58,0x0f,0x59
       ,0x27,0x60,0x23,0x61,0x23,0x62,0x27,0x63
       ,0x23,0x64,0x27,0x65,0x27,0x66,0x23,0x67
       ,0x2b,0x68,0x2f,0x69,0x23,0x64,0x27,0x65
       ,0x27,0x66,0x23,0x67,0x2b,0x68,0x2f,0x69
       ,0x23,0x70,0x27,0x71,0x27,0x72,0x23,0x73
       ,0x27,0x74,0x23,0x75,0x23,0x76,0x27,0x77
       ,0x2f,0x78,0x2b,0x79,0x27,0x74,0x23,0x75
       ,0x23,0x76,0x27,0x77,0x2f,0x78,0x2b,0x79
       ,0x83,0x80,0x87,0x81,0x87,0x82,0x83,0x83
       ,0x87,0x84,0x83,0x85,0x83,0x86,0x87,0x87
       ,0x8f,0x88,0x8b,0x89,0x87,0x84,0x83,0x85
       ,0x83,0x86,0x87,0x87,0x8f,0x88,0x8b,0x89
       ,0x87,0x90,0x83,0x91,0x83,0x92,0x87,0x93
       ,0x83,0x94,0x87,0x95,0x87,0x96,0x83,0x97
       ,0x8b,0x98,0x8f,0x99,0x83,0x94,0x87,0x95
       ,0x87,0x96,0x83,0x97,0x8b,0x98,0x8f,0x99
       ,0xa7,0xa0,0xa3,0xa1,0xa3,0xa2,0xa7,0xa3
       ,0xa3,0xa4,0xa7,0xa5,0xa7,0xa6,0xa3,0xa7
       ,0xab,0xa8,0xaf,0xa9,0xa3,0xa4,0xa7,0xa5
       ,0xa7,0xa6,0xa3,0xa7,0xab,0xa8,0xaf,0xa9
       ,0xa3,0xb0,0xa7,0xb1,0xa7,0xb2,0xa3,0xb3
       ,0xa7,0xb4,0xa3,0xb5,0xa3,0xb6,0xa7,0xb7
       ,0xaf,0xb8,0xab,0xb9,0xa7,0xb4,0xa3,0xb5
       ,0xa3,0xb6,0xa7,0xb7,0xaf,0xb8,0xab,0xb9
       ,0x87,0xc0,0x83,0xc1,0x83,0xc2,0x87,0xc3
       ,0x83,0xc4,0x87,0xc5,0x87,0xc6,0x83,0xc7
       ,0x8b,0xc8,0x8f,0xc9,0x83,0xc4,0x87,0xc5
       ,0x87,0xc6,0x83,0xc7,0x8b,0xc8,0x8f,0xc9
       ,0x83,0xd0,0x87,0xd1,0x87,0xd2,0x83,0xd3
       ,0x87,0xd4,0x83,0xd5,0x83,0xd6,0x87,0xd7
       ,0x8f,0xd8,0x8b,0xd9,0x87,0xd4,0x83,0xd5
       ,0x83,0xd6,0x87,0xd7,0x8f,0xd8,0x8b,0xd9
       ,0xa3,0xe0,0xa7,0xe1,0xa7,0xe2,0xa3,0xe3
       ,0xa7,0xe4,0xa3,0xe5,0xa3,0xe6,0xa7,0xe7
       ,0xaf,0xe8,0xab,0xe9,0xa7,0xe4,0xa3,0xe5
       ,0xa3,0xe6,0xa7,0xe7,0xaf,0xe8,0xab,0xe9
       ,0xa7,0xf0,0xa3,0xf1,0xa3,0xf2,0xa7,0xf3
       ,0xa3,0xf4,0xa7,0xf5,0xa7,0xf6,0xa3,0xf7
       ,0xab,0xf8,0xaf,0xf9,0xa3,0xf4,0xa7,0xf5
       ,0xa7,0xf6,0xa3,0xf7,0xab,0xf8,0xaf,0xf9
       ,0x47,0x00,0x03,0x01,0x03,0x02,0x07,0x03
       ,0x03,0x04,0x07,0x05,0x07,0x06,0x03,0x07
       ,0x0b,0x08,0x0f,0x09,0x03,0x04,0x07,0x05
       ,0x07,0x06,0x03,0x07,0x0b,0x08,0x0f,0x09
       ,0x03,0x10,0x07,0x11,0x07,0x12,0x03,0x13
       ,0x07,0x14,0x03,0x15,0x03,0x16,0x07,0x17
       ,0x0f,0x18,0x0b,0x19,0x07,0x14,0x03,0x15
       ,0x03,0x16,0x07,0x17,0x0f,0x18,0x0b,0x19
       ,0x23,0x20,0x27,0x21,0x27,0x22,0x23,0x23
       ,0x27,0x24,0x23,0x25,0x23,0x26,0x27,0x27
       ,0x2f,0x28,0x2b,0x29,0x27,0x24,0x23,0x25
       ,0x23,0x26,0x27,0x27,0x2f,0x28,0x2b,0x29
       ,0x27,0x30,0x23,0x31,0x23,0x32,0x27,0x33
       ,0x23,0x34,0x27,0x35,0x27,0x36,0x23,0x37
       ,0x2b,0x38,0x2f,0x39,0x23,0x34,0x27,0x35
       ,0x27,0x36,0x23,0x37,0x2b,0x38,0x2f,0x39
       ,0x03,0x40,0x07,0x41,0x07,0x42,0x03,0x43
       ,0x07,0x44,0x03,0x45,0x03,0x46,0x07,0x47
       ,0x0f,0x48,0x0b,0x49,0x07,0x44,0x03,0x45
       ,0x03,0x46,0x07,0x47,0x0f,0x48,0x0b,0x49
       ,0x07,0x50,0x03,0x51,0x03,0x52,0x07,0x53
       ,0x03,0x54,0x07,0x55,0x07,0x56,0x03,0x57
       ,0x0b,0x58,0x0f,0x59,0x03,0x54,0x07,0x55
       ,0x07,0x56,0x03,0x57,0x0b,0x58,0x0f,0x59
       ,0x27,0x60,0x23,0x61,0x23,0x62,0x27,0x63
       ,0x23,0x64,0x27,0x65,0x27,0x66,0x23,0x67
       ,0x2b,0x68,0x2f,0x69,0x23,0x64,0x27,0x65
       ,0x27,0x66,0x23,0x67,0x2b,0x68,0x2f,0x69
       ,0x23,0x70,0x27,0x71,0x27,0x72,0x23,0x73
       ,0x27,0x74,0x23,0x75,0x23,0x76,0x27,0x77
       ,0x2f,0x78,0x2b,0x79,0x27,0x74,0x23,0x75
       ,0x23,0x76,0x27,0x77,0x2f,0x78,0x2b,0x79
       ,0x83,0x80,0x87,0x81,0x87,0x82,0x83,0x83
       ,0x87,0x84,0x83,0x85,0x83,0x86,0x87,0x87
       ,0x8f,0x88,0x8b,0x89,0x87,0x84,0x83,0x85
       ,0x83,0x86,0x87,0x87,0x8f,0x88,0x8b,0x89
       ,0x87,0x90,0x83,0x91,0x83,0x92,0x87,0x93
       ,0x83,0x94,0x87,0x95,0x87,0x96,0x83,0x97
       ,0x8b,0x98,0x8f,0x99,0x83,0x94,0x87,0x95
       ,0x87,0x96,0x83,0x97,0x8b,0x98,0x8f,0x99
       ,0x04,0x06,0x00,0x07,0x08,0x08,0x0c,0x09
       ,0x0c,0x0a,0x08,0x0b,0x0c,0x0c,0x08,0x0d
       ,0x08,0x0e,0x0c,0x0f,0x10,0x10,0x14,0x11
       ,0x14,0x12,0x10,0x13,0x14,0x14,0x10,0x15
       ,0x00,0x16,0x04,0x17,0x0c,0x18,0x08,0x19
       ,0x08,0x1a,0x0c,0x1b,0x08,0x1c,0x0c,0x1d
       ,0x0c,0x1e,0x08,0x1f,0x30,0x20,0x34,0x21
       ,0x34,0x22,0x30,0x23,0x34,0x24,0x30,0x25
       ,0x20,0x26,0x24,0x27,0x2c,0x28,0x28,0x29
       ,0x28,0x2a,0x2c,0x2b,0x28,0x2c,0x2c,0x2d
       ,0x2c,0x2e,0x28,0x2f,0x34,0x30,0x30,0x31
       ,0x30,0x32,0x34,0x33,0x30,0x34,0x34,0x35
       ,0x24,0x36,0x20,0x37,0x28,0x38,0x2c,0x39
       ,0x2c,0x3a,0x28,0x3b,0x2c,0x3c,0x28,0x3d
       ,0x28,0x3e,0x2c,0x3f,0x10,0x40,0x14,0x41
       ,0x14,0x42,0x10,0x43,0x14,0x44,0x10,0x45
       ,0x00,0x46,0x04,0x47,0x0c,0x48,0x08,0x49
       ,0x08,0x4a,0x0c,0x4b,0x08,0x4c,0x0c,0x4d
       ,0x0c,0x4e,0x08,0x4f,0x14,0x50,0x10,0x51
       ,0x10,0x52,0x14,0x53,0x10,0x54,0x14,0x55
       ,0x04,0x56,0x00,0x57,0x08,0x58,0x0c,0x59
       ,0x0c,0x5a,0x08,0x5b,0x0c,0x5c,0x08,0x5d
       ,0x08,0x5e,0x0c,0x5f,0x34,0x60,0x30,0x61
       ,0x30,0x62,0x34,0x63,0x30,0x64,0x34,0x65
       ,0x24,0x66,0x20,0x67,0x28,0x68,0x2c,0x69
       ,0x2c,0x6a,0x28,0x6b,0x2c,0x6c,0x28,0x6d
       ,0x28,0x6e,0x2c,0x6f,0x30,0x70,0x34,0x71
       ,0x34,0x72,0x30,0x73,0x34,0x74,0x30,0x75
       ,0x20,0x76,0x24,0x77,0x2c,0x78,0x28,0x79
       ,0x28,0x7a,0x2c,0x7b,0x28,0x7c,0x2c,0x7d
       ,0x2c,0x7e,0x28,0x7f,0x90,0x80,0x94,0x81
       ,0x94,0x82,0x90,0x83,0x94,0x84,0x90,0x85
       ,0x80,0x86,0x84,0x87,0x8c,0x88,0x88,0x89
       ,0x88,0x8a,0x8c,0x8b,0x88,0x8c,0x8c,0x8d
       ,0x8c,0x8e,0x88,0x8f,0x94,0x90,0x90,0x91
       ,0x90,0x92,0x94,0x93,0x90,0x94,0x94,0x95
       ,0x84,0x96,0x80,0x97,0x88,0x98,0x8c,0x99
       ,0x8c,0x9a,0x88,0x9b,0x8c,0x9c,0x88,0x9d
       ,0x88,0x9e,0x8c,0x9f,0x55,0x00,0x11,0x01
       ,0x11,0x02,0x15,0x03,0x11,0x04,0x15,0x05
       ,0x05,0x06,0x01,0x07,0x09,0x08,0x0d,0x09
       ,0x0d,0x0a,0x09,0x0b,0x0d,0x0c,0x09,0x0d
       ,0x09,0x0e,0x0d,0x0f,0x11,0x10,0x15,0x11
       ,0x15,0x12,0x11,0x13,0x15,0x14,0x11,0x15
       ,0x01,0x16,0x05,0x17,0x0d,0x18,0x09,0x19
       ,0x09,0x1a,0x0d,0x1b,0x09,0x1c,0x0d,0x1d
       ,0x0d,0x1e,0x09,0x1f,0x31,0x20,0x35,0x21
       ,0x35,0x22,0x31,0x23,0x35,0x24,0x31,0x25
       ,0x21,0x26,0x25,0x27,0x2d,0x28,0x29,0x29
       ,0x29,0x2a,0x2d,0x2b,0x29,0x2c,0x2d,0x2d
       ,0x2d,0x2e,0x29,0x2f,0x35,0x30,0x31,0x31
       ,0x31,0x32,0x35,0x33,0x31,0x34,0x35,0x35
       ,0x25,0x36,0x21,0x37,0x29,0x38,0x2d,0x39
       ,0x2d,0x3a,0x29,0x3b,0x2d,0x3c,0x29,0x3d
       ,0x29,0x3e,0x2d,0x3f,0x11,0x40,0x15,0x41
       ,0x15,0x42,0x11,0x43,0x15,0x44,0x11,0x45
       ,0x01,0x46,0x05,0x47,0x0d,0x48,0x09,0x49
       ,0x09,0x4a,0x0d,0x4b,0x09,0x4c,0x0d,0x4d
       ,0x0d,0x4e,0x09,0x4f,0x15,0x50,0x11,0x51
       ,0x11,0x52,0x15,0x53,0x11,0x54,0x15,0x55
       ,0x05,0x56,0x01,0x57,0x09,0x58,0x0d,0x59
       ,0x0d,0x5a,0x09,0x5b,0x0d,0x5c,0x09,0x5d
       ,0x09,0x5e,0x0d,0x5f,0x35,0x60,0x31,0x61
       ,0x31,0x62,0x35,0x63,0x31,0x64,0x35,0x65
       ,0x25,0x66,0x21,0x67,0x29,0x68,0x2d,0x69
       ,0x2d,0x6a,0x29,0x6b,0x2d,0x6c,0x29,0x6d
       ,0x29,0x6e,0x2d,0x6f,0x31,0x70,0x35,0x71
       ,0x35,0x72,0x31,0x73,0x35,0x74,0x31,0x75
       ,0x21,0x76,0x25,0x77,0x2d,0x78,0x29,0x79
       ,0x29,0x7a,0x2d,0x7b,0x29,0x7c,0x2d,0x7d
       ,0x2d,0x7e,0x29,0x7f,0x91,0x80,0x95,0x81
       ,0x95,0x82,0x91,0x83,0x95,0x84,0x91,0x85
       ,0x81,0x86,0x85,0x87,0x8d,0x88,0x89,0x89
       ,0x89,0x8a,0x8d,0x8b,0x89,0x8c,0x8d,0x8d
       ,0x8d,0x8e,0x89,0x8f,0x95,0x90,0x91,0x91
       ,0x91,0x92,0x95,0x93,0x91,0x94,0x95,0x95
       ,0x85,0x96,0x81,0x97,0x89,0x98,0x8d,0x99
       ,0x8d,0x9a,0x89,0x9b,0x8d,0x9c,0x89,0x9d
       ,0x89,0x9e,0x8d,0x9f,0xb5,0xa0,0xb1,0xa1
       ,0xb1,0xa2,0xb5,0xa3,0xb1,0xa4,0xb5,0xa5
       ,0xa5,0xa6,0xa1,0xa7,0xa9,0xa8,0xad,0xa9
       ,0xad,0xaa,0xa9,0xab,0xad,0xac,0xa9,0xad
       ,0xa9,0xae,0xad,0xaf,0xb1,0xb0,0xb5,0xb1
       ,0xb5,0xb2,0xb1,0xb3,0xb5,0xb4,0xb1,0xb5
       ,0xa1,0xb6,0xa5,0xb7,0xad,0xb8,0xa9,0xb9
       ,0xa9,0xba,0xad,0xbb,0xa9,0xbc,0xad,0xbd
       ,0xad,0xbe,0xa9,0xbf,0x95,0xc0,0x91,0xc1
       ,0x91,0xc2,0x95,0xc3,0x91,0xc4,0x95,0xc5
       ,0x85,0xc6,0x81,0xc7,0x89,0xc8,0x8d,0xc9
       ,0x8d,0xca,0x89,0xcb,0x8d,0xcc,0x89,0xcd
       ,0x89,0xce,0x8d,0xcf,0x91,0xd0,0x95,0xd1
       ,0x95,0xd2,0x91,0xd3,0x95,0xd4,0x91,0xd5
       ,0x81,0xd6,0x85,0xd7,0x8d,0xd8,0x89,0xd9
       ,0x89,0xda,0x8d,0xdb,0x89,0xdc,0x8d,0xdd
       ,0x8d,0xde,0x89,0xdf,0xb1,0xe0,0xb5,0xe1
       ,0xb5,0xe2,0xb1,0xe3,0xb5,0xe4,0xb1,0xe5
       ,0xa1,0xe6,0xa5,0xe7,0xad,0xe8,0xa9,0xe9
       ,0xa9,0xea,0xad,0xeb,0xa9,0xec,0xad,0xed
       ,0xad,0xee,0xa9,0xef,0xb5,0xf0,0xb1,0xf1
       ,0xb1,0xf2,0xb5,0xf3,0xb1,0xf4,0xb5,0xf5
       ,0xa5,0xf6,0xa1,0xf7,0xa9,0xf8,0xad,0xf9
       ,0xad,0xfa,0xa9,0xfb,0xad,0xfc,0xa9,0xfd
       ,0xa9,0xfe,0xad,0xff,0x55,0x00,0x11,0x01
       ,0x11,0x02,0x15,0x03,0x11,0x04,0x15,0x05
       ,0x05,0x06,0x01,0x07,0x09,0x08,0x0d,0x09
       ,0x0d,0x0a,0x09,0x0b,0x0d,0x0c,0x09,0x0d
       ,0x09,0x0e,0x0d,0x0f,0x11,0x10,0x15,0x11
       ,0x15,0x12,0x11,0x13,0x15,0x14,0x11,0x15
       ,0x01,0x16,0x05,0x17,0x0d,0x18,0x09,0x19
       ,0x09,0x1a,0x0d,0x1b,0x09,0x1c,0x0d,0x1d
       ,0x0d,0x1e,0x09,0x1f,0x31,0x20,0x35,0x21
       ,0x35,0x22,0x31,0x23,0x35,0x24,0x31,0x25
       ,0x21,0x26,0x25,0x27,0x2d,0x28,0x29,0x29
       ,0x29,0x2a,0x2d,0x2b,0x29,0x2c,0x2d,0x2d
       ,0x2d,0x2e,0x29,0x2f,0x35,0x30,0x31,0x31
       ,0x31,0x32,0x35,0x33,0x31,0x34,0x35,0x35
       ,0x25,0x36,0x21,0x37,0x29,0x38,0x2d,0x39
       ,0x2d,0x3a,0x29,0x3b,0x2d,0x3c,0x29,0x3d
       ,0x29,0x3e,0x2d,0x3f,0x11,0x40,0x15,0x41
       ,0x15,0x42,0x11,0x43,0x15,0x44,0x11,0x45
       ,0x01,0x46,0x05,0x47,0x0d,0x48,0x09,0x49
       ,0x09,0x4a,0x0d,0x4b,0x09,0x4c,0x0d,0x4d
       ,0x0d,0x4e,0x09,0x4f,0x15,0x50,0x11,0x51
       ,0x11,0x52,0x15,0x53,0x11,0x54,0x15,0x55
       ,0x05,0x56,0x01,0x57,0x09,0x58,0x0d,0x59
       ,0x0d,0x5a,0x09,0x5b,0x0d,0x5c,0x09,0x5d
       ,0x09,0x5e,0x0d,0x5f,0x35,0x60,0x31,0x61
       ,0x31,0x62,0x35,0x63,0x31,0x64,0x35,0x65
       ,0xbe,0xfa,0xba,0xfb,0xbe,0xfc,0xba,0xfd
       ,0xba,0xfe,0xbe,0xff,0x46,0x00,0x02,0x01
       ,0x02,0x02,0x06,0x03,0x02,0x04,0x06,0x05
       ,0x06,0x06,0x02,0x07,0x0a,0x08,0x0e,0x09
       ,0x1e,0x0a,0x1a,0x0b,0x1e,0x0c,0x1a,0x0d
       ,0x1a,0x0e,0x1e,0x0f,0x02,0x10,0x06,0x11
       ,0x06,0x12,0x02,0x13,0x06,0x14,0x02,0x15
       ,0x02,0x16,0x06,0x17,0x0e,0x18,0x0a,0x19
       ,0x1a,0x1a,0x1e,0x1b,0x1a,0x1c,0x1e,0x1d
       ,0x1e,0x1e,0x1a,0x1f,0x22,0x20,0x26,0x21
       ,0x26,0x22,0x22,0x23,0x26,0x24,0x22,0x25
       ,0x22,0x26,0x26,0x27,0x2e,0x28,0x2a,0x29
       ,0x3a,0x2a,0x3e,0x2b,0x3a,0x2c,0x3e,0x2d
       ,0x3e,0x2e,0x3a,0x2f,0x26,0x30,0x22,0x31
       ,0x22,0x32,0x26,0x33,0x22,0x34,0x26,0x35
       ,0x26,0x36,0x22,0x37,0x2a,0x38,0x2e,0x39
       ,0x3e,0x3a,0x3a,0x3b,0x3e,0x3c,0x3a,0x3d
       ,0x3a,0x3e,0x3e,0x3f,0x02,0x40,0x06,0x41
       ,0x06,0x42,0x02,0x43,0x06,0x44,0x02,0x45
       ,0x02,0x46,0x06,0x47,0x0e,0x48,0x0a,0x49
       ,0x1a,0x4a,0x1e,0x4b,0x1a,0x4c,0x1e,0x4d
       ,0x1e,0x4e,0x1a,0x4f,0x06,0x50,0x02,0x51
       ,0x02,0x52,0x06,0x53,0x02,0x54,0x06,0x55
       ,0x06,0x56,0x02,0x57,0x0a,0x58,0x0e,0x59
       ,0x1e,0x5a,0x1a,0x5b,0x1e,0x5c,0x1a,0x5d
       ,0x1a,0x5e,0x1e,0x5f,0x26,0x60,0x22,0x61
       ,0x22,0x62,0x26,0x63,0x22,0x64,0x26,0x65
       ,0x26,0x66,0x22,0x67,0x2a,0x68,0x2e,0x69
       ,0x3e,0x6a,0x3a,0x6b,0x3e,0x6c,0x3a,0x6d
       ,0x3a,0x6e,0x3e,0x6f,0x22,0x70,0x26,0x71
       ,0x26,0x72,0x22,0x73,0x26,0x74,0x22,0x75
       ,0x22,0x76,0x26,0x77,0x2e,0x78,0x2a,0x79
       ,0x3a,0x7a,0x3e,0x7b,0x3a,0x7c,0x3e,0x7d
       ,0x3e,0x7e,0x3a,0x7f,0x82,0x80,0x86,0x81
       ,0x86,0x82,0x82,0x83,0x86,0x84,0x82,0x85
       ,0x82,0x86,0x86,0x87,0x8e,0x88,0x8a,0x89
       ,0x9a,0x8a,0x9e,0x8b,0x9a,0x8c,0x9e,0x8d
       ,0x9e,0x8e,0x9a,0x8f,0x86,0x90,0x82,0x91
       ,0x82,0x92,0x86,0x93,0x23,0x34,0x27,0x35
       ,0x27,0x36,0x23,0x37,0x2b,0x38,0x2f,0x39
       ,0x3f,0x3a,0x3b,0x3b,0x3f,0x3c,0x3b,0x3d
       ,0x3b,0x3e,0x3f,0x3f,0x03,0x40,0x07,0x41
       ,0x07,0x42,0x03,0x43,0x07,0x44,0x03,0x45
       ,0x03,0x46,0x07,0x47,0x0f,0x48,0x0b,0x49
       ,0x1b,0x4a,0x1f,0x4b,0x1b,0x4c,0x1f,0x4d
       ,0x1f,0x4e,0x1b,0x4f,0x07,0x50,0x03,0x51
       ,0x03,0x52,0x07,0x53,0x03,0x54,0x07,0x55
       ,0x07,0x56,0x03,0x57,0x0b,0x58,0x0f,0x59
       ,0x1f,0x5a,0x1b,0x5b,0x1f,0x5c,0x1b,0x5d
       ,0x1b,0x5e,0x1f,0x5f,0x27,0x60,0x23,0x61
       ,0x23,0x62,0x27,0x63,0x23,0x64,0x27,0x65
       ,0x27,0x66,0x23,0x67,0x2b,0x68,0x2f,0x69
       ,0x3f,0x6a,0x3b,0x6b,0x3f,0x6c,0x3b,0x6d
       ,0x3b,0x6e,0x3f,0x6f,0x23,0x70,0x27,0x71
       ,0x27,0x72,0x23,0x73,0x27,0x74,0x23,0x75
       ,0x23,0x76,0x27,0x77,0x2f,0x78,0x2b,0x79
       ,0x3b,0x7a,0x3f,0x7b,0x3b,0x7c,0x3f,0x7d
       ,0x3f,0x7e,0x3b,0x7f,0x83,0x80,0x87,0x81
       ,0x87,0x82,0x83,0x83,0x87,0x84,0x83,0x85
       ,0x83,0x86,0x87,0x87,0x8f,0x88,0x8b,0x89
       ,0x9b,0x8a,0x9f,0x8b,0x9b,0x8c,0x9f,0x8d
       ,0x9f,0x8e,0x9b,0x8f,0x87,0x90,0x83,0x91
       ,0x83,0x92,0x87,0x93,0x83,0x94,0x87,0x95
       ,0x87,0x96,0x83,0x97,0x8b,0x98,0x8f,0x99
       ,0x9f,0x9a,0x9b,0x9b,0x9f,0x9c,0x9b,0x9d
       ,0x9b,0x9e,0x9f,0x9f,0xa7,0xa0,0xa3,0xa1
       ,0xa3,0xa2,0xa7,0xa3,0xa3,0xa4,0xa7,0xa5
       ,0xa7,0xa6,0xa3,0xa7,0xab,0xa8,0xaf,0xa9
       ,0xbf,0xaa,0xbb,0xab,0xbf,0xac,0xbb,0xad
       ,0xbb,0xae,0xbf,0xaf,0xa3,0xb0,0xa7,0xb1
       ,0xa7,0xb2,0xa3,0xb3,0xa7,0xb4,0xa3,0xb5
       ,0xa3,0xb6,0xa7,0xb7,0xaf,0xb8,0xab,0xb9
       ,0xbb,0xba,0xbf,0xbb,0xbb,0xbc,0xbf,0xbd
       ,0xbf,0xbe,0xbb,0xbf,0x87,0xc0,0x83,0xc1
       ,0x83,0xc2,0x87,0xc3,0x83,0xc4,0x87,0xc5
       ,0x87,0xc6,0x83,0xc7,0x8b,0xc8,0x8f,0xc9
       ,0x9f,0xca,0x9b,0xcb,0x9f,0xcc,0x9b,0xcd
       ,0x9b,0xce,0x9f,0xcf,0x83,0xd0,0x87,0xd1
       ,0x87,0xd2,0x83,0xd3,0x87,0xd4,0x83,0xd5
       ,0x83,0xd6,0x87,0xd7,0x8f,0xd8,0x8b,0xd9
       ,0x9b,0xda,0x9f,0xdb,0x9b,0xdc,0x9f,0xdd
       ,0x9f,0xde,0x9b,0xdf,0xa3,0xe0,0xa7,0xe1
       ,0xa7,0xe2,0xa3,0xe3,0xa7,0xe4,0xa3,0xe5
       ,0xa3,0xe6,0xa7,0xe7,0xaf,0xe8,0xab,0xe9
       ,0xbb,0xea,0xbf,0xeb,0xbb,0xec,0xbf,0xed
       ,0xbf,0xee,0xbb,0xef,0xa7,0xf0,0xa3,0xf1
       ,0xa3,0xf2,0xa7,0xf3,0xa3,0xf4,0xa7,0xf5
       ,0xa7,0xf6,0xa3,0xf7,0xab,0xf8,0xaf,0xf9
       ,0xbf,0xfa,0xbb,0xfb,0xbf,0xfc,0xbb,0xfd
       ,0xbb,0xfe,0xbf,0xff,0x47,0x00,0x03,0x01
       ,0x03,0x02,0x07,0x03,0x03,0x04,0x07,0x05
       ,0x07,0x06,0x03,0x07,0x0b,0x08,0x0f,0x09
       ,0x1f,0x0a,0x1b,0x0b,0x1f,0x0c,0x1b,0x0d
       ,0x1b,0x0e,0x1f,0x0f,0x03,0x10,0x07,0x11
       ,0x07,0x12,0x03,0x13,0x07,0x14,0x03,0x15
       ,0x03,0x16,0x07,0x17,0x0f,0x18,0x0b,0x19
       ,0x1b,0x1a,0x1f,0x1b,0x1b,0x1c,0x1f,0x1d
       ,0x1f,0x1e,0x1b,0x1f,0x23,0x20,0x27,0x21
       ,0x27,0x22,0x23,0x23,0x27,0x24,0x23,0x25
       ,0x23,0x26,0x27,0x27,0x2f,0x28,0x2b,0x29
       ,0x3b,0x2a,0x3f,0x2b,0x3b,0x2c,0x3f,0x2d
       ,0x3f,0x2e,0x3b,0x2f,0x27,0x30,0x23,0x31
       ,0x23,0x32,0x27,0x33,0x23,0x34,0x27,0x35
       ,0x27,0x36,0x23,0x37,0x2b,0x38,0x2f,0x39
       ,0x3f,0x3a,0x3b,0x3b,0x3f,0x3c,0x3b,0x3d
       ,0x3b,0x3e,0x3f,0x3f,0x03,0x40,0x07,0x41
       ,0x07,0x42,0x03,0x43,0x07,0x44,0x03,0x45
       ,0x03,0x46,0x07,0x47,0x0f,0x48,0x0b,0x49
       ,0x1b,0x4a,0x1f,0x4b,0x1b,0x4c,0x1f,0x4d
       ,0x1f,0x4e,0x1b,0x4f,0x07,0x50,0x03,0x51
       ,0x03,0x52,0x07,0x53,0x03,0x54,0x07,0x55
       ,0x07,0x56,0x03,0x57,0x0b,0x58,0x0f,0x59
       ,0x1f,0x5a,0x1b,0x5b,0x1f,0x5c,0x1b,0x5d
       ,0x1b,0x5e,0x1f,0x5f,0x27,0x60,0x23,0x61
       ,0x23,0x62,0x27,0x63,0x23,0x64,0x27,0x65
       ,0x27,0x66,0x23,0x67,0x2b,0x68,0x2f,0x69
       ,0x3f,0x6a,0x3b,0x6b,0x3f,0x6c,0x3b,0x6d
       ,0x3b,0x6e,0x3f,0x6f,0x23,0x70,0x27,0x71
       ,0x27,0x72,0x23,0x73,0x27,0x74,0x23,0x75
       ,0x23,0x76,0x27,0x77,0x2f,0x78,0x2b,0x79
       ,0x3b,0x7a,0x3f,0x7b,0x3b,0x7c,0x3f,0x7d
       ,0x3f,0x7e,0x3b,0x7f,0x83,0x80,0x87,0x81
       ,0x87,0x82,0x83,0x83,0x87,0x84,0x83,0x85
       ,0x83,0x86,0x87,0x87,0x8f,0x88,0x8b,0x89
       ,0x9b,0x8a,0x9f,0x8b,0x9b,0x8c,0x9f,0x8d
       ,0x9f,0x8e,0x9b,0x8f,0x87,0x90,0x83,0x91
       ,0x83,0x92,0x87,0x93,0x83,0x94,0x87,0x95
       ,0x87,0x96,0x83,0x97,0x8b,0x98,0x8f,0x99
};
