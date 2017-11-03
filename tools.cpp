//
// some useful tools
//

#include <iostream>
#include <iomanip>
#include "header.hpp"
#include "tools.hpp"

// shortcut to boost program_options
namespace opt = boost::program_options;

void print_vm(const opt::variables_map& vm, unsigned padding)
{
  for (opt::variables_map::const_iterator it = vm.begin(); it != vm.end(); ++it)
  {
    std::cout << std::left << " " << std::setw(padding-39) << it->first;
    //if (((boost::any)it->second.value()).empty()) {
    //  std::cout << "(empty)";
    //}
    //if (vm[it->first].defaulted() || it->second.defaulted()) {
    //  std::cout << "(default)";
    //}
    std::cout << " = " << std::right << std::setw(35);

    bool is_char;
    try {
      boost::any_cast<const char *>(it->second.value());
      is_char = true;
    } catch (const boost::bad_any_cast &) {
      is_char = false;
    }
    bool is_str;
    try {
      boost::any_cast<std::string>(it->second.value());
      is_str = true;
    } catch (const boost::bad_any_cast &) {
      is_str = false;
    }

    if (((boost::any)it->second.value()).type() == typeid(int)) {
      std::cout << vm[it->first].as<int>() << std::endl;
    } else if (((boost::any)it->second.value()).type() == typeid(unsigned)) {
      std::cout << vm[it->first].as<unsigned>() << std::endl;
    } else if (((boost::any)it->second.value()).type() == typeid(size_t)) {
      std::cout << vm[it->first].as<size_t>() << std::endl;
    } else if (((boost::any)it->second.value()).type() == typeid(bool)) {
      std::cout << vm[it->first].as<bool>() << std::endl;
    } else if (((boost::any)it->second.value()).type() == typeid(double)) {
      std::cout << vm[it->first].as<double>() << std::endl;
    } else if (((boost::any)it->second.value()).type() == typeid(float)) {
      std::cout << vm[it->first].as<float>() << std::endl;
    } else if (is_char) {
      std::cout << vm[it->first].as<const char *>() << std::endl;
    } else if (is_str) {
      std::string temp = vm[it->first].as<std::string>();
      if (temp.size()) {
        std::cout << temp << std::endl;
      } else {
        std::cout << "true" << std::endl;
      }
    } else { // Assumes that the only remainder is vector<string>
      try {
        std::vector<std::string> vect = vm[it->first].as<std::vector<std::string> >();
        uint i = 0;
        for (std::vector<std::string>::iterator oit=vect.begin();
            oit != vect.end(); oit++, ++i) {
          std::cout << "\r> " << it->first << "[" << i << "]=" << (*oit) << std::endl;
        }
      } catch (const boost::bad_any_cast &) {
        std::cout << "UnknownType(" << ((boost::any)it->second.value()).type().name() << ")" << std::endl;
      }
    }
  }
}
