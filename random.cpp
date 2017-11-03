// random numbers

#include <random>
#include "random.hpp"
#include "ziggurat_inline.hpp"

using namespace std;

// size of the random numbers tables
constexpr int table_size = 16777216;
// the tables
vector<float> normal_values(table_size);
vector<float> unifor_values(table_size);

// return random real, uniform distribution
float random_real()
{
  //return r4_uni_value();

  static int i = -1;
  if(++i==table_size) i=0;
  return unifor_values[i];
}

// return random real, normally distributed
float random_normal()
{
  //return r4_nor_value();

  static int i = -1;
  if(++i==table_size) i=0;
  return normal_values[i];
}

void init_random()
{
  // init
  random_device rd;
  zigset(rd(), rd(), rd(), rd());

  // populate tables
  for(long int i=0; i<table_size; ++i)
  {
    normal_values[i] = r4_nor_value();
    unifor_values[i] = r4_uni_value();
  }
}
