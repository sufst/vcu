/***************************************************************************
 * @file   driver_profile_data.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2022-02-13
 * @brief  Raw data for driver profiles
 ***************************************************************************/

#include "driver_profiles.h"
#include <stdint.h>

/**
 * @brief Default linear throttle curve with a small deadzone
 */
const uint16_t default_linear_throttle_map [THROTTLE_MAP_SIZE] = {
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0021, 0x0043, 0x0065, 0x0086, 0x00a8, 0x00ca, 0x00eb, 0x010d, 0x012f, 0x0151, 0x0172, 0x0194,
	0x01b6, 0x01d7, 0x01f9, 0x021b, 0x023d, 0x025e, 0x0280, 0x02a2, 0x02c3, 0x02e5, 0x0307, 0x0329, 0x034a, 0x036c, 0x038e, 0x03af,
	0x03d1, 0x03f3, 0x0415, 0x0436, 0x0458, 0x047a, 0x049b, 0x04bd, 0x04df, 0x0501, 0x0522, 0x0544, 0x0566, 0x0587, 0x05a9, 0x05cb,
	0x05ec, 0x060e, 0x0630, 0x0652, 0x0673, 0x0695, 0x06b7, 0x06d8, 0x06fa, 0x071c, 0x073e, 0x075f, 0x0781, 0x07a3, 0x07c4, 0x07e6,
	0x0808, 0x082a, 0x084b, 0x086d, 0x088f, 0x08b0, 0x08d2, 0x08f4, 0x0916, 0x0937, 0x0959, 0x097b, 0x099c, 0x09be, 0x09e0, 0x0a02,
	0x0a23, 0x0a45, 0x0a67, 0x0a88, 0x0aaa, 0x0acc, 0x0aee, 0x0b0f, 0x0b31, 0x0b53, 0x0b74, 0x0b96, 0x0bb8, 0x0bd9, 0x0bfb, 0x0c1d,
	0x0c3f, 0x0c60, 0x0c82, 0x0ca4, 0x0cc5, 0x0ce7, 0x0d09, 0x0d2b, 0x0d4c, 0x0d6e, 0x0d90, 0x0db1, 0x0dd3, 0x0df5, 0x0e17, 0x0e38,
	0x0e5a, 0x0e7c, 0x0e9d, 0x0ebf, 0x0ee1, 0x0f03, 0x0f24, 0x0f46, 0x0f68, 0x0f89, 0x0fab, 0x0fcd, 0x0fef, 0x1010, 0x1032, 0x1054,
	0x1075, 0x1097, 0x10b9, 0x10da, 0x10fc, 0x111e, 0x1140, 0x1161, 0x1183, 0x11a5, 0x11c6, 0x11e8, 0x120a, 0x122c, 0x124d, 0x126f,
	0x1291, 0x12b2, 0x12d4, 0x12f6, 0x1318, 0x1339, 0x135b, 0x137d, 0x139e, 0x13c0, 0x13e2, 0x1404, 0x1425, 0x1447, 0x1469, 0x148a,
	0x14ac, 0x14ce, 0x14f0, 0x1511, 0x1533, 0x1555, 0x1576, 0x1598, 0x15ba, 0x15dc, 0x15fd, 0x161f, 0x1641, 0x1662, 0x1684, 0x16a6,
	0x16c7, 0x16e9, 0x170b, 0x172d, 0x174e, 0x1770, 0x1792, 0x17b3, 0x17d5, 0x17f7, 0x1819, 0x183a, 0x185c, 0x187e, 0x189f, 0x18c1,
	0x18e3, 0x1905, 0x1926, 0x1948, 0x196a, 0x198b, 0x19ad, 0x19cf, 0x19f1, 0x1a12, 0x1a34, 0x1a56, 0x1a77, 0x1a99, 0x1abb, 0x1add,
	0x1afe, 0x1b20, 0x1b42, 0x1b63, 0x1b85, 0x1ba7, 0x1bc9, 0x1bea, 0x1c0c, 0x1c2e, 0x1c4f, 0x1c71, 0x1c93, 0x1cb4, 0x1cd6, 0x1cf8,
	0x1d1a, 0x1d3b, 0x1d5d, 0x1d7f, 0x1da0, 0x1dc2, 0x1de4, 0x1e06, 0x1e27, 0x1e49, 0x1e6b, 0x1e8c, 0x1eae, 0x1ed0, 0x1ef2, 0x1f13,
	0x1f35, 0x1f57, 0x1f78, 0x1f9a, 0x1fbc, 0x1fde, 0x1fff, 0x2021, 0x2043, 0x2064, 0x2086, 0x20a8, 0x20ca, 0x20eb, 0x210d, 0x212f,
	0x2150, 0x2172, 0x2194, 0x21b5, 0x21d7, 0x21f9, 0x221b, 0x223c, 0x225e, 0x2280, 0x22a1, 0x22c3, 0x22e5, 0x2307, 0x2328, 0x234a,
	0x236c, 0x238d, 0x23af, 0x23d1, 0x23f3, 0x2414, 0x2436, 0x2458, 0x2479, 0x249b, 0x24bd, 0x24df, 0x2500, 0x2522, 0x2544, 0x2565,
	0x2587, 0x25a9, 0x25cb, 0x25ec, 0x260e, 0x2630, 0x2651, 0x2673, 0x2695, 0x26b7, 0x26d8, 0x26fa, 0x271c, 0x273d, 0x275f, 0x2781,
	0x27a2, 0x27c4, 0x27e6, 0x2808, 0x2829, 0x284b, 0x286d, 0x288e, 0x28b0, 0x28d2, 0x28f4, 0x2915, 0x2937, 0x2959, 0x297a, 0x299c,
	0x29be, 0x29e0, 0x2a01, 0x2a23, 0x2a45, 0x2a66, 0x2a88, 0x2aaa, 0x2acc, 0x2aed, 0x2b0f, 0x2b31, 0x2b52, 0x2b74, 0x2b96, 0x2bb8,
	0x2bd9, 0x2bfb, 0x2c1d, 0x2c3e, 0x2c60, 0x2c82, 0x2ca3, 0x2cc5, 0x2ce7, 0x2d09, 0x2d2a, 0x2d4c, 0x2d6e, 0x2d8f, 0x2db1, 0x2dd3,
	0x2df5, 0x2e16, 0x2e38, 0x2e5a, 0x2e7b, 0x2e9d, 0x2ebf, 0x2ee1, 0x2f02, 0x2f24, 0x2f46, 0x2f67, 0x2f89, 0x2fab, 0x2fcd, 0x2fee,
	0x3010, 0x3032, 0x3053, 0x3075, 0x3097, 0x30b9, 0x30da, 0x30fc, 0x311e, 0x313f, 0x3161, 0x3183, 0x31a5, 0x31c6, 0x31e8, 0x320a,
	0x322b, 0x324d, 0x326f, 0x3290, 0x32b2, 0x32d4, 0x32f6, 0x3317, 0x3339, 0x335b, 0x337c, 0x339e, 0x33c0, 0x33e2, 0x3403, 0x3425,
	0x3447, 0x3468, 0x348a, 0x34ac, 0x34ce, 0x34ef, 0x3511, 0x3533, 0x3554, 0x3576, 0x3598, 0x35ba, 0x35db, 0x35fd, 0x361f, 0x3640,
	0x3662, 0x3684, 0x36a6, 0x36c7, 0x36e9, 0x370b, 0x372c, 0x374e, 0x3770, 0x3792, 0x37b3, 0x37d5, 0x37f7, 0x3818, 0x383a, 0x385c,
	0x387d, 0x389f, 0x38c1, 0x38e3, 0x3904, 0x3926, 0x3948, 0x3969, 0x398b, 0x39ad, 0x39cf, 0x39f0, 0x3a12, 0x3a34, 0x3a55, 0x3a77,
	0x3a99, 0x3abb, 0x3adc, 0x3afe, 0x3b20, 0x3b41, 0x3b63, 0x3b85, 0x3ba7, 0x3bc8, 0x3bea, 0x3c0c, 0x3c2d, 0x3c4f, 0x3c71, 0x3c93,
	0x3cb4, 0x3cd6, 0x3cf8, 0x3d19, 0x3d3b, 0x3d5d, 0x3d7e, 0x3da0, 0x3dc2, 0x3de4, 0x3e05, 0x3e27, 0x3e49, 0x3e6a, 0x3e8c, 0x3eae,
	0x3ed0, 0x3ef1, 0x3f13, 0x3f35, 0x3f56, 0x3f78, 0x3f9a, 0x3fbc, 0x3fdd, 0x3fff, 0x4021, 0x4042, 0x4064, 0x4086, 0x40a8, 0x40c9,
	0x40eb, 0x410d, 0x412e, 0x4150, 0x4172, 0x4194, 0x41b5, 0x41d7, 0x41f9, 0x421a, 0x423c, 0x425e, 0x4280, 0x42a1, 0x42c3, 0x42e5,
	0x4306, 0x4328, 0x434a, 0x436b, 0x438d, 0x43af, 0x43d1, 0x43f2, 0x4414, 0x4436, 0x4457, 0x4479, 0x449b, 0x44bd, 0x44de, 0x4500,
	0x4522, 0x4543, 0x4565, 0x4587, 0x45a9, 0x45ca, 0x45ec, 0x460e, 0x462f, 0x4651, 0x4673, 0x4695, 0x46b6, 0x46d8, 0x46fa, 0x471b,
	0x473d, 0x475f, 0x4781, 0x47a2, 0x47c4, 0x47e6, 0x4807, 0x4829, 0x484b, 0x486c, 0x488e, 0x48b0, 0x48d2, 0x48f3, 0x4915, 0x4937,
	0x4958, 0x497a, 0x499c, 0x49be, 0x49df, 0x4a01, 0x4a23, 0x4a44, 0x4a66, 0x4a88, 0x4aaa, 0x4acb, 0x4aed, 0x4b0f, 0x4b30, 0x4b52,
	0x4b74, 0x4b96, 0x4bb7, 0x4bd9, 0x4bfb, 0x4c1c, 0x4c3e, 0x4c60, 0x4c82, 0x4ca3, 0x4cc5, 0x4ce7, 0x4d08, 0x4d2a, 0x4d4c, 0x4d6e,
	0x4d8f, 0x4db1, 0x4dd3, 0x4df4, 0x4e16, 0x4e38, 0x4e59, 0x4e7b, 0x4e9d, 0x4ebf, 0x4ee0, 0x4f02, 0x4f24, 0x4f45, 0x4f67, 0x4f89,
	0x4fab, 0x4fcc, 0x4fee, 0x5010, 0x5031, 0x5053, 0x5075, 0x5097, 0x50b8, 0x50da, 0x50fc, 0x511d, 0x513f, 0x5161, 0x5183, 0x51a4,
	0x51c6, 0x51e8, 0x5209, 0x522b, 0x524d, 0x526f, 0x5290, 0x52b2, 0x52d4, 0x52f5, 0x5317, 0x5339, 0x535b, 0x537c, 0x539e, 0x53c0,
	0x53e1, 0x5403, 0x5425, 0x5446, 0x5468, 0x548a, 0x54ac, 0x54cd, 0x54ef, 0x5511, 0x5532, 0x5554, 0x5576, 0x5598, 0x55b9, 0x55db,
	0x55fd, 0x561e, 0x5640, 0x5662, 0x5684, 0x56a5, 0x56c7, 0x56e9, 0x570a, 0x572c, 0x574e, 0x5770, 0x5791, 0x57b3, 0x57d5, 0x57f6,
	0x5818, 0x583a, 0x585c, 0x587d, 0x589f, 0x58c1, 0x58e2, 0x5904, 0x5926, 0x5947, 0x5969, 0x598b, 0x59ad, 0x59ce, 0x59f0, 0x5a12,
	0x5a33, 0x5a55, 0x5a77, 0x5a99, 0x5aba, 0x5adc, 0x5afe, 0x5b1f, 0x5b41, 0x5b63, 0x5b85, 0x5ba6, 0x5bc8, 0x5bea, 0x5c0b, 0x5c2d,
	0x5c4f, 0x5c71, 0x5c92, 0x5cb4, 0x5cd6, 0x5cf7, 0x5d19, 0x5d3b, 0x5d5d, 0x5d7e, 0x5da0, 0x5dc2, 0x5de3, 0x5e05, 0x5e27, 0x5e49,
	0x5e6a, 0x5e8c, 0x5eae, 0x5ecf, 0x5ef1, 0x5f13, 0x5f34, 0x5f56, 0x5f78, 0x5f9a, 0x5fbb, 0x5fdd, 0x5fff, 0x6020, 0x6042, 0x6064,
	0x6086, 0x60a7, 0x60c9, 0x60eb, 0x610c, 0x612e, 0x6150, 0x6172, 0x6193, 0x61b5, 0x61d7, 0x61f8, 0x621a, 0x623c, 0x625e, 0x627f,
	0x62a1, 0x62c3, 0x62e4, 0x6306, 0x6328, 0x634a, 0x636b, 0x638d, 0x63af, 0x63d0, 0x63f2, 0x6414, 0x6436, 0x6457, 0x6479, 0x649b,
	0x64bc, 0x64de, 0x6500, 0x6521, 0x6543, 0x6565, 0x6587, 0x65a8, 0x65ca, 0x65ec, 0x660d, 0x662f, 0x6651, 0x6673, 0x6694, 0x66b6,
	0x66d8, 0x66f9, 0x671b, 0x673d, 0x675f, 0x6780, 0x67a2, 0x67c4, 0x67e5, 0x6807, 0x6829, 0x684b, 0x686c, 0x688e, 0x68b0, 0x68d1,
	0x68f3, 0x6915, 0x6937, 0x6958, 0x697a, 0x699c, 0x69bd, 0x69df, 0x6a01, 0x6a22, 0x6a44, 0x6a66, 0x6a88, 0x6aa9, 0x6acb, 0x6aed,
	0x6b0e, 0x6b30, 0x6b52, 0x6b74, 0x6b95, 0x6bb7, 0x6bd9, 0x6bfa, 0x6c1c, 0x6c3e, 0x6c60, 0x6c81, 0x6ca3, 0x6cc5, 0x6ce6, 0x6d08,
	0x6d2a, 0x6d4c, 0x6d6d, 0x6d8f, 0x6db1, 0x6dd2, 0x6df4, 0x6e16, 0x6e38, 0x6e59, 0x6e7b, 0x6e9d, 0x6ebe, 0x6ee0, 0x6f02, 0x6f24,
	0x6f45, 0x6f67, 0x6f89, 0x6faa, 0x6fcc, 0x6fee, 0x700f, 0x7031, 0x7053, 0x7075, 0x7096, 0x70b8, 0x70da, 0x70fb, 0x711d, 0x713f,
	0x7161, 0x7182, 0x71a4, 0x71c6, 0x71e7, 0x7209, 0x722b, 0x724d, 0x726e, 0x7290, 0x72b2, 0x72d3, 0x72f5, 0x7317, 0x7339, 0x735a,
	0x737c, 0x739e, 0x73bf, 0x73e1, 0x7403, 0x7425, 0x7446, 0x7468, 0x748a, 0x74ab, 0x74cd, 0x74ef, 0x7510, 0x7532, 0x7554, 0x7576,
	0x7597, 0x75b9, 0x75db, 0x75fc, 0x761e, 0x7640, 0x7662, 0x7683, 0x76a5, 0x76c7, 0x76e8, 0x770a, 0x772c, 0x774e, 0x776f, 0x7791,
	0x77b3, 0x77d4, 0x77f6, 0x7818, 0x783a, 0x785b, 0x787d, 0x789f, 0x78c0, 0x78e2, 0x7904, 0x7926, 0x7947, 0x7969, 0x798b, 0x79ac,
	0x79ce, 0x79f0, 0x7a12, 0x7a33, 0x7a55, 0x7a77, 0x7a98, 0x7aba, 0x7adc, 0x7afd, 0x7b1f, 0x7b41, 0x7b63, 0x7b84, 0x7ba6, 0x7bc8,
	0x7be9, 0x7c0b, 0x7c2d, 0x7c4f, 0x7c70, 0x7c92, 0x7cb4, 0x7cd5, 0x7cf7, 0x7d19, 0x7d3b, 0x7d5c, 0x7d7e, 0x7da0, 0x7dc1, 0x7de3,
	0x7e05, 0x7e27, 0x7e48, 0x7e6a, 0x7e8c, 0x7ead, 0x7ecf, 0x7ef1, 0x7f13, 0x7f34, 0x7f56, 0x7f78, 0x7f99, 0x7fbb, 0x7fdd, 0x7fdd,
};

