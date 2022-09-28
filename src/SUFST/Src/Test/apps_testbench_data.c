/***************************************************************************
 * @file   apps_testbench_data.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-03-08
 * @brief  Data for APPS input test bench
 ***************************************************************************/

#include "Test/apps_testbench_data.h"

#if RUN_APPS_TESTBENCH

const uint16_t standing_start_time_lookup[1001] = {
    0x0000, 0x013d, 0x0219, 0x02c9, 0x0361, 0x03e8, 0x0463, 0x04d4, 0x053e,
    0x05a1, 0x05ff, 0x0659, 0x06af, 0x0701, 0x0750, 0x079d, 0x07e7, 0x082f,
    0x0874, 0x08b8, 0x08fb, 0x093b, 0x097b, 0x09b9, 0x09f5, 0x0a31, 0x0a6b,
    0x0aa5, 0x0add, 0x0b15, 0x0b4b, 0x0b82, 0x0bb7, 0x0bed, 0x0c22, 0x0c56,
    0x0c8b, 0x0cbf, 0x0cf2, 0x0d26, 0x0d59, 0x0d8b, 0x0dbd, 0x0dee, 0x0e20,
    0x0e53, 0x0e88, 0x0ec1, 0x0efb, 0x0f38, 0x0f78, 0x0fb9, 0x0ffc, 0x1040,
    0x1087, 0x10cf, 0x1119, 0x1164, 0x11b0, 0x11fc, 0x1249, 0x1296, 0x12e4,
    0x1330, 0x137d, 0x13c9, 0x1414, 0x145d, 0x14a5, 0x14ea, 0x152d, 0x1570,
    0x15b1, 0x15f2, 0x1632, 0x1672, 0x16b0, 0x16ee, 0x172b, 0x1767, 0x17a2,
    0x17dc, 0x1815, 0x184d, 0x1885, 0x18bb, 0x18f1, 0x1926, 0x195a, 0x198d,
    0x19c0, 0x19f2, 0x1a24, 0x1a55, 0x1a85, 0x1ab5, 0x1ae4, 0x1b13, 0x1b41,
    0x1b6f, 0x1b9d, 0x1bca, 0x1bf6, 0x1c22, 0x1c4e, 0x1c7a, 0x1ca5, 0x1cd0,
    0x1cfd, 0x1d2c, 0x1d5c, 0x1d8f, 0x1dc4, 0x1dfb, 0x1e35, 0x1e72, 0x1eb3,
    0x1ef7, 0x1f3f, 0x1f8c, 0x1fdd, 0x2032, 0x208b, 0x20e9, 0x214a, 0x21ac,
    0x220e, 0x2272, 0x22d7, 0x233b, 0x239f, 0x2400, 0x245e, 0x24ba, 0x2513,
    0x2568, 0x25bb, 0x260c, 0x265a, 0x26a6, 0x26f1, 0x2739, 0x2780, 0x27c5,
    0x2810, 0x2860, 0x28b3, 0x290a, 0x2962, 0x29bc, 0x2a19, 0x2a79, 0x2adb,
    0x2b41, 0x2ba7, 0x2c0c, 0x2c6f, 0x2ccf, 0x2d2c, 0x2d84, 0x2dda, 0x2e2c,
    0x2e7b, 0x2ec8, 0x2f12, 0x2f59, 0x2f9f, 0x2fe3, 0x3025, 0x3066, 0x30a5,
    0x30e3, 0x3120, 0x315b, 0x3196, 0x31cf, 0x3208, 0x323f, 0x3276, 0x32ab,
    0x32e0, 0x3315, 0x3348, 0x337b, 0x33ad, 0x33df, 0x3410, 0x3440, 0x3470,
    0x349f, 0x34ce, 0x34fd, 0x352b, 0x3558, 0x3585, 0x35b2, 0x35de, 0x360a,
    0x3635, 0x3660, 0x368b, 0x36b6, 0x36e0, 0x3709, 0x3733, 0x375c, 0x3785,
    0x37ae, 0x37d9, 0x3803, 0x382f, 0x385c, 0x3889, 0x38b8, 0x38e7, 0x3917,
    0x3948, 0x397b, 0x39ae, 0x39e4, 0x3a1b, 0x3a56, 0x3a93, 0x3ad4, 0x3b18,
    0x3b5e, 0x3ba6, 0x3bf0, 0x3c3a, 0x3c85, 0x3cd0, 0x3d1b, 0x3d66, 0x3db0,
    0x3df9, 0x3e41, 0x3e87, 0x3ecb, 0x3f0d, 0x3f4d, 0x3f8d, 0x3fca, 0x4007,
    0x4043, 0x407d, 0x40b6, 0x40ef, 0x4126, 0x415d, 0x4192, 0x41c7, 0x41fb,
    0x422f, 0x4262, 0x4294, 0x42c6, 0x42f7, 0x4327, 0x4357, 0x4386, 0x43b5,
    0x43e3, 0x4411, 0x443f, 0x446c, 0x4498, 0x44c5, 0x44f0, 0x451c, 0x4547,
    0x4572, 0x459c, 0x45c7, 0x45f2, 0x461e, 0x464b, 0x4679, 0x46a8, 0x46d9,
    0x470a, 0x473c, 0x4770, 0x47a5, 0x47db, 0x4813, 0x484c, 0x4888, 0x48c6,
    0x4909, 0x4950, 0x499b, 0x49e9, 0x4a39, 0x4a8b, 0x4add, 0x4b30, 0x4b83,
    0x4bd6, 0x4c26, 0x4c73, 0x4cbd, 0x4d06, 0x4d4c, 0x4d90, 0x4dd3, 0x4e14,
    0x4e54, 0x4e93, 0x4ed3, 0x4f16, 0x4f5b, 0x4fa2, 0x4fea, 0x5035, 0x5082,
    0x50d0, 0x511e, 0x516b, 0x51b6, 0x51ff, 0x524d, 0x529f, 0x52f5, 0x534e,
    0x53a7, 0x5401, 0x545a, 0x54b1, 0x5504, 0x5554, 0x55a7, 0x55fe, 0x5658,
    0x56b3, 0x570e, 0x5768, 0x57c1, 0x5815, 0x5866, 0x58b9, 0x590f, 0x5967,
    0x59bf, 0x5a15, 0x5a6a, 0x5abb, 0x5b09, 0x5b55, 0x5b9f, 0x5be6, 0x5c2b,
    0x5c6f, 0x5cb2, 0x5cf3, 0x5d33, 0x5d73, 0x5db1, 0x5dee, 0x5e2b, 0x5e66,
    0x5ea0, 0x5ed9, 0x5f11, 0x5f49, 0x5f7f, 0x5fb5, 0x5feb, 0x6020, 0x6054,
    0x6088, 0x60bc, 0x60ef, 0x6121, 0x6152, 0x6183, 0x61b4, 0x61e4, 0x6213,
    0x6242, 0x6270, 0x629e, 0x62cb, 0x62f8, 0x6325, 0x6351, 0x637d, 0x63a8,
    0x63d4, 0x63fe, 0x6429, 0x6453, 0x647c, 0x64a6, 0x64cf, 0x64f8, 0x6520,
    0x6549, 0x6571, 0x6598, 0x65c0, 0x65e7, 0x660e, 0x6635, 0x665b, 0x6682,
    0x66a8, 0x66ce, 0x66f4, 0x6719, 0x673e, 0x6764, 0x6789, 0x67ae, 0x67d3,
    0x67f7, 0x681c, 0x6841, 0x6866, 0x688a, 0x68af, 0x68d3, 0x68f7, 0x691c,
    0x6940, 0x6965, 0x6989, 0x69ae, 0x69d3, 0x69f8, 0x6a1d, 0x6a42, 0x6a67,
    0x6a8b, 0x6ab0, 0x6ad5, 0x6af9, 0x6b1e, 0x6b42, 0x6b67, 0x6b8b, 0x6bb0,
    0x6bd4, 0x6bf9, 0x6c1e, 0x6c44, 0x6c6a, 0x6c92, 0x6cbb, 0x6ce5, 0x6d11,
    0x6d3d, 0x6d6c, 0x6d9c, 0x6dcf, 0x6e04, 0x6e3b, 0x6e75, 0x6eb2, 0x6ef3,
    0x6f38, 0x6f81, 0x6fcf, 0x7022, 0x7079, 0x70d4, 0x7135, 0x719b, 0x7207,
    0x7279, 0x72ef, 0x7367, 0x73e3, 0x7460, 0x74db, 0x7550, 0x75c0, 0x762a,
    0x768e, 0x76ef, 0x774b, 0x77a3, 0x77f7, 0x7848, 0x7897, 0x78e2, 0x792b,
    0x7973, 0x79b8, 0x79fb, 0x7a3d, 0x7a7d, 0x7abe, 0x7b02, 0x7b46, 0x7b8b,
    0x7bce, 0x7c11, 0x7c5a, 0x7ca7, 0x7cf6, 0x7d47, 0x7d97, 0x7de6, 0x7e31,
    0x7e7a, 0x7ec2, 0x7f08, 0x7f4d, 0x7f90, 0x7fd2, 0x8013, 0x8052, 0x8090,
    0x80cd, 0x8108, 0x8143, 0x817c, 0x81b4, 0x81ec, 0x8223, 0x8259, 0x828e,
    0x82c4, 0x82f9, 0x832d, 0x8361, 0x8393, 0x83c6, 0x83fb, 0x8431, 0x8468,
    0x84a0, 0x84d8, 0x850f, 0x8546, 0x857c, 0x85b1, 0x85e6, 0x861b, 0x8651,
    0x8686, 0x86bc, 0x86f0, 0x8724, 0x8758, 0x878a, 0x87bd, 0x87ef, 0x8821,
    0x8852, 0x8883, 0x88b4, 0x88e4, 0x8913, 0x8942, 0x8971, 0x899f, 0x89cd,
    0x89fa, 0x8a27, 0x8a53, 0x8a80, 0x8aab, 0x8ad7, 0x8b02, 0x8b2c, 0x8b57,
    0x8b81, 0x8bab, 0x8bd4, 0x8bfd, 0x8c26, 0x8c4e, 0x8c77, 0x8c9f, 0x8cc6,
    0x8cee, 0x8d15, 0x8d3c, 0x8d63, 0x8d89, 0x8db0, 0x8dd6, 0x8dfc, 0x8e21,
    0x8e47, 0x8e6c, 0x8e91, 0x8eb6, 0x8edb, 0x8eff, 0x8f24, 0x8f48, 0x8f6d,
    0x8f91, 0x8fb6, 0x8fda, 0x8fff, 0x9023, 0x9048, 0x906c, 0x9091, 0x90b5,
    0x90db, 0x9101, 0x9127, 0x914f, 0x9177, 0x91a0, 0x91ca, 0x91f4, 0x921e,
    0x924a, 0x9276, 0x92a2, 0x92cf, 0x92fd, 0x932b, 0x9359, 0x9388, 0x93b8,
    0x93e9, 0x941a, 0x944d, 0x9481, 0x94b6, 0x94ef, 0x952a, 0x9568, 0x95a9,
    0x95ec, 0x9631, 0x9677, 0x96be, 0x9706, 0x974f, 0x9798, 0x97e0, 0x9828,
    0x986e, 0x98b2, 0x98f4, 0x9939, 0x9980, 0x99c9, 0x9a12, 0x9a5a, 0x9aa2,
    0x9aea, 0x9b31, 0x9b77, 0x9bbc, 0x9c01, 0x9c45, 0x9c88, 0x9cc9, 0x9d09,
    0x9d48, 0x9d86, 0x9dc2, 0x9dff, 0x9e3b, 0x9e77, 0x9eb4, 0x9ef2, 0x9f31,
    0x9f70, 0x9fb0, 0x9ff1, 0xa031, 0xa072, 0xa0b2, 0xa0f3, 0xa133, 0xa174,
    0xa1b4, 0xa1f3, 0xa231, 0xa26e, 0xa2a9, 0xa2e4, 0xa31e, 0xa358, 0xa391,
    0xa3ca, 0xa402, 0xa43b, 0xa473, 0xa4ab, 0xa4e3, 0xa51a, 0xa552, 0xa589,
    0xa5c0, 0xa5f6, 0xa62c, 0xa661, 0xa695, 0xa6c9, 0xa6fc, 0xa72f, 0xa760,
    0xa791, 0xa7c2, 0xa7f2, 0xa822, 0xa851, 0xa87f, 0xa8ad, 0xa8db, 0xa908,
    0xa935, 0xa961, 0xa98d, 0xa9b9, 0xa9e4, 0xaa0f, 0xaa39, 0xaa63, 0xaa8d,
    0xaab7, 0xaae0, 0xab09, 0xab31, 0xab5a, 0xab82, 0xabaa, 0xabd1, 0xabf9,
    0xac20, 0xac47, 0xac6d, 0xac94, 0xacba, 0xace0, 0xad06, 0xad2b, 0xad51,
    0xad76, 0xad9b, 0xadc0, 0xade4, 0xae0a, 0xae31, 0xae59, 0xae82, 0xaead,
    0xaed9, 0xaf07, 0xaf36, 0xaf68, 0xaf9c, 0xafd2, 0xb00b, 0xb047, 0xb086,
    0xb0c8, 0xb10d, 0xb156, 0xb1a1, 0xb1ef, 0xb23f, 0xb291, 0xb2e2, 0xb333,
    0xb384, 0xb3d4, 0xb423, 0xb470, 0xb4bb, 0xb504, 0xb553, 0xb5a6, 0xb5fd,
    0xb657, 0xb6b4, 0xb714, 0xb776, 0xb7da, 0xb83f, 0xb8a3, 0xb906, 0xb967,
    0xb9c4, 0xba1d, 0xba72, 0xbac4, 0xbb13, 0xbb5f, 0xbba9, 0xbbf1, 0xbc37,
    0xbc7d, 0xbcc1, 0xbd04, 0xbd47, 0xbd8b, 0xbdd1, 0xbe18, 0xbe60, 0xbea8,
    0xbef1, 0xbf3b, 0xbf85, 0xbfce, 0xc017, 0xc05e, 0xc0a4, 0xc0ec, 0xc13b,
    0xc18e, 0xc1e4, 0xc23b, 0xc292, 0xc2e7, 0xc33b, 0xc38b, 0xc3d8, 0xc424,
    0xc473, 0xc4c3, 0xc513, 0xc563, 0xc5b1, 0xc5fe, 0xc649, 0xc692, 0xc6d8,
    0xc71d, 0xc760, 0xc7a2, 0xc7e2, 0xc821, 0xc85f, 0xc89c, 0xc8d8, 0xc913,
    0xc94d, 0xc987, 0xc9bf, 0xc9f6, 0xca2d, 0xca63, 0xca9b, 0xcad7, 0xcb17,
    0xcb5b, 0xcba4, 0xcbf2, 0xcc48, 0xcca9, 0xcd1b, 0xcd8c, 0xcdfe, 0xce6f,
    0xcee1, 0xcf52, 0xcfbf, 0xd026, 0xd086, 0xd0e2, 0xd13a, 0xd18e, 0xd1de,
    0xd22c, 0xd277, 0xd2c0, 0xd307, 0xd34c, 0xd38f, 0xd3d0, 0xd410, 0xd44f,
    0xd48c, 0xd4c8, 0xd503, 0xd53d, 0xd576, 0xd5ae, 0xd5e5, 0xd61b, 0xd650,
    0xd685, 0xd6b9, 0xd6ec, 0xd71f, 0xd750, 0xd782, 0xd7b2, 0xd7e3, 0xd812,
    0xd843, 0xd876, 0xd8ab, 0xd8e4, 0xd920, 0xd960, 0xd9a5, 0xd9f1, 0xda44,
    0xdaa4, 0xdb15, 0xdb7f, 0xdbe3, 0xdc41, 0xdc9b, 0xdcf2, 0xdd46, 0xdd97,
    0xdde6, 0xde33, 0xde7e, 0xdec9, 0xdf14, 0xdf60, 0xdfab, 0xdff6, 0xe042,
    0xe08d, 0xe0d8, 0xe124, 0xe16f, 0xe1ba, 0xe205, 0xe251, 0xe29c, 0xe2e7,
    0xe330, 0xe377, 0xe3bc, 0xe3ff, 0xe441, 0xe481, 0xe4c0, 0xe4fd, 0xe539,
    0xe574, 0xe5ae, 0xe5e7, 0xe61f, 0xe656, 0xe68c, 0xe6c1, 0xe6f6, 0xe72a,
    0xe75d, 0xe790, 0xe7c2, 0xe7f3, 0xe824, 0xe854, 0xe884, 0xe8b3, 0xe8e1,
    0xe910, 0xe93d, 0xe96b, 0xe997, 0xe9c4, 0xe9f0, 0xea1c, 0xea47, 0xea72,
    0xea9c, 0xeac7, 0xeaf1, 0xeb1a, 0xeb45, 0xeb70, 0xeb9e, 0xebce, 0xebff,
    0xec33, 0xec6a, 0xeca4, 0xece1, 0xed23, 0xed6b, 0xedb9, 0xee12, 0xee75,
    0xeedc, 0xef43, 0xefab, 0xf012, 0xf079, 0xf0e0, 0xf148, 0xf1af, 0xf216,
    0xf27d, 0xf2e5, 0xf34c, 0xf3b3, 0xf41a, 0xf482, 0xf4e9, 0xf550, 0xf5b6,
    0xf616, 0xf671,
};

