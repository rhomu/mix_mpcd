#ifndef VECTOR_HPP_
#define VECTOR_HPP_

#include <iostream>
#include <array>

// simple vector
template<class T, int D>
class vect
{
  // internal array of components
  std::array<T, D> components;
  // assert that D>0
  static_assert(D>0, "zero-length vector not permitted");

public:
  vect()
  {}
  vect(T init_val)
  { components.fill(init_val); }
  vect(const std::array<T, D>& components_)
    : components(components_)
  {}
  vect(const vect& p)
    : components(p.components)
  {}

  // return iterator on components
  typename std::array<T, D>::iterator begin()
  { return components.begin(); }
  typename std::array<T, D>::const_iterator begin() const
  { return components.begin(); }
  // return iterator on components
  typename std::array<T, D>::iterator end()
  { return components.end(); }
  typename std::array<T, D>::const_iterator end() const
  { return components.end(); }

  // addition
  vect& operator+=(const vect& p)
  {
    for(size_t i=0; i<D; ++i)
      components[i] += p.components[i];
    return *this;
  }
  vect operator+(const vect& p) const
  {
    auto r = *this;
    return (r+=p);
  }

  // return components
  std::array<double, 2> get_components() const
  { return components; }

  // subtraction
  vect operator-=(const vect& p)
  {
    for(size_t i=0; i<D; ++i)
      components[i] -= p.components[i];
    return *this;
  }
  vect operator-(const vect& p) const
  {
    auto r = *this;
    return (r-=p);
  }
  vect operator-() const
  {
    vect r;
    for(size_t i=0; i<D; ++i)
      r.components[i] = -components[i];
    return r;
  }
  // element wise product
  vect operator*(const vect& p) const
  {
    vect c;
    for(size_t i=0; i<D; ++i)
      c[i] = components[i]*p.components[i];
    return c;
  }
  // vector product
  T times(const vect& p) const
  {
    T c;
    for(size_t i=0; i<D; ++i)
      c += components[i]*p.components[i];
    return c;
  }

  // division by scalar
  vect& operator/=(const T& s)
  {
    for(auto& c: components)
      c /= s;
    return *this;
  }
  // division by scalar
  vect operator/(const T& s)
  {
    auto r = *this;
    return (r /= s);
  }

  T& operator[](size_t i)
  { return components[i]; }
  T operator[](size_t i) const
  { return components[i]; }

  // norm squared
  T sq() const
  { return this->times(*this); }
  // norm
  T norm() const
  { return sqrt(sq()); }
  // normalize
  vect& normalize()
  {
    return *this/=norm();
  }

  template<class TT, int DD>
  friend vect<TT, DD> operator*(const vect<TT, DD>&, const TT&);
  template<class TT, int DD>
  friend vect<TT, DD> operator*(const TT&, const vect<TT, DD>&);
  template<class TT, int DD>
  friend std::ostream& operator<<(std::ostream&, const vect<TT, DD>&);

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar & auto_name(components);
  }
};

template<class T, int D>
vect<T, D> operator*(const vect<T, D>& p, const T& s)
{
  auto r = p;
  for(size_t i=0; i<D; ++i)
    r.components[i] *= s;
  return r;
}

template<class T, int D>
vect<T, D> operator*(const T& s, const vect<T, D>& p)
{
  auto r = p;
  for(size_t i=0; i<D; ++i)
    r.components[i] *= s;
  return r;
}

template<class T, int D>
std::ostream& operator<<(std::ostream& str, const vect<T, D>& p)
{
  str << "(" << p.components[0];
  for(size_t i=1; i<D; ++i)
    str << "," << p.components[i];
  str << ")";
  return str;
}

#endif//VECTOR_HPP_
