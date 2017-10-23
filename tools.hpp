// tools.hpp
// some utility functions

#ifndef TOOLS_HPP_
#define TOOLS_HPP_

#include <iostream>
#include <boost/program_options.hpp>
#include "vector.hpp"

/** Modulo function that works correctly with negative values */
double modu(double num, double div);

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

inline std::vector<double> get_doubles_from_string(std::string s)
{
  std::vector<double> values;

  std::replace(begin(s), end(s), ']', ',');
  std::replace(begin(s), end(s), '[', ' ');

  for(int p=0, q=0; p!=s.npos; p=q)
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

  for(int p=0, q=0; p!=s.npos; p=q)
  {
    const auto ss = s.substr(p+(p!=0), (q=s.find(',', p+1))-p-(p!=0));
    if(not ss.empty()) values.push_back(atoi(ss.c_str()));
  }

  return values;
}

// write single value to binary stream
template<class T>
inline std::ostream& write_binary(std::ostream& stream, T* value)
{
  return stream.write((char*)value, sizeof(T));
}

// modulo with correct handling of negative values
inline double modu(double num, double div)
{
  return div*signbit(num) + std::fmod(num, div);
}

// puts vector back on [0,1]^dim
template<class T, int D>
inline vect<T, D> collapse(vect<T, D> v)
{
  for(auto& c : v) c = modu(c, 1.);
  return v;
}

#endif//TOOLS_HPP_