const uint16_t standing_start_apps_lookup[1001] = {
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03fb, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03d8, 0x034c, 0x02eb, 0x02a2, 0x0262, 0x023c, 0x022f, 0x022d,
    0x0236, 0x0254, 0x0278, 0x02a0, 0x02d3, 0x031f, 0x03a1, 0x011a, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0002, 0x006b, 0x008c,
    0x00c3, 0x013c, 0x01d8, 0x02d0, 0x03ff, 0x03ff, 0x03ff, 0x02ea, 0x021a,
    0x01ea, 0x0204, 0x0263, 0x02c9, 0x02ee, 0x02e6, 0x02f2, 0x0327, 0x0384,
    0x03de, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03fe, 0x03ff, 0x03ff, 0x03ff, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x004b, 0x010a, 0x01a3, 0x0222, 0x02a1, 0x02f4, 0x0323,
    0x0333, 0x033f, 0x032b, 0x0311, 0x02f1, 0x02fe, 0x03f3, 0x03ff, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0040, 0x0086, 0x01a6, 0x024e, 0x02f7, 0x0396, 0x03a5, 0x03c5, 0x03ea,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03fe, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03d5, 0x0386, 0x034a, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0064, 0x00b0, 0x012a, 0x01db,
    0x02fa, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03fe, 0x03ff, 0x03fe, 0x03f2, 0x03b1,
    0x037b, 0x0101, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x007a, 0x011e,
    0x02f7, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03fd, 0x0326,
    0x02f6, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0048, 0x0110, 0x0355, 0x03ff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0045, 0x00ba, 0x0232, 0x03ff, 0x03ff, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0060, 0x00d6, 0x0212, 0x03ff, 0x03ff, 0x0000, 0x0000, 0x0000, 0x0067,
    0x015a, 0x026c, 0x03ac, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x039e, 0x0363,
    0x035b, 0x033e, 0x02fd, 0x02f5, 0x02e6, 0x0305, 0x0387, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03f6, 0x034c, 0x02dc, 0x029f, 0x028c, 0x02ae, 0x0311,
    0x03b8, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03fe, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03d2, 0x0399, 0x0353, 0x0307, 0x02ca, 0x02a0, 0x0280,
    0x0273, 0x0274, 0x029c, 0x02ef, 0x0260, 0x017b, 0x017b, 0x017b, 0x017b,
    0x017b, 0x017b, 0x017b, 0x0000, 0x0000, 0x0117, 0x0195, 0x01b4, 0x0223,
    0x02ba, 0x0357, 0x03e2, 0x0257, 0x017b, 0x017b, 0x017b, 0x017b, 0x017b,
    0x017b, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x012d, 0x0220, 0x02c3, 0x032a, 0x034e,
    0x035f, 0x038c, 0x03bc, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03f9, 0x0000, 0x0000, 0x0000, 0x00e7, 0x0313,
    0x008a, 0x0000, 0x0000, 0x0000, 0x0000, 0x0127, 0x029a, 0x03ff, 0x03ff,
    0x0395, 0x02dc, 0x02bd, 0x02db, 0x0369, 0x03f7, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ed, 0x0343, 0x02de, 0x02b0, 0x029a,
    0x0294, 0x02f0, 0x03ff, 0x03ff, 0x01ce, 0x0000, 0x0000, 0x0000, 0x0000,
    0x00ae, 0x0175, 0x02e3, 0x03ff, 0x03ff, 0x0300, 0x0000, 0x0000, 0x0128,
    0x0230, 0x0361, 0x03ff, 0x03ff, 0x0391, 0x0235, 0x01c1, 0x01f7, 0x02d8,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x0368, 0x0311, 0x0334, 0x03a9, 0x03f1,
    0x03ff, 0x03ff, 0x03fe, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x0312, 0x017b, 0x017b, 0x017b,
    0x017b, 0x017b, 0x017b, 0x017b, 0x017b, 0x017b, 0x017b, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0077, 0x00cf, 0x0197, 0x0318,
    0x03ff, 0x03ff, 0x0000, 0x0000, 0x0000, 0x0087, 0x0106, 0x012e, 0x0143,
    0x015b, 0x019c, 0x01de, 0x01ff, 0x0217, 0x0266, 0x02f6, 0x03fa, 0x03ff,
    0x03ff, 0x02aa, 0x01a3, 0x00f7, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0002, 0x0060, 0x005a, 0x0079, 0x008c, 0x00ce, 0x014e,
    0x0233, 0x0387, 0x03ff, 0x03ff, 0x03c5, 0x02a0, 0x01fb, 0x01be, 0x019a,
    0x0176, 0x0162, 0x016f, 0x0169, 0x013b, 0x012c, 0x0170, 0x01e2, 0x021d,
    0x0248, 0x02ad, 0x0351, 0x03e2, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03fe, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x01c3, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0058, 0x008b, 0x00b2,
    0x011c, 0x0194, 0x0284, 0x03ff, 0x039f, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0064, 0x0124, 0x01e7, 0x02b8,
    0x03f0, 0x03ff, 0x03ff, 0x03ff, 0x03d7, 0x039d, 0x0399, 0x034f, 0x02d5,
    0x0269, 0x021a, 0x01f7, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x00a5, 0x01b2, 0x0358, 0x03ff, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0081, 0x0157, 0x0281, 0x03ff, 0x03ff, 0x0258, 0x0000,
    0x0000, 0x007d, 0x011a, 0x0190, 0x0216, 0x0302, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03fe, 0x03ad, 0x0339, 0x0309, 0x0305, 0x0329, 0x0364, 0x03ab,
    0x03fb, 0x03ff, 0x03ff, 0x03ff, 0x0391, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0024, 0x0024, 0x0024, 0x0024,
    0x0024, 0x0232, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x03ff, 0x03ff, 0x03ff, 0x03cf, 0x0394, 0x0363, 0x0331, 0x0305,
    0x02e0, 0x03ca, 0x0000, 0x004f, 0x004f, 0x004f, 0x004f, 0x004f, 0x004f,
    0x004f, 0x004f, 0x004f, 0x004f, 0x004f, 0x004f, 0x004f, 0x004f, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03fe, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x002b, 0x002b, 0x002b, 0x002b, 0x002b, 0x002b, 0x002b, 0x002b, 0x002b,
    0x002b, 0x002b, 0x002b, 0x002b, 0x002b, 0x002b, 0x002b, 0x0149, 0x03ff,
    0x03ff, 0x0037,
};

