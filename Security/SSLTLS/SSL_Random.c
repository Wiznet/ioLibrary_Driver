/*
 * file: SSL_Random.h
 * description: random generator function
 * author: peter
 * company: wiznet
 * data: 2015.11.26
 */

#include "SSL_Random.h"

void RandomSeeding(int randSeed)
{
	srand(randSeed);
}


int SSLRandomCB( void *p_rng, unsigned char *output, size_t output_len )
{
    int i;

	if(output_len <= 0)
	{
         return (1);
	}
    for(i = 0;i < output_len;i++)
    {
        *output++ = rand() % 0xff;
	}
    srand(rand());
	return (0);
}
