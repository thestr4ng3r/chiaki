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

#include <chiaki/fec.h>

#include <jerasure.h>
#include <cauchy.h>

#include <string.h>
#include <stdlib.h>

int *create_matrix(unsigned int k, unsigned int m)
{
	return cauchy_original_coding_matrix(k, m, CHIAKI_FEC_WORDSIZE);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_fec_decode(uint8_t *frame_buf, size_t unit_size, size_t stride, unsigned int k, unsigned int m, const unsigned int *erasures, size_t erasures_count)
{
	if(stride < unit_size)
		return CHIAKI_ERR_INVALID_DATA;
	int *matrix = create_matrix(k, m);
	if(!matrix)
		return CHIAKI_ERR_MEMORY;

	ChiakiErrorCode err = CHIAKI_ERR_SUCCESS;

	int *jerasures = calloc(erasures_count + 1, sizeof(int));
	if(!jerasures)
	{
		err = CHIAKI_ERR_MEMORY;
		goto error_matrix;
	}
	memcpy(jerasures, erasures, erasures_count * sizeof(int));
	jerasures[erasures_count] = -1;

	uint8_t **data_ptrs = calloc(k, sizeof(uint8_t *));
	if(!data_ptrs)
	{
		err = CHIAKI_ERR_MEMORY;
		goto error_jerasures;
	}

	uint8_t **coding_ptrs = calloc(m, sizeof(uint8_t *));
	if(!coding_ptrs)
	{
		err = CHIAKI_ERR_MEMORY;
		goto error_data_ptrs;
	}

	for(size_t i=0; i<k+m; i++)
	{
		uint8_t *buf_ptr = frame_buf + stride * i;
		if(i < k)
			data_ptrs[i] = buf_ptr;
		else
			coding_ptrs[i - k] = buf_ptr;
	}

	int res = jerasure_matrix_decode(k, m, CHIAKI_FEC_WORDSIZE, matrix, 0, jerasures,
									 (char **)data_ptrs, (char **)coding_ptrs, unit_size);

	if(res < 0)
		err = CHIAKI_ERR_FEC_FAILED;
	else
		err = CHIAKI_ERR_SUCCESS;

	free(coding_ptrs);
error_data_ptrs:
	free(data_ptrs);
error_jerasures:
	free(jerasures);
error_matrix:
	free(matrix);
	return err;
}