const uint16_t flying_lap_time_lookup[999] = {
    0x0000, 0x0057, 0x00aa, 0x00fa, 0x0148, 0x0192, 0x01db, 0x0221, 0x0266,
    0x02a9, 0x02ea, 0x032a, 0x0368, 0x03a5, 0x03e1, 0x041c, 0x0456, 0x048e,
    0x04c7, 0x04fe, 0x0535, 0x056b, 0x05a0, 0x05d4, 0x0608, 0x063b, 0x066e,
    0x069f, 0x06d1, 0x0701, 0x0732, 0x0762, 0x0792, 0x07c2, 0x07f2, 0x0822,
    0x0851, 0x0881, 0x08b0, 0x08e0, 0x090f, 0x093d, 0x096d, 0x099d, 0x09ce,
    0x0a01, 0x0a37, 0x0a6f, 0x0aa9, 0x0ae7, 0x0b26, 0x0b67, 0x0baa, 0x0bef,
    0x0c35, 0x0c7d, 0x0cc7, 0x0d12, 0x0d5e, 0x0daa, 0x0df7, 0x0e45, 0x0e92,
    0x0edf, 0x0f2b, 0x0f77, 0x0fc2, 0x100c, 0x1053, 0x1098, 0x10dc, 0x111e,
    0x115f, 0x11a0, 0x11e0, 0x1220, 0x125e, 0x129c, 0x12d9, 0x1315, 0x1350,
    0x138a, 0x13c3, 0x13fc, 0x1433, 0x1469, 0x149f, 0x14d4, 0x1508, 0x153c,
    0x156e, 0x15a1, 0x15d2, 0x1603, 0x1633, 0x1663, 0x1692, 0x16c1, 0x16f0,
    0x171d, 0x174b, 0x1778, 0x17a4, 0x17d1, 0x17fc, 0x1828, 0x1853, 0x187e,
    0x18ab, 0x18da, 0x190b, 0x193d, 0x1972, 0x19a9, 0x19e3, 0x1a20, 0x1a61,
    0x1aa5, 0x1aee, 0x1b3a, 0x1b8b, 0x1be0, 0x1c39, 0x1c97, 0x1cf8, 0x1d5a,
    0x1dbc, 0x1e20, 0x1e85, 0x1ee9, 0x1f4d, 0x1fae, 0x200d, 0x2068, 0x20c1,
    0x2116, 0x2169, 0x21ba, 0x2208, 0x2254, 0x229f, 0x22e8, 0x232e, 0x2373,
    0x23be, 0x240e, 0x2462, 0x24b8, 0x2510, 0x256a, 0x25c7, 0x2627, 0x268a,
    0x26ef, 0x2755, 0x27ba, 0x281d, 0x287d, 0x28da, 0x2933, 0x2988, 0x29da,
    0x2a29, 0x2a76, 0x2ac0, 0x2b08, 0x2b4d, 0x2b91, 0x2bd4, 0x2c14, 0x2c54,
    0x2c91, 0x2cce, 0x2d0a, 0x2d44, 0x2d7d, 0x2db6, 0x2ded, 0x2e24, 0x2e5a,
    0x2e8f, 0x2ec3, 0x2ef6, 0x2f29, 0x2f5b, 0x2f8d, 0x2fbe, 0x2fee, 0x301e,
    0x304e, 0x307d, 0x30ab, 0x30d9, 0x3106, 0x3133, 0x3160, 0x318c, 0x31b8,
    0x31e3, 0x320f, 0x3239, 0x3264, 0x328e, 0x32b8, 0x32e1, 0x330a, 0x3333,
    0x335d, 0x3387, 0x33b2, 0x33dd, 0x340a, 0x3438, 0x3466, 0x3495, 0x34c5,
    0x34f6, 0x3529, 0x355d, 0x3592, 0x35c9, 0x3604, 0x3642, 0x3683, 0x36c6,
    0x370d, 0x3755, 0x379e, 0x37e8, 0x3833, 0x387e, 0x38c9, 0x3914, 0x395e,
    0x39a8, 0x39ef, 0x3a35, 0x3a79, 0x3abb, 0x3afc, 0x3b3b, 0x3b79, 0x3bb5,
    0x3bf1, 0x3c2b, 0x3c64, 0x3c9d, 0x3cd4, 0x3d0b, 0x3d40, 0x3d75, 0x3daa,
    0x3ddd, 0x3e10, 0x3e42, 0x3e74, 0x3ea5, 0x3ed5, 0x3f05, 0x3f34, 0x3f63,
    0x3f92, 0x3fc0, 0x3fed, 0x401a, 0x4047, 0x4073, 0x409f, 0x40ca, 0x40f5,
    0x4120, 0x414a, 0x4175, 0x41a0, 0x41cc, 0x41fa, 0x4228, 0x4257, 0x4287,
    0x42b8, 0x42eb, 0x431e, 0x4353, 0x4389, 0x43c1, 0x43fa, 0x4436, 0x4474,
    0x44b7, 0x44fe, 0x4549, 0x4597, 0x45e7, 0x4639, 0x468b, 0x46df, 0x4732,
    0x4784, 0x47d4, 0x4821, 0x486c, 0x48b4, 0x48fa, 0x493f, 0x4981, 0x49c2,
    0x4a02, 0x4a41, 0x4a81, 0x4ac4, 0x4b09, 0x4b50, 0x4b99, 0x4be4, 0x4c31,
    0x4c7e, 0x4ccc, 0x4d19, 0x4d65, 0x4dae, 0x4dfb, 0x4e4d, 0x4ea3, 0x4efc,
    0x4f55, 0x4faf, 0x5008, 0x505f, 0x50b2, 0x5102, 0x5155, 0x51ac, 0x5206,
    0x5261, 0x52bc, 0x5317, 0x536f, 0x53c3, 0x5415, 0x5467, 0x54bd, 0x5515,
    0x556d, 0x55c3, 0x5618, 0x5669, 0x56b8, 0x5703, 0x574d, 0x5794, 0x57d9,
    0x581d, 0x5860, 0x58a1, 0x58e2, 0x5921, 0x595f, 0x599d, 0x59d9, 0x5a14,
    0x5a4e, 0x5a87, 0x5abf, 0x5af7, 0x5b2d, 0x5b63, 0x5b99, 0x5bce, 0x5c03,
    0x5c37, 0x5c6a, 0x5c9d, 0x5ccf, 0x5d01, 0x5d32, 0x5d62, 0x5d92, 0x5dc1,
    0x5df0, 0x5e1e, 0x5e4c, 0x5e7a, 0x5ea7, 0x5ed3, 0x5eff, 0x5f2b, 0x5f57,
    0x5f82, 0x5fac, 0x5fd7, 0x6001, 0x602b, 0x6054, 0x607d, 0x60a6, 0x60ce,
    0x60f7, 0x611f, 0x6147, 0x616e, 0x6195, 0x61bc, 0x61e3, 0x620a, 0x6230,
    0x6256, 0x627c, 0x62a2, 0x62c7, 0x62ed, 0x6312, 0x6337, 0x635c, 0x6381,
    0x63a6, 0x63ca, 0x63ef, 0x6414, 0x6438, 0x645d, 0x6481, 0x64a6, 0x64ca,
    0x64ef, 0x6513, 0x6538, 0x655c, 0x6581, 0x65a6, 0x65cb, 0x65f0, 0x6615,
    0x663a, 0x665e, 0x6683, 0x66a8, 0x66cc, 0x66f1, 0x6715, 0x673a, 0x675e,
    0x6782, 0x67a7, 0x67cc, 0x67f2, 0x6819, 0x6840, 0x6869, 0x6893, 0x68bf,
    0x68ec, 0x691a, 0x694b, 0x697d, 0x69b2, 0x69e9, 0x6a23, 0x6a60, 0x6aa1,
    0x6ae6, 0x6b30, 0x6b7e, 0x6bd0, 0x6c27, 0x6c82, 0x6ce3, 0x6d49, 0x6db5,
    0x6e27, 0x6e9d, 0x6f15, 0x6f91, 0x700f, 0x7089, 0x70fe, 0x716e, 0x71d8,
    0x723d, 0x729d, 0x72f9, 0x7351, 0x73a6, 0x73f7, 0x7445, 0x7490, 0x74da,
    0x7521, 0x7566, 0x75a9, 0x75eb, 0x762b, 0x766c, 0x76b0, 0x76f5, 0x7739,
    0x777c, 0x77bf, 0x7808, 0x7855, 0x78a5, 0x78f5, 0x7946, 0x7994, 0x79df,
    0x7a29, 0x7a70, 0x7ab6, 0x7afb, 0x7b3e, 0x7b81, 0x7bc1, 0x7c01, 0x7c3e,
    0x7c7b, 0x7cb6, 0x7cf1, 0x7d2a, 0x7d63, 0x7d9a, 0x7dd1, 0x7e07, 0x7e3d,
    0x7e72, 0x7ea7, 0x7edb, 0x7f0f, 0x7f42, 0x7f74, 0x7fa9, 0x7fdf, 0x8017,
    0x804e, 0x8086, 0x80bd, 0x80f4, 0x812a, 0x815f, 0x8194, 0x81c9, 0x81ff,
    0x8235, 0x826a, 0x829f, 0x82d3, 0x8306, 0x8339, 0x836b, 0x839d, 0x83cf,
    0x8401, 0x8432, 0x8462, 0x8492, 0x84c1, 0x84f0, 0x851f, 0x854d, 0x857b,
    0x85a8, 0x85d5, 0x8602, 0x862e, 0x865a, 0x8685, 0x86b0, 0x86db, 0x8705,
    0x872f, 0x8759, 0x8782, 0x87ab, 0x87d4, 0x87fd, 0x8825, 0x884d, 0x8875,
    0x889c, 0x88c3, 0x88ea, 0x8911, 0x8938, 0x895e, 0x8984, 0x89aa, 0x89d0,
    0x89f5, 0x8a1a, 0x8a3f, 0x8a64, 0x8a89, 0x8aae, 0x8ad2, 0x8af7, 0x8b1b,
    0x8b40, 0x8b64, 0x8b88, 0x8bad, 0x8bd1, 0x8bf6, 0x8c1a, 0x8c3f, 0x8c63,
    0x8c89, 0x8caf, 0x8cd6, 0x8cfd, 0x8d25, 0x8d4e, 0x8d78, 0x8da2, 0x8dcd,
    0x8df8, 0x8e24, 0x8e50, 0x8e7d, 0x8eab, 0x8ed9, 0x8f07, 0x8f37, 0x8f67,
    0x8f97, 0x8fc9, 0x8ffb, 0x902f, 0x9065, 0x909d, 0x90d8, 0x9117, 0x9157,
    0x919a, 0x91df, 0x9225, 0x926c, 0x92b4, 0x92fd, 0x9346, 0x938e, 0x93d6,
    0x941c, 0x9460, 0x94a3, 0x94e7, 0x952e, 0x9577, 0x95c0, 0x9609, 0x9651,
    0x9698, 0x96df, 0x9725, 0x976b, 0x97af, 0x97f3, 0x9836, 0x9877, 0x98b8,
    0x98f6, 0x9934, 0x9971, 0x99ad, 0x99e9, 0x9a25, 0x9a63, 0x9aa0, 0x9adf,
    0x9b1e, 0x9b5f, 0x9b9f, 0x9bdf, 0x9c20, 0x9c60, 0x9ca1, 0x9ce1, 0x9d22,
    0x9d62, 0x9da1, 0x9ddf, 0x9e1c, 0x9e58, 0x9e92, 0x9ecc, 0x9f06, 0x9f3f,
    0x9f78, 0x9fb1, 0x9fe9, 0xa021, 0xa059, 0xa091, 0xa0c9, 0xa100, 0xa137,
    0xa16e, 0xa1a4, 0xa1da, 0xa20f, 0xa244, 0xa277, 0xa2aa, 0xa2dd, 0xa30f,
    0xa340, 0xa370, 0xa3a0, 0xa3d0, 0xa3ff, 0xa42d, 0xa45b, 0xa489, 0xa4b6,
    0xa4e3, 0xa50f, 0xa53b, 0xa567, 0xa592, 0xa5bd, 0xa5e7, 0xa611, 0xa63b,
    0xa665, 0xa68e, 0xa6b7, 0xa6e0, 0xa708, 0xa730, 0xa758, 0xa780, 0xa7a7,
    0xa7ce, 0xa7f5, 0xa81b, 0xa842, 0xa868, 0xa88e, 0xa8b4, 0xa8d9, 0xa8ff,
    0xa924, 0xa949, 0xa96e, 0xa993, 0xa9b8, 0xa9df, 0xaa07, 0xaa30, 0xaa5b,
    0xaa87, 0xaab5, 0xaae5, 0xab16, 0xab4a, 0xab80, 0xabba, 0xabf6, 0xac34,
    0xac76, 0xacbc, 0xad04, 0xad4f, 0xad9d, 0xadee, 0xae3f, 0xae90, 0xaee1,
    0xaf32, 0xaf82, 0xafd1, 0xb01f, 0xb069, 0xb0b2, 0xb101, 0xb154, 0xb1ab,
    0xb205, 0xb262, 0xb2c2, 0xb325, 0xb389, 0xb3ed, 0xb452, 0xb4b5, 0xb515,
    0xb572, 0xb5cb, 0xb620, 0xb672, 0xb6c1, 0xb70d, 0xb757, 0xb79f, 0xb7e6,
    0xb82b, 0xb86f, 0xb8b2, 0xb8f5, 0xb939, 0xb97f, 0xb9c6, 0xba0e, 0xba56,
    0xba9f, 0xbae9, 0xbb33, 0xbb7d, 0xbbc5, 0xbc0d, 0xbc52, 0xbc9b, 0xbce9,
    0xbd3c, 0xbd92, 0xbde9, 0xbe40, 0xbe96, 0xbee9, 0xbf39, 0xbf87, 0xbfd3,
    0xc022, 0xc072, 0xc0c2, 0xc111, 0xc15f, 0xc1ac, 0xc1f7, 0xc240, 0xc287,
    0xc2cb, 0xc30e, 0xc350, 0xc390, 0xc3cf, 0xc40d, 0xc44a, 0xc486, 0xc4c1,
    0xc4fc, 0xc535, 0xc56d, 0xc5a4, 0xc5db, 0xc611, 0xc649, 0xc685, 0xc6c5,
    0xc709, 0xc752, 0xc7a0, 0xc7f7, 0xc857, 0xc8c9, 0xc93a, 0xc9ac, 0xca1d,
    0xca8f, 0xcb00, 0xcb6d, 0xcbd4, 0xcc35, 0xcc90, 0xcce8, 0xcd3c, 0xcd8d,
    0xcdda, 0xce26, 0xce6f, 0xceb5, 0xcefa, 0xcf3d, 0xcf7f, 0xcfbf, 0xcffd,
    0xd03a, 0xd076, 0xd0b1, 0xd0eb, 0xd124, 0xd15c, 0xd193, 0xd1c9, 0xd1ff,
    0xd233, 0xd267, 0xd29a, 0xd2cd, 0xd2ff, 0xd330, 0xd361, 0xd391, 0xd3c0,
    0xd3f1, 0xd424, 0xd45a, 0xd492, 0xd4ce, 0xd50e, 0xd553, 0xd59f, 0xd5f3,
    0xd652, 0xd6c4, 0xd72d, 0xd791, 0xd7ef, 0xd849, 0xd8a0, 0xd8f4, 0xd945,
    0xd994, 0xd9e1, 0xda2c, 0xda77, 0xdac2, 0xdb0e, 0xdb59, 0xdba4, 0xdbf0,
    0xdc3b, 0xdc86, 0xdcd2, 0xdd1d, 0xdd68, 0xddb4, 0xddff, 0xde4a, 0xde96,
    0xdedf, 0xdf26, 0xdf6a, 0xdfae, 0xdfef, 0xe02f, 0xe06e, 0xe0ab, 0xe0e7,
    0xe122, 0xe15c, 0xe195, 0xe1cd, 0xe204, 0xe23a, 0xe270, 0xe2a4, 0xe2d8,
    0xe30b, 0xe33e, 0xe370, 0xe3a1, 0xe3d2, 0xe402, 0xe432, 0xe461, 0xe490,
    0xe4be, 0xe4eb, 0xe519, 0xe546, 0xe572, 0xe59e, 0xe5ca, 0xe5f5, 0xe620,
    0xe64b, 0xe675, 0xe69f, 0xe6c8, 0xe6f3, 0xe71f, 0xe74c, 0xe77c, 0xe7ad,
    0xe7e1, 0xe818, 0xe852, 0xe890, 0xe8d2, 0xe919, 0xe968, 0xe9c0, 0xea23,
    0xea8a, 0xeaf2, 0xeb59, 0xebc0, 0xec27, 0xec8f, 0xecf6, 0xed5d, 0xedc4,
    0xee2c, 0xee93, 0xeefa, 0xef61, 0xefc9, 0xf030, 0xf097, 0xf0fe, 0xf164,
};

