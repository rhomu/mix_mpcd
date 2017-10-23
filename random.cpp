// random numbers

#include "random.hpp"

using namespace std;

// truly random device to generate seed
random_device rd;
// pseudo random generator
generator_t gen(rd());
// uniform distribution on (0, 1)
uniform_real_distribution<> dist01(0, 1);
// uniform distribution on (0, 1)
uniform_real_distribution<> dist11(-1, 1);

// return random real, uniform distribution
double random_real(double min, double max)
{
  return uniform_real_distribution<>(min, max)(gen);
}

// return random real, gaussian distributed
double random_normal(double sigma)
{
  return normal_distribution<>(0., sigma)(gen);
}

// return random vector, with uniform distribution
vec random_vec(double min, double max)
{
  vec v;
  auto dist = uniform_real_distribution<>(min, max);
  generate(begin(v), end(v), [&](){ return dist(gen); } );
  return v;
}

generator_t& get_gen()
{
  return gen;
}
