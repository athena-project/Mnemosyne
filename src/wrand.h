//#include <stdint>
#include <iostream>
//https://en.wikipedia.org/wiki/Linear_congruential_generator
//Newlib, Musl

/**
 * x0 is the seed, one step rand
 * modulo done because 64bits >= 0
 * Xn = a * Xn-1 + b mod 2^64
 */
inline uint64_t wrand( uint64_t x0, uint64_t n){
	uint64_t a = 6364136223846793005;
	uint64_t b = 1; 
	
	return a * ((a * x0 + b) ^ n ) + b; //Using Name-Based Mappings to Increase Hit Rates, David G. Thaler and Chinya V. Ravishankar
}