/**
 * @brief Linear five percent limited throttle map with a small deadzone
 */
const uint16_t linear_five_percent_limited_throttle_map [THROTTLE_MAP_SIZE] = {
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
	0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0x0003, 0x0005, 0x0006, 0x0008, 0x000a, 0x000b, 0x000d, 0x000f, 0x0010, 0x0012, 0x0014, 
	0x0015, 0x0017, 0x0019, 0x001b, 0x001c, 0x001e, 0x0020, 0x0021, 0x0023, 0x0025, 0x0026, 0x0028, 0x002a, 0x002b, 0x002d, 0x002f, 
	0x0030, 0x0032, 0x0034, 0x0036, 0x0037, 0x0039, 0x003b, 0x003c, 0x003e, 0x0040, 0x0041, 0x0043, 0x0045, 0x0046, 0x0048, 0x004a, 
	0x004b, 0x004d, 0x004f, 0x0051, 0x0052, 0x0054, 0x0056, 0x0057, 0x0059, 0x005b, 0x005c, 0x005e, 0x0060, 0x0061, 0x0063, 0x0065, 
	0x0067, 0x0068, 0x006a, 0x006c, 0x006d, 0x006f, 0x0071, 0x0072, 0x0074, 0x0076, 0x0077, 0x0079, 0x007b, 0x007c, 0x007e, 0x0080, 
	0x0082, 0x0083, 0x0085, 0x0087, 0x0088, 0x008a, 0x008c, 0x008d, 0x008f, 0x0091, 0x0092, 0x0094, 0x0096, 0x0097, 0x0099, 0x009b, 
	0x009d, 0x009e, 0x00a0, 0x00a2, 0x00a3, 0x00a5, 0x00a7, 0x00a8, 0x00aa, 0x00ac, 0x00ad, 0x00af, 0x00b1, 0x00b2, 0x00b4, 0x00b6, 
	0x00b8, 0x00b9, 0x00bb, 0x00bd, 0x00be, 0x00c0, 0x00c2, 0x00c3, 0x00c5, 0x00c7, 0x00c8, 0x00ca, 0x00cc, 0x00ce, 0x00cf, 0x00d1, 
	0x00d3, 0x00d4, 0x00d6, 0x00d8, 0x00d9, 0x00db, 0x00dd, 0x00de, 0x00e0, 0x00e2, 0x00e3, 0x00e5, 0x00e7, 0x00e9, 0x00ea, 0x00ec, 
	0x00ee, 0x00ef, 0x00f1, 0x00f3, 0x00f4, 0x00f6, 0x00f8, 0x00f9, 0x00fb, 0x00fd, 0x00fe, 0x0100, 0x0102, 0x0104, 0x0105, 0x0107, 
	0x0109, 0x010a, 0x010c, 0x010e, 0x010f, 0x0111, 0x0113, 0x0114, 0x0116, 0x0118, 0x011a, 0x011b, 0x011d, 0x011f, 0x0120, 0x0122, 
	0x0124, 0x0125, 0x0127, 0x0129, 0x012a, 0x012c, 0x012e, 0x012f, 0x0131, 0x0133, 0x0135, 0x0136, 0x0138, 0x013a, 0x013b, 0x013d, 
	0x013f, 0x0140, 0x0142, 0x0144, 0x0145, 0x0147, 0x0149, 0x014a, 0x014c, 0x014e, 0x0150, 0x0151, 0x0153, 0x0155, 0x0156, 0x0158, 
	0x015a, 0x015b, 0x015d, 0x015f, 0x0160, 0x0162, 0x0164, 0x0165, 0x0167, 0x0169, 0x016b, 0x016c, 0x016e, 0x0170, 0x0171, 0x0173, 
	0x0175, 0x0176, 0x0178, 0x017a, 0x017b, 0x017d, 0x017f, 0x0181, 0x0182, 0x0184, 0x0186, 0x0187, 0x0189, 0x018b, 0x018c, 0x018e, 
	0x0190, 0x0191, 0x0193, 0x0195, 0x0196, 0x0198, 0x019a, 0x019c, 0x019d, 0x019f, 0x01a1, 0x01a2, 0x01a4, 0x01a6, 0x01a7, 0x01a9, 
	0x01ab, 0x01ac, 0x01ae, 0x01b0, 0x01b1, 0x01b3, 0x01b5, 0x01b7, 0x01b8, 0x01ba, 0x01bc, 0x01bd, 0x01bf, 0x01c1, 0x01c2, 0x01c4, 
	0x01c6, 0x01c7, 0x01c9, 0x01cb, 0x01cd, 0x01ce, 0x01d0, 0x01d2, 0x01d3, 0x01d5, 0x01d7, 0x01d8, 0x01da, 0x01dc, 0x01dd, 0x01df, 
	0x01e1, 0x01e2, 0x01e4, 0x01e6, 0x01e8, 0x01e9, 0x01eb, 0x01ed, 0x01ee, 0x01f0, 0x01f2, 0x01f3, 0x01f5, 0x01f7, 0x01f8, 0x01fa, 
	0x01fc, 0x01fd, 0x01ff, 0x0201, 0x0203, 0x0204, 0x0206, 0x0208, 0x0209, 0x020b, 0x020d, 0x020e, 0x0210, 0x0212, 0x0213, 0x0215, 
	0x0217, 0x0218, 0x021a, 0x021c, 0x021e, 0x021f, 0x0221, 0x0223, 0x0224, 0x0226, 0x0228, 0x0229, 0x022b, 0x022d, 0x022e, 0x0230, 
	0x0232, 0x0234, 0x0235, 0x0237, 0x0239, 0x023a, 0x023c, 0x023e, 0x023f, 0x0241, 0x0243, 0x0244, 0x0246, 0x0248, 0x0249, 0x024b, 
	0x024d, 0x024f, 0x0250, 0x0252, 0x0254, 0x0255, 0x0257, 0x0259, 0x025a, 0x025c, 0x025e, 0x025f, 0x0261, 0x0263, 0x0264, 0x0266, 
	0x0268, 0x026a, 0x026b, 0x026d, 0x026f, 0x0270, 0x0272, 0x0274, 0x0275, 0x0277, 0x0279, 0x027a, 0x027c, 0x027e, 0x0280, 0x0281, 
	0x0283, 0x0285, 0x0286, 0x0288, 0x028a, 0x028b, 0x028d, 0x028f, 0x0290, 0x0292, 0x0294, 0x0295, 0x0297, 0x0299, 0x029b, 0x029c, 
	0x029e, 0x02a0, 0x02a1, 0x02a3, 0x02a5, 0x02a6, 0x02a8, 0x02aa, 0x02ab, 0x02ad, 0x02af, 0x02b0, 0x02b2, 0x02b4, 0x02b6, 0x02b7, 
	0x02b9, 0x02bb, 0x02bc, 0x02be, 0x02c0, 0x02c1, 0x02c3, 0x02c5, 0x02c6, 0x02c8, 0x02ca, 0x02cb, 0x02cd, 0x02cf, 0x02d1, 0x02d2, 
	0x02d4, 0x02d6, 0x02d7, 0x02d9, 0x02db, 0x02dc, 0x02de, 0x02e0, 0x02e1, 0x02e3, 0x02e5, 0x02e7, 0x02e8, 0x02ea, 0x02ec, 0x02ed, 
	0x02ef, 0x02f1, 0x02f2, 0x02f4, 0x02f6, 0x02f7, 0x02f9, 0x02fb, 0x02fc, 0x02fe, 0x0300, 0x0302, 0x0303, 0x0305, 0x0307, 0x0308, 
	0x030a, 0x030c, 0x030d, 0x030f, 0x0311, 0x0312, 0x0314, 0x0316, 0x0317, 0x0319, 0x031b, 0x031d, 0x031e, 0x0320, 0x0322, 0x0323, 
	0x0325, 0x0327, 0x0328, 0x032a, 0x032c, 0x032d, 0x032f, 0x0331, 0x0333, 0x0334, 0x0336, 0x0338, 0x0339, 0x033b, 0x033d, 0x033e, 
	0x0340, 0x0342, 0x0343, 0x0345, 0x0347, 0x0348, 0x034a, 0x034c, 0x034e, 0x034f, 0x0351, 0x0353, 0x0354, 0x0356, 0x0358, 0x0359, 
	0x035b, 0x035d, 0x035e, 0x0360, 0x0362, 0x0363, 0x0365, 0x0367, 0x0369, 0x036a, 0x036c, 0x036e, 0x036f, 0x0371, 0x0373, 0x0374, 
	0x0376, 0x0378, 0x0379, 0x037b, 0x037d, 0x037e, 0x0380, 0x0382, 0x0384, 0x0385, 0x0387, 0x0389, 0x038a, 0x038c, 0x038e, 0x038f, 
	0x0391, 0x0393, 0x0394, 0x0396, 0x0398, 0x039a, 0x039b, 0x039d, 0x039f, 0x03a0, 0x03a2, 0x03a4, 0x03a5, 0x03a7, 0x03a9, 0x03aa, 
	0x03ac, 0x03ae, 0x03af, 0x03b1, 0x03b3, 0x03b5, 0x03b6, 0x03b8, 0x03ba, 0x03bb, 0x03bd, 0x03bf, 0x03c0, 0x03c2, 0x03c4, 0x03c5, 
	0x03c7, 0x03c9, 0x03ca, 0x03cc, 0x03ce, 0x03d0, 0x03d1, 0x03d3, 0x03d5, 0x03d6, 0x03d8, 0x03da, 0x03db, 0x03dd, 0x03df, 0x03e0, 
	0x03e2, 0x03e4, 0x03e5, 0x03e7, 0x03e9, 0x03eb, 0x03ec, 0x03ee, 0x03f0, 0x03f1, 0x03f3, 0x03f5, 0x03f6, 0x03f8, 0x03fa, 0x03fb, 
	0x03fd, 0x03ff, 0x0401, 0x0402, 0x0404, 0x0406, 0x0407, 0x0409, 0x040b, 0x040c, 0x040e, 0x0410, 0x0411, 0x0413, 0x0415, 0x0416, 
	0x0418, 0x041a, 0x041c, 0x041d, 0x041f, 0x0421, 0x0422, 0x0424, 0x0426, 0x0427, 0x0429, 0x042b, 0x042c, 0x042e, 0x0430, 0x0431, 
	0x0433, 0x0435, 0x0437, 0x0438, 0x043a, 0x043c, 0x043d, 0x043f, 0x0441, 0x0442, 0x0444, 0x0446, 0x0447, 0x0449, 0x044b, 0x044d, 
	0x044e, 0x0450, 0x0452, 0x0453, 0x0455, 0x0457, 0x0458, 0x045a, 0x045c, 0x045d, 0x045f, 0x0461, 0x0462, 0x0464, 0x0466, 0x0468, 
	0x0469, 0x046b, 0x046d, 0x046e, 0x0470, 0x0472, 0x0473, 0x0475, 0x0477, 0x0478, 0x047a, 0x047c, 0x047d, 0x047f, 0x0481, 0x0483, 
	0x0484, 0x0486, 0x0488, 0x0489, 0x048b, 0x048d, 0x048e, 0x0490, 0x0492, 0x0493, 0x0495, 0x0497, 0x0498, 0x049a, 0x049c, 0x049e, 
	0x049f, 0x04a1, 0x04a3, 0x04a4, 0x04a6, 0x04a8, 0x04a9, 0x04ab, 0x04ad, 0x04ae, 0x04b0, 0x04b2, 0x04b4, 0x04b5, 0x04b7, 0x04b9, 
	0x04ba, 0x04bc, 0x04be, 0x04bf, 0x04c1, 0x04c3, 0x04c4, 0x04c6, 0x04c8, 0x04c9, 0x04cb, 0x04cd, 0x04cf, 0x04d0, 0x04d2, 0x04d4, 
	0x04d5, 0x04d7, 0x04d9, 0x04da, 0x04dc, 0x04de, 0x04df, 0x04e1, 0x04e3, 0x04e4, 0x04e6, 0x04e8, 0x04ea, 0x04eb, 0x04ed, 0x04ef, 
	0x04f0, 0x04f2, 0x04f4, 0x04f5, 0x04f7, 0x04f9, 0x04fa, 0x04fc, 0x04fe, 0x0500, 0x0501, 0x0503, 0x0505, 0x0506, 0x0508, 0x050a, 
	0x050b, 0x050d, 0x050f, 0x0510, 0x0512, 0x0514, 0x0515, 0x0517, 0x0519, 0x051b, 0x051c, 0x051e, 0x0520, 0x0521, 0x0523, 0x0525, 
	0x0526, 0x0528, 0x052a, 0x052b, 0x052d, 0x052f, 0x0530, 0x0532, 0x0534, 0x0536, 0x0537, 0x0539, 0x053b, 0x053c, 0x053e, 0x0540, 
	0x0541, 0x0543, 0x0545, 0x0546, 0x0548, 0x054a, 0x054b, 0x054d, 0x054f, 0x0551, 0x0552, 0x0554, 0x0556, 0x0557, 0x0559, 0x055b, 
	0x055c, 0x055e, 0x0560, 0x0561, 0x0563, 0x0565, 0x0567, 0x0568, 0x056a, 0x056c, 0x056d, 0x056f, 0x0571, 0x0572, 0x0574, 0x0576, 
	0x0577, 0x0579, 0x057b, 0x057c, 0x057e, 0x0580, 0x0582, 0x0583, 0x0585, 0x0587, 0x0588, 0x058a, 0x058c, 0x058d, 0x058f, 0x0591, 
	0x0592, 0x0594, 0x0596, 0x0597, 0x0599, 0x059b, 0x059d, 0x059e, 0x05a0, 0x05a2, 0x05a3, 0x05a5, 0x05a7, 0x05a8, 0x05aa, 0x05ac, 
	0x05ad, 0x05af, 0x05b1, 0x05b3, 0x05b4, 0x05b6, 0x05b8, 0x05b9, 0x05bb, 0x05bd, 0x05be, 0x05c0, 0x05c2, 0x05c3, 0x05c5, 0x05c7, 
	0x05c8, 0x05ca, 0x05cc, 0x05ce, 0x05cf, 0x05d1, 0x05d3, 0x05d4, 0x05d6, 0x05d8, 0x05d9, 0x05db, 0x05dd, 0x05de, 0x05e0, 0x05e2, 
	0x05e3, 0x05e5, 0x05e7, 0x05e9, 0x05ea, 0x05ec, 0x05ee, 0x05ef, 0x05f1, 0x05f3, 0x05f4, 0x05f6, 0x05f8, 0x05f9, 0x05fb, 0x05fd, 
	0x05fe, 0x0600, 0x0602, 0x0604, 0x0605, 0x0607, 0x0609, 0x060a, 0x060c, 0x060e, 0x060f, 0x0611, 0x0613, 0x0614, 0x0616, 0x0618, 
	0x061a, 0x061b, 0x061d, 0x061f, 0x0620, 0x0622, 0x0624, 0x0625, 0x0627, 0x0629, 0x062a, 0x062c, 0x062e, 0x062f, 0x0631, 0x0633, 
	0x0635, 0x0636, 0x0638, 0x063a, 0x063b, 0x063d, 0x063f, 0x0640, 0x0642, 0x0644, 0x0645, 0x0647, 0x0649, 0x064a, 0x064c, 0x064e, 
	0x0650, 0x0651, 0x0653, 0x0655, 0x0656, 0x0658, 0x065a, 0x065b, 0x065d, 0x065f, 0x0660, 0x0662, 0x0664, 0x0666, 0x0666, 0x0666, 
};