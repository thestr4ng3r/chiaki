/*
 * This file is part of Chiaki.
 *
 * Chiaki is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Chiaki is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Chiaki.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <chiaki/rpcrypt.h>



CHIAKI_EXPORT void chiaki_rpcrypt_bright_ambassador(uint8_t *bright, uint8_t *ambassador, const uint8_t *nonce, const uint8_t *morning)
{
	static const uint8_t echo_a[] = { 0x01, 0x49, 0x87, 0x9b, 0x65, 0x39, 0x8b, 0x39, 0x4b, 0x3a, 0x8d, 0x48, 0xc3, 0x0a, 0xef, 0x51 };
	static const uint8_t echo_b[] = { 0xe1, 0xec, 0x9c, 0x3a, 0xdd, 0xbd, 0x08, 0x85, 0xfc, 0x0e, 0x1d, 0x78, 0x90, 0x32, 0xc0, 0x04 };

	for(uint8_t i=0; i<CHIAKI_KEY_BYTES; i++)
	{
		uint8_t v = nonce[i];
		v -= i;
		v -= 0x27;
		v ^= echo_a[i];
		ambassador[i] = v;
	}

	for(uint8_t i=0; i<CHIAKI_KEY_BYTES; i++)
	{
		uint8_t v = morning[i];
		v -= i;
		v += 0x34;
		v ^= echo_b[i];
		v ^= nonce[i];
		bright[i] = v;
	}
}