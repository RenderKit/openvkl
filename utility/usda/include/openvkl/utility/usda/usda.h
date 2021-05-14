// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <initializer_list>
#include <sstream>
#include <string>
#include <vector>

namespace openvkl {
  namespace utility {
    namespace usda {

      struct StreamOp
      {
        virtual ~StreamOp() = default;
        std::string str;
      };

    }  // namespace usda
  }    // namespace utility
}  // namespace openvkl

inline std::ostream &operator<<(std::ostream &os,
                                const openvkl::utility::usda::StreamOp &op)
{
  return os << op.str;
}

namespace openvkl {
  namespace utility {
    namespace usda {

      struct Scope
      {
        static int &getIndent()
        {
          static int s_indent{0};
          return s_indent;
        }

        struct Indent : public StreamOp
        {
          Indent()
          {
            for (int i = 0; i < Scope::getIndent(); ++i) {
              str += " ";
            }
          }
        };

        Scope(std::ostream &os,
              const std::string &type,
              const std::string &name)
            : os(os)
        {
          os << Indent() << "def " << type << " \"" << name << "\" {\n";
          ++getIndent();
        }

        ~Scope()
        {
          --getIndent();
          os << Indent() << "}\n";
        }
        std::ostream &os;
      };

      struct Header
      {
        Header(std::ostream &os) : os(os)
        {
          os << "#usda 1.0\n(\n";
          ++Scope::getIndent();
        }
        ~Header()
        {
          --Scope::getIndent();
          os << ")\n\n";
        }
        std::ostream &os;
      };

      struct Attribute
      {
        template <class Value>
        Attribute(std::ostream &os,
                  const std::string &type,
                  const std::string &name,
                  Value &&value)
        {
          os << Scope::Indent() << type << " " << name << " = " << value
             << "\n";
        }

        template <class Value>
        Attribute(std::ostream &os, const std::string &name, Value &&value)
        {
          os << Scope::Indent() << name << " = " << value << "\n";
        }
      };

      struct String : public StreamOp
      {
        template <class Value>
        explicit String(Value &&value)
        {
          std::ostringstream os;
          os << "\"" << value << "\"";
          str = os.str();
        }
      };

      template <class Ctr>
      std::string join(const Ctr &ctr)
      {
        std::ostringstream os;
        auto it        = std::begin(ctr);
        const auto end = std::end(ctr);
        if (it != end) {
          os << *it;
          while (++it != end) {
            os << ", " << *it;
          }
        }
        return os.str();
      }

      template <class T>
      struct List : public StreamOp
      {
        template <class Ctr = std::initializer_list<T>>
        explicit List(const Ctr &ctr)
        {
          str = ('[' + join(ctr)) + ']';
        }
      };

      template <class T>
      struct Tuple : public StreamOp
      {
        template <class Ctr = std::initializer_list<T>>
        explicit Tuple(const Ctr &ctr)
        {
          str = ('(' + join(ctr)) + ')';
        }
      };

    }  // namespace usda
  }    // namespace utility
}  // namespace openvkl
