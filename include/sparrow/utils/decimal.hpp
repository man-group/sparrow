#pragma once


// mpl
#include "sparrow/utils/mp_utils.hpp"
#include "sparrow/utils/large_int.hpp"
#include <string_view>
#include <sstream>
#include <iostream>

// pow
#include <cmath>


namespace sparrow 
{




    template<class T>
    class decimal
    {
    public:
        using integer_type = T;
    
        decimal()
            : m_value(0)
            , m_scale(0)
        {
        }

        decimal(T value, int scale)
            : m_value(value)
            , m_scale(scale)
        {
        }


        bool operator==(const decimal& other) const 
        {
            return m_value == other.m_value && m_scale == other.m_scale;
        }

        // operator !=
        bool operator!=(const decimal& other) const
        {
            return !(*this == other);
        }

        explicit operator float() const requires(!is_int_placeholder_v<T>)
        {
            return convert_to_floating_point<float>();
        }
        explicit operator double() const requires(!is_int_placeholder_v<T>)
        {
            return convert_to_floating_point<double>();
        }
        explicit operator long double() const requires(!is_int_placeholder_v<T>)
        {
            return convert_to_floating_point<long double>();
        }

        // convert to string
        explicit operator std::string() const requires(!is_int_placeholder_v<T>)
        {  
            std::stringstream ss;
            ss << m_value;
            std::string result = ss.str();
            if( m_scale == 0 )
            {
                return result;
            }
            if(result[0] == '0')
            {
                return "0";
            }
            // remove -  (we handle it later)
            if(result[0] == '-')
            {
                result = result.substr(1);
            }
    
            if (m_scale > 0)
            {
                if (result.length() <= static_cast<std::size_t>(m_scale)) {
                    result.insert(0, std::string(static_cast<std::size_t>(m_scale) + 1 - result.length(), '0')); // Pad with leading zeros
                }
                std::size_t int_part_len = result.length() - static_cast<std::size_t>(m_scale);
                std::string int_part = result.substr(0, int_part_len);
                std::string frac_part = result.substr(int_part_len);
                result = int_part + "." + frac_part;
            }
            else
            {
                result += std::string(static_cast<std::size_t>(-m_scale), '0'); // Append zeros
            }
            // handle negative values
            if(m_value < 0)
            {
                result.insert(0, 1, '-');
            }
            return result;
        }
            

        const T & storage() const
        {
            return m_value;
        }
       
        int scale() const
        {
            return m_scale;
        }
            
    private:
        template<class FLOAT_TYPE>
        FLOAT_TYPE convert_to_floating_point() const requires(!is_int_placeholder_v<T>)
        {
            using to_type = FLOAT_TYPE;
            if constexpr( std::is_same_v<T, int256_t> )
            {
                // danger zone
                auto val = static_cast<int128_t>(m_value);
                return static_cast<to_type>(val) / static_cast<to_type>(std::pow(10, m_scale));
            }
            else
            {
                return static_cast<to_type>(m_value) / static_cast<to_type>(std::pow(10, m_scale));
            }
        }


        T m_value;
        //int m_precision;
        int m_scale;
    };

    template <typename T>
    constexpr bool is_decimal_v = mpl::is_type_instance_of_v<T, decimal>;

} // namespace sparrow