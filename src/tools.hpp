// tools.hpp
// some utility functions

#ifndef TOOLS_HPP_
#define TOOLS_HPP_

#include <iostream>
#include <cmath>
#include <boost/program_options.hpp>
#include "vector.hpp"

/** Print variables from variables_map
  *
  * from: https://gist.github.com/gesquive/8673796
  */
void print_vm(const boost::program_options::variables_map& vm, unsigned padding);

namespace detail
{
  /** Convert to strig and catenate arguments */
  template<class Head>
  void inline_str_add_args(std::ostream& stream, Head&& head)
  {
    stream << std::forward<Head>(head);
  }
  /** Convert to strig and catenate arguments */
  template<class Head, class... Tail>
  void inline_str_add_args(std::ostream& stream, Head&& head, Tail&&... tail)
  {
    stream << std::forward<Head>(head);
    inline_str_add_args(stream, std::forward<Tail>(tail)...);
  }
} // namespace detail

/** Convert any number of arguments to string and catenate
 *
 * It does pretty much what is advertised. Look at the code if you want to learn
 * some pretty neat modern C++.
 * */
template<class... Args>
std::string inline_str(Args&&... args)
{
  std::stringstream s;
  detail::inline_str_add_args(s, std::forward<Args>(args)...);
  return s.str();
}

inline std::vector<float> get_floats_from_string(std::string s)
{
  std::vector<float> values;

  std::replace(begin(s), end(s), ']', ',');
  std::replace(begin(s), end(s), '[', ' ');

  for(size_t p=0, q=0; p!=s.npos; p=q)
  {
    const auto ss = s.substr(p+(p!=0), (q=s.find(',', p+1))-p-(p!=0));
    if(not ss.empty()) values.push_back(atof(ss.c_str()));
  }

  return values;
}

inline std::vector<int> get_ints_from_string(std::string s)
{
  std::vector<int> values;

  std::replace(begin(s), end(s), ']', ',');
  std::replace(begin(s), end(s), '[', ' ');

  for(size_t p=0, q=0; p!=s.npos; p=q)
  {
    const auto ss = s.substr(p+(p!=0), (q=s.find(',', p+1))-p-(p!=0));
    if(not ss.empty()) values.push_back(atoi(ss.c_str()));
  }

  return values;
}

// write single value to binary stream
template<class T>
inline std::ostream& write_binary(std::ostream& stream, const T& value)
{
  return stream.write((char*)&value, sizeof(T));
}

// modulo with correct handling of negative values
inline float modu(float num, float div)
{
  return div*std::signbit(num) + std::fmod(num, div);
}

// component-wise modu
template<int D, typename U>
inline vect<float, D> modu(const vect<float, D>& num, const U& div)
{
  vect<float, D> ret;
  for(int i=0; i<D; ++i) ret[i] = modu(num[i], div[i]);
  return ret;
}

/* Branchless division accounting for zero
 *
 * This function is the branchless version of
 *
 *    if(norm==0) value /= 1;
 *    else value /= norm;
 *
 * and is useful when norm==0 implies value==0 (think about normalizing the
 * average of an empty array by the number of its elements).
 * */
template<typename T, typename U>
inline void normalize(T& value, U norm)
{
  value /= norm += norm == 0;
}

#endif//TOOLS_HPP_
