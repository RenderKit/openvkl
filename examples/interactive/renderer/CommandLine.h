// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <list>
#include <sstream>
#include <string>
#include <tuple>

namespace openvkl {
  namespace examples {
    namespace cmd {

      inline void consume_0(std::list<std::string> &list,
                            std::list<std::string>::iterator &it)
      {
        it = list.erase(it);
      }

      template <class T>
      inline T consume_1(std::list<std::string> &list,
                         std::list<std::string>::iterator &it)
      {
        T result;

        std::list<std::string>::iterator oit = it;

        ++oit;
        if (oit == list.end())
          throw std::runtime_error(*it + ": missing arguments");

        std::istringstream is(*oit);
        if (!(is >> result))
          throw std::runtime_error(*it + ": invalid argument " + *oit);

        // Note: We do not change the list until we know that the
        // arguments are valid.
        it = list.erase(it);
        it = list.erase(it);

        return result;
      }

      template <class T1, class T2>
      inline std::tuple<T1, T2> consume_2(
          std::list<std::string> &list, std::list<std::string>::iterator &it)
      {
        std::tuple<T1, T2> result;
        std::list<std::string>::iterator oit = it;

        {
          ++oit;
          if (oit == list.end())
            throw std::runtime_error(*it + ": missing arguments");

          std::istringstream is(*oit);
          if (!(is >> std::get<0>(result)))
            throw std::runtime_error(*it + ": invalid argument " + *oit);
        }
        {
          ++oit;
          if (oit == list.end())
            throw std::runtime_error(*it + ": missing arguments");

          std::istringstream is(*oit);
          if (!(is >> std::get<1>(result)))
            throw std::runtime_error(*it + ": invalid argument " + *oit);
        }

        // Note: We do not change the list until we know that the
        // arguments are valid.
        it = list.erase(it);
        it = list.erase(it);
        it = list.erase(it);

        return result;
      }

      template <class T1, class T2, class T3>
      inline std::tuple<T1, T2, T3> consume_3(
          std::list<std::string> &list, std::list<std::string>::iterator &it)
      {
        std::tuple<T1, T2, T3> result;
        std::list<std::string>::iterator oit = it;

        {
          ++oit;
          if (oit == list.end())
            throw std::runtime_error(*it + ": missing arguments");

          std::istringstream is(*oit);
          if (!(is >> std::get<0>(result)))
            throw std::runtime_error(*it + ": invalid argument " + *oit);
        }
        {
          ++oit;
          if (oit == list.end())
            throw std::runtime_error(*it + ": missing arguments");

          std::istringstream is(*oit);
          if (!(is >> std::get<1>(result)))
            throw std::runtime_error(*it + ": invalid argument " + *oit);
        }
        {
          ++oit;
          if (oit == list.end())
            throw std::runtime_error(*it + ": missing arguments");

          std::istringstream is(*oit);
          if (!(is >> std::get<2>(result)))
            throw std::runtime_error(*it + ": invalid argument " + *oit);
        }

        // Note: We do not change the list until we know that the
        // arguments are valid.
        it = list.erase(it);
        it = list.erase(it);
        it = list.erase(it);
        it = list.erase(it);

        return result;
      }


      template <class T1, class T2, class T3, class T4>
      inline std::tuple<T1, T2, T3, T4> consume_4(
          std::list<std::string> &list, std::list<std::string>::iterator &it)
      {
        std::tuple<T1, T2, T3, T4> result;
        std::list<std::string>::iterator oit = it;

        {
          ++oit;
          if (oit == list.end())
            throw std::runtime_error(*it + ": missing arguments");

          std::istringstream is(*oit);
          if (!(is >> std::get<0>(result)))
            throw std::runtime_error(*it + ": invalid argument " + *oit);
        }
        {
          ++oit;
          if (oit == list.end())
            throw std::runtime_error(*it + ": missing arguments");

          std::istringstream is(*oit);
          if (!(is >> std::get<1>(result)))
            throw std::runtime_error(*it + ": invalid argument " + *oit);
        }
        {
          ++oit;
          if (oit == list.end())
            throw std::runtime_error(*it + ": missing arguments");

          std::istringstream is(*oit);
          if (!(is >> std::get<2>(result)))
            throw std::runtime_error(*it + ": invalid argument " + *oit);
        }
        {
          ++oit;
          if (oit == list.end())
            throw std::runtime_error(*it + ": missing arguments");

          std::istringstream is(*oit);
          if (!(is >> std::get<3>(result)))
            throw std::runtime_error(*it + ": invalid argument " + *oit);
        }

        // Note: We do not change the list until we know that the
        // arguments are valid.
        it = list.erase(it);
        it = list.erase(it);
        it = list.erase(it);
        it = list.erase(it);
        it = list.erase(it);

        return result;
      }


    }  // namespace cmd
  }    // namespace examples
}  // namespace openvkl

