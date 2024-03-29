/*
      This file is part of my_cuda
      Copyright (C) 2021  Julien Thevenon ( julien_thevenon at yahoo.fr )

      This program is free software: you can redistribute it and/or modify
      it under the terms of the GNU General Public License as published by
      the Free Software Foundation, either version 3 of the License, or
      (at your option) any later version.

      This program is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
      GNU General Public License for more details.

      You should have received a copy of the GNU General Public License
      along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#ifndef MY_CUDA_CUDA_PRINT_H
#define MY_CUDA_CUDA_PRINT_H

#include "my_cuda.h"

namespace my_cuda
{
#ifdef LOG_EXECUTION
    /**
     * Device implementation for strlen
     * @param p_string
     * @return same than strlen
     */
    __device__
    unsigned int my_strlen(const char * p_string)
    {
        unsigned int l_index = 0;
        while('\0' != p_string[l_index])
        {
            ++l_index;
        }
        return l_index;
    }

    /**
     * Count the number of carriage return in a null terminated a char array
     * @param p_string
     * @return number of carriage return in char array
     */
    __device__
    unsigned int count_cr(const char * p_string)
    {
        unsigned int l_nb_cr = 0;
        unsigned int l_index = 0;
        while('\0' != p_string[l_index])
        {
            l_nb_cr += ('\n' == p_string[l_index++]);
        }
        return l_nb_cr;
    }

    /**
     * Compute a new format including CUDA thread information and suited to the
     * number of lines contained in format
     * @param p_level indentation level
     * @param p_header text to print
     * @param p_format format
     * @return new format adapted to the text provided in p_header
     */
    __device__
    char *
    prepare_format(unsigned int p_level
                  ,const char * p_header
                  ,const char * p_format
                  )
    {
        unsigned int l_len = my_strlen(p_format);
        unsigned int l_nb_cr = count_cr(p_format);
        bool l_additional_cr = (!l_len) || (p_format[l_len - 1] != '\n');
        unsigned int l_total_cr = l_nb_cr + l_additional_cr;
        unsigned int l_header_len = my_strlen(p_header);
        // Don't forget to add + 1 to the allocated size as strlen does not count final '/0'
        char * l_format = static_cast<char*>(malloc(1 + l_len + l_additional_cr + l_total_cr * (l_header_len + 2 * p_level)));
        unsigned int l_index = 0;
        unsigned int l_char_index = 0;
        for(unsigned int l_line_index = 0; l_line_index < l_total_cr; ++l_line_index)
        {
            for(unsigned int l_level_index = 0; l_level_index < 2 * p_level; ++l_level_index)
            {
                l_format[l_index++] = ' ';
            }
            for(unsigned int l_header_index = 0; l_header_index < l_header_len; ++l_header_index)
            {
                l_format[l_index++] = l_line_index ? ' ' : p_header[l_header_index];
            }
            while('\0' != p_format[l_char_index] && '\n' != p_format[l_char_index])
            {
                l_format[l_index++] = p_format[l_char_index++];
            }
            ++l_char_index;
            l_format[l_index++] = '\n';
        }
        l_format[l_index++] = '\0';
        return l_format;
    }
#endif // LOG_EXECUTION

    /**
     * Make all thread print the message
     * @tparam Targs type of arguments to display
     * @param p_level indentation level
     * @param p_format format indication
     * @param p_fargs arguments to display
     */
    template<typename... Targs>
    __device__
    void print_all(unsigned int p_level
#ifndef ENABLE_CUDA_CODE
                  ,dim3 threadIdx
#endif // ENABLE_CUDA_CODE
                  ,const char * p_format
                  ,Targs... p_fargs
                  )
    {
#ifdef LOG_EXECUTION
        char * l_format = prepare_format(p_level, "Thread%3i : ", p_format);
        printf(l_format, threadIdx.x, p_fargs...);
        free(l_format);
#endif // LOG_EXECUTION
    }

    /**
     * Make only threads whose corresponding mask bit is set print the message
     * @tparam Targs type of arguments to display
     * @param p_level indentation level
     * @param p_format format indication
     * @param p_fargs arguments to display
     */
    template<typename... Targs>
    __device__
    void print_mask(unsigned int p_level
                   ,uint32_t p_mask
#ifndef ENABLE_CUDA_CODE
                   ,dim3 threadIdx
#endif // ENABLE_CUDA_CODE
                   ,const char * p_format
                   ,Targs... p_fargs
                   )
    {
#ifdef LOG_EXECUTION
        // Do format treatment in all threads to minimise divergence
        char * l_format = prepare_format(p_level, "Thread%3i : ", p_format);
        if((1u << threadIdx.x) & p_mask)
        {
            printf(l_format, threadIdx.x, p_fargs...);
        }
        free(l_format);
#endif // LOG_EXECUTION
    }

    /**
     * Only first thread in warp print the message
     * @tparam Targs type of arguments to display
     * @param p_level indentation level
     * @param p_format format indication
     * @param Fargs arguments to display
     */
    template<typename... Targs>
    __device__
    void print_single(unsigned int p_level
#ifndef ENABLE_CUDA_CODE
                     ,dim3 threadIdx
#endif // ENABLE_CUDA_CODE
                     ,const char * p_format
                     ,Targs... Fargs
                     )
    {
#ifdef LOG_EXECUTION
        char * l_format = prepare_format(p_level, "", p_format);
        if(!threadIdx.x)
        {
            printf(l_format, Fargs...);
        }
        free(l_format);
#endif // LOG_EXECUTION
    }

#ifndef ENABLE_CUDA_CODE
    /**
     * Wrappers emulating
     * @tparam Targs
     * @param p_level
     * @param p_format
     * @param p_fargs
     */
    template<typename... Targs>
    __device__
    void print_single(unsigned int p_level
                     ,const char * p_format
                     ,Targs... p_fargs
                     )
    {
#ifdef LOG_EXECUTION
        print_single(p_level,{0, 1, 1}, p_format, p_fargs...);
#endif // LOG_EXECUTION
    }

    /**
     * Wrappers emulating
     * @tparam Targs
     * @param p_level
     * @param p_format
     * @param p_fargs
     */
    template<typename... Targs>
    __device__
    void print_single(unsigned int p_level
                     ,const char * p_format
                     )
    {
#ifdef LOG_EXECUTION
        print_single(p_level,{0, 1, 1}, p_format, 0);
#endif // LOG_EXECUTION
    }
#endif // ENABLE_CUDA_CODE


}
#endif //MY_CUDA_CUDA_PRINT_H
//EOF
