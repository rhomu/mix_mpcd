#ifndef RANDOM_HPP_
#define RANDOM_HPP_

#include "header.hpp"
#include <algorithm>
#include <random>

using generator_t = std::mt19937;

// return random real, uniform distribution
double random_real(double min, double max);

// return random vector, with uniform distribution
vec random_vec(double min, double max);

// return the generator
generator_t& get_gen();

// return random real, arbitrary distribution
template<class T>
double random_real(const T& dist)
{
  return dist(get_gen());
}

// return random vector, with arbitrary distribution
template<class T>
vec random_vec(T&& dist)
{
  vec v;
  std::generate(std::begin(v),
                std::end(v),
                [&](){ return dist(get_gen()); }
               );
  return v;
}

#endif//RANDOM_HPP_
