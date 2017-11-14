#ifndef RANDOM_HPP_
#define RANDOM_HPP_

// self explanatory
void init_random();
// return random real, uniform distribution
float random_real();
// return random real, normally distributed
float random_normal();
// return random uint
uint32_t random_uint32();

inline float random_real(float min, float max)
{
  return min+random_real()*(max-min);
}

inline float random_normal(float mu, float sigma)
{
  return mu + sigma*random_normal();
}

// generate unisgned in range [lower, upper)
inline uint32_t random_uint32(uint32_t lower, uint32_t upper)
{
  // this is ok when the range is small compared to UINT_MAX!!!
  return ((random_uint32() % (upper-lower+1)) + lower);
}

#endif//RANDOM_HPP_