const uint16_t flying_lap_apps_lookup[999] = {
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03fe, 0x03f0, 0x03c8, 0x039b, 0x0370,
    0x034c, 0x0356, 0x0392, 0x03e1, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03fd, 0x0345, 0x0295, 0x0218, 0x01bf, 0x0177, 0x0154, 0x0150, 0x015a,
    0x0171, 0x0197, 0x01d6, 0x0209, 0x0255, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0002, 0x006b, 0x008c,
    0x00c3, 0x013c, 0x01d8, 0x02d0, 0x03ff, 0x03ff, 0x03ff, 0x02ea, 0x021a,
    0x01ea, 0x0204, 0x0263, 0x02c9, 0x02ee, 0x02e6, 0x02f2, 0x0327, 0x0384,
    0x03de, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03fe, 0x03ff, 0x03ff, 0x03ff, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x004b, 0x010a, 0x01a3, 0x0222, 0x02a1, 0x02f4, 0x0323,
    0x0333, 0x033f, 0x032b, 0x0311, 0x02f1, 0x02fe, 0x03f3, 0x03ff, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0040, 0x0086, 0x01a6, 0x024e, 0x02f7, 0x0396, 0x03a5, 0x03c5, 0x03ea,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03fe, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03d5, 0x0386, 0x034a, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0064, 0x00b0, 0x012a, 0x01db,
    0x02fa, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03fe, 0x03ff, 0x03fe, 0x03f2, 0x03b1,
    0x037b, 0x0101, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x007a, 0x011e,
    0x02f7, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03fd, 0x0326,
    0x02f6, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0048, 0x0110, 0x0355, 0x03ff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0045, 0x00ba, 0x0232, 0x03ff, 0x03ff, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0060, 0x00d6, 0x0212, 0x03ff, 0x03ff, 0x0000, 0x0000, 0x0000, 0x0067,
    0x015a, 0x026c, 0x03ac, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x039e, 0x0363,
    0x035b, 0x033e, 0x02fd, 0x02f5, 0x02e6, 0x0305, 0x0387, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03f6, 0x034c, 0x02dc, 0x029f, 0x028c, 0x02ae, 0x0311,
    0x03b8, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03fe, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03d2, 0x0399, 0x0353, 0x0307, 0x02ca, 0x02a0, 0x0280,
    0x0273, 0x0274, 0x029c, 0x02ef, 0x0260, 0x017b, 0x017b, 0x017b, 0x017b,
    0x017b, 0x017b, 0x017b, 0x0000, 0x0000, 0x0117, 0x0195, 0x01b4, 0x0223,
    0x02ba, 0x0357, 0x03e2, 0x0257, 0x017b, 0x017b, 0x017b, 0x017b, 0x017b,
    0x017b, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x012d, 0x0220, 0x02c3, 0x032a, 0x034e,
    0x035f, 0x038c, 0x03bc, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03f9, 0x0000, 0x0000, 0x0000, 0x00e7, 0x0313,
    0x008a, 0x0000, 0x0000, 0x0000, 0x0000, 0x0127, 0x029a, 0x03ff, 0x03ff,
    0x0395, 0x02dc, 0x02bd, 0x02db, 0x0369, 0x03f7, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ed, 0x0343, 0x02de, 0x02b0, 0x029a,
    0x0294, 0x02f0, 0x03ff, 0x03ff, 0x01ce, 0x0000, 0x0000, 0x0000, 0x0000,
    0x00ae, 0x0175, 0x02e3, 0x03ff, 0x03ff, 0x0300, 0x0000, 0x0000, 0x0128,
    0x0230, 0x0361, 0x03ff, 0x03ff, 0x0391, 0x0235, 0x01c1, 0x01f7, 0x02d8,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x0368, 0x0311, 0x0334, 0x03a9, 0x03f1,
    0x03ff, 0x03ff, 0x03fe, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x0312, 0x017b, 0x017b, 0x017b,
    0x017b, 0x017b, 0x017b, 0x017b, 0x017b, 0x017b, 0x017b, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0077, 0x00cf, 0x0197, 0x0318,
    0x03ff, 0x03ff, 0x0000, 0x0000, 0x0000, 0x0087, 0x0106, 0x012e, 0x0143,
    0x015b, 0x019c, 0x01de, 0x01ff, 0x0217, 0x0266, 0x02f6, 0x03fa, 0x03ff,
    0x03ff, 0x02aa, 0x01a3, 0x00f7, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0002, 0x0060, 0x005a, 0x0079, 0x008c, 0x00ce, 0x014e,
    0x0233, 0x0387, 0x03ff, 0x03ff, 0x03c5, 0x02a0, 0x01fb, 0x01be, 0x019a,
    0x0176, 0x0162, 0x016f, 0x0169, 0x013b, 0x012c, 0x0170, 0x01e2, 0x021d,
    0x0248, 0x02ad, 0x0351, 0x03e2, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03fe, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x01c3, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0058, 0x008b, 0x00b2,
    0x011c, 0x0194, 0x0284, 0x03ff, 0x039f, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0064, 0x0124, 0x01e7, 0x02b8,
    0x03f0, 0x03ff, 0x03ff, 0x03ff, 0x03d7, 0x039d, 0x0399, 0x034f, 0x02d5,
    0x0269, 0x021a, 0x01f7, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x00a5, 0x01b2, 0x0358, 0x03ff, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0081, 0x0157, 0x0281, 0x03ff, 0x03ff, 0x0258, 0x0000,
    0x0000, 0x007d, 0x011a, 0x0190, 0x0216, 0x0302, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03fe, 0x03ad, 0x0339, 0x0309, 0x0305, 0x0329, 0x0364, 0x03ab,
    0x03fb, 0x03ff, 0x03ff, 0x03ff, 0x0391, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0024, 0x0024, 0x0024, 0x0024,
    0x0024, 0x0232, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x03ff, 0x03ff, 0x03ff, 0x03cf, 0x0394, 0x0363, 0x0331, 0x0305,
    0x02e0, 0x03ca, 0x0000, 0x004f, 0x004f, 0x004f, 0x004f, 0x004f, 0x004f,
    0x004f, 0x004f, 0x004f, 0x004f, 0x004f, 0x004f, 0x004f, 0x004f, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x03ff, 0x03fe, 0x03ff, 0x03ff, 0x03ff, 0x03ff,
    0x03ff, 0x03ff, 0x03ff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x002b, 0x002b, 0x002b, 0x002b, 0x002b, 0x002b, 0x002b, 0x002b, 0x002b,
    0x002b, 0x002b, 0x002b, 0x002b, 0x002b, 0x002b, 0x002b, 0x0149, 0x03ff,
};

#endif