// Copyright 2024 Man Group Operations Limited
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "backtrace.hpp"

#include <array>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>


// Platform-specific includes
#ifdef _WIN32
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
// clang-format off
#    include <windows.h>
#    include <dbghelp.h>
#    include <winnt.h>
// clang-format on


#    pragma comment(lib, "dbghelp.lib")
#elif defined(__linux__) || defined(__APPLE__)
#    include <cstdlib>

#    include <dlfcn.h>
#    include <execinfo.h>
#    include <unistd.h>

#    ifdef __linux__
#        include <cxxabi.h>
#    endif
#endif

namespace sparrow::test
{
    namespace
    {  // Global flag to prevent recursive signal handling
        static volatile std::sig_atomic_t in_signal_handler = 0;

        void signal_handler(int sig)
        {
            // Prevent recursive calls
            if (in_signal_handler != 0)
            {
                std::_Exit(EXIT_FAILURE);
            }
            in_signal_handler = 1;

            const char* signal_name = "Unknown";
            switch (sig)
            {
                case SIGSEGV:
                    signal_name = "SIGSEGV (Segmentation fault)";
                    break;
                case SIGABRT:
                    signal_name = "SIGABRT (Abort)";
                    break;
                case SIGFPE:
                    signal_name = "SIGFPE (Floating point exception)";
                    break;
                case SIGILL:
                    signal_name = "SIGILL (Illegal instruction)";
                    break;
#ifndef _WIN32
                case SIGBUS:
                    signal_name = "SIGBUS (Bus error)";
                    break;
#endif
                default:
                    break;
            }
            std::cerr << "\n*** Crash detected in unit tests ***\n";
            std::cerr << "Signal: " << signal_name << " (" << sig << ")\n";
            std::cerr << "\nBacktrace:\n";
            std::cerr.flush();

            print_backtrace(2);  // Skip signal_handler and the signal trampoline

            std::cerr << "\n*** End of backtrace ***\n";
            std::cerr.flush();

            // Re-raise the signal with the default handler
            std::signal(sig, SIG_DFL);
            std::raise(sig);
        }

#ifdef _WIN32
        [[maybe_unused]] std::string demangle_symbol(const char* name)
        {
            return std::string(name);  // Windows doesn't mangle C++ names in stack traces
        }
#elif defined(__linux__)
        std::string demangle_symbol(const char* name)
        {
            int status = 0;
            char* demangled = abi::__cxa_demangle(name, nullptr, nullptr, &status);
            if (status == 0 && demangled)
            {
                std::string result(demangled);
                std::free(demangled);
                return result;
            }
            return std::string(name);
        }
#else
        std::string demangle_symbol(const char* name)
        {
            return std::string(name);  // macOS - could use dladdr for better symbols
        }
#endif
    }

