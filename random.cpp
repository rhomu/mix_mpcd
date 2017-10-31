// random numbers

#include <random>
#include "random.hpp"
#include "ziggurat_inline.hpp"

using namespace std;

// return random real, uniform distribution
float random_real()
{
  return r4_uni_value();
}

// return random real, normally distributed
float random_normal()
{
  return r4_nor_value();
}

void init_random()
{
  random_device rd;
  zigset(rd(), rd(), rd(), rd());
}
