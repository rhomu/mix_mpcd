#ifndef RANDOM_HPP_
#define RANDOM_HPP_

// self explanatory
void init_random();
// return random real, uniform distribution
float random_real();
// return random real, normally distributed
float random_normal();

inline float random_real(float min, float max)
{
  return min+random_real()*(max-min);
}

inline float random_normal(float mu, float sigma)
{
  return mu + sigma*random_normal();
}

#endif//RANDOM_HPP_