    void initialize_backtrace_on_crash()
    {
        // Install signal handlers for common crash signals
        std::signal(SIGSEGV, signal_handler);
        std::signal(SIGABRT, signal_handler);
        std::signal(SIGFPE, signal_handler);
        std::signal(SIGILL, signal_handler);
#ifndef _WIN32
        std::signal(SIGBUS, signal_handler);
#endif

#ifdef _WIN32
        // Initialize symbol handler for Windows
        SymInitialize(GetCurrentProcess(), nullptr, TRUE);
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS);
#endif
    }

    std::string capture_backtrace(int skip_frames)
    {
        std::string result;

#ifdef _WIN32
        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();

        CONTEXT context;
        std::memset(&context, 0, sizeof(CONTEXT));
        context.ContextFlags = CONTEXT_FULL;
        RtlCaptureContext(&context);

        DWORD image_type = 0;
        STACKFRAME64 stackframe;
        std::memset(&stackframe, 0, sizeof(STACKFRAME64));

#    ifdef _M_IX86
        image_type = IMAGE_FILE_MACHINE_I386;
        stackframe.AddrPC.Offset = context.Eip;
        stackframe.AddrPC.Mode = AddrModeFlat;
        stackframe.AddrFrame.Offset = context.Ebp;
        stackframe.AddrFrame.Mode = AddrModeFlat;
        stackframe.AddrStack.Offset = context.Esp;
        stackframe.AddrStack.Mode = AddrModeFlat;
#    elif _M_X64
        image_type = IMAGE_FILE_MACHINE_AMD64;
        stackframe.AddrPC.Offset = context.Rip;
        stackframe.AddrPC.Mode = AddrModeFlat;
        stackframe.AddrFrame.Offset = context.Rsp;
        stackframe.AddrFrame.Mode = AddrModeFlat;
        stackframe.AddrStack.Offset = context.Rsp;
        stackframe.AddrStack.Mode = AddrModeFlat;
#    elif _M_IA64
        image_type = IMAGE_FILE_MACHINE_IA64;
        stackframe.AddrPC.Offset = context.StIIP;
        stackframe.AddrPC.Mode = AddrModeFlat;
        stackframe.AddrFrame.Offset = context.IntSp;
        stackframe.AddrFrame.Mode = AddrModeFlat;
        stackframe.AddrBStore.Offset = context.RsBSP;
        stackframe.AddrBStore.Mode = AddrModeFlat;
        stackframe.AddrStack.Offset = context.IntSp;
        stackframe.AddrStack.Mode = AddrModeFlat;
#    endif

        constexpr int max_frames = 256;
        for (int frame = 0; frame < max_frames; frame++)
        {
            BOOL stack_walk_ok = StackWalk64(
                image_type,
                process,
                thread,
                &stackframe,
                &context,
                nullptr,
                SymFunctionTableAccess64,
                SymGetModuleBase64,
                nullptr
            );

            if (stack_walk_ok == FALSE || stackframe.AddrPC.Offset == 0)
            {
                break;
            }

            if (frame < skip_frames)
            {
                continue;
            }
            constexpr size_t buffer_size = sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR);
            std::array<std::uint8_t, buffer_size> buffer_array{};
            SYMBOL_INFO* symbol = static_cast<SYMBOL_INFO*>(static_cast<void*>(buffer_array.data()));
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol->MaxNameLen = MAX_SYM_NAME;

            DWORD64 displacement = 0;
            if (SymFromAddr(process, stackframe.AddrPC.Offset, &displacement, symbol) != FALSE)
            {
                IMAGEHLP_LINE64 line;
                line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
                DWORD line_displacement = 0;
                result += "  ";
                const char* symbol_name = symbol->Name;
                result += std::string(symbol_name);
                if (SymGetLineFromAddr64(process, stackframe.AddrPC.Offset, &line_displacement, &line) != FALSE)
                {
                    result += " at ";
                    const char* filename = line.FileName ? line.FileName : "<unknown file>";
                    result += std::string(filename);
                    result += ":";
                    result += std::to_string(line.LineNumber);
                }
                result += "\n";
            }
            else
            {
                result += "  <unknown symbol>\n";
            }
        }

#elif defined(__linux__) || defined(__APPLE__)
        void* array[256];
        int size = backtrace(array, 256);
        char** strings = backtrace_symbols(array, size);

        if (strings != nullptr)
        {
            for (int i = skip_frames; i < size; i++)
            {
                std::string frame_info(strings[i]);

                // Try to extract and demangle the function name
                size_t start = frame_info.find('(');
                size_t end = frame_info.find('+', start);
                if (start != std::string::npos && end != std::string::npos && end > start + 1)
                {
                    std::string mangled_name = frame_info.substr(start + 1, end - start - 1);
                    if (!mangled_name.empty())
                    {
                        std::string demangled = demangle_symbol(mangled_name.c_str());
                        frame_info.replace(start + 1, end - start - 1, demangled);
                    }
                }

                result += "  ";
                result += frame_info;
                result += "\n";
            }
            std::free(strings);
        }
        else
        {
            result = "  <backtrace_symbols failed>\n";
        }
#else
        result = "  <backtrace not available on this platform>\n";
#endif

        return result;
    }

    void print_backtrace(int skip_frames)
    {
        std::string backtrace_str = capture_backtrace(skip_frames + 1);  // +1 to skip this function
        std::cerr << backtrace_str;
        std::cerr.flush();
    }
}
