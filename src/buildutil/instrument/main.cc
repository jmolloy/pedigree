/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define PEDIGREE_EXTERNAL_SOURCE 1

#include <getopt.h>
#include <stdlib.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <lib/instrument.h>

#define RECORDS_PER_READ        128

struct InstrumentedFunction;

// Dataset helper types.
typedef std::unordered_map<uintptr_t, std::shared_ptr<InstrumentedFunction>> dataset_t;
typedef std::pair<uintptr_t, std::shared_ptr<InstrumentedFunction>> dataset_pair_t;

// Caller count helper types.
typedef std::unordered_map<uintptr_t, size_t> counter_t;
typedef std::pair<uintptr_t, size_t> counter_pair_t;

template <class T>
class has_caller_data : public std::false_type
{
};

template <>
class has_caller_data<InstrumentationRecord> : public std::true_type
{
};

enum long_opts
{
    opt_input_file,
    opt_max_rows,
};

struct InstrumentedFunction
{
    /**
     * Address of this function, for symbol name lookups.
     */
    uintptr_t address;

    /**
     * Map of callsite -> count, should be sorted on count before using for
     * any actual analysis.
     */
    counter_t callerCounts;

    /**
     * Total times this function has been called.
     */
    size_t totalCalls;

    /** Comparer for the given InstrumentedFunction objects. */
    static bool comparer(const dataset_pair_t &left, const dataset_pair_t &right)
    {
        return left.second->totalCalls > right.second->totalCalls;
    }
};

/// \todo if uintptr_t == uint32_t, this will not work.
uintptr_t extendPointer(uint32_t pointer)
{
    return static_cast<uintptr_t>(pointer) | 0xFFFFFFFF00000000ULL;
}

uintptr_t extendPointer(uintptr_t pointer)
{
    return pointer;
}

std::string symbolToName(uintptr_t function, const char *kernel)
{
    char buf[256];
    snprintf(buf, 256, "addr2line -C -p -s -f -i -e %s %lx", kernel, function);

    FILE *fp = popen(buf, "r");
    if (!fp)
    {
        return std::string("(unknown)");
    }

    std::string result;

    char buffer[128];
    while (!feof(fp))
    {
        ssize_t n = fread(buffer, 1, 128, fp);
        if (n > 0)
        {
            buffer[n] = 0;
            result += buffer;
        }
    }

    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    return result;
}

/**
 * Processes records that are marked as "lite" instrumentation records.
 */
bool processRecord(const LiteInstrumentationRecord &record, dataset_t &dataset)
{
    uintptr_t function = extendPointer(record.data.function);

    auto it = dataset.find(function);
    if (it == dataset.end())
    {
        auto member = std::make_shared<InstrumentedFunction>();
        member->totalCalls = 1;
        member->address = function;
        dataset.insert(std::make_pair(function, member));
    }
    else
    {
        ++(it->second->totalCalls);
    }

    return true;
}

/**
 * Processes standard instrumentation records.
 */
bool processRecord(const InstrumentationRecord &record, dataset_t &dataset)
{
    if (record.data.magic != INSTRUMENT_MAGIC)
    {
        std::cerr << "Aborting file read due to magic mismatch: ";
        std::cerr << std::hex << record.data.magic << " is not ";
        std::cerr << INSTRUMENT_MAGIC << std::dec << std::endl;
        return false;
    }

    uint8_t flags = record.data.flags;
    if ((flags & INSTRUMENT_RECORD_ENTRY) != INSTRUMENT_RECORD_ENTRY)
    {
        return true;
    }

    // Have we seen this function already?
    uintptr_t function = record.data.function;
    uintptr_t caller = record.data.caller;
    auto it = dataset.find(function);
    if (it == dataset.end())
    {
        // Insert.
        auto member = std::make_shared<InstrumentedFunction>();
        member->totalCalls = 1;
        member->address = function;
        dataset.insert(std::make_pair(function, member));
    }
    else
    {
        ++(it->second->totalCalls);
        it->second->callerCounts[caller] += 1;
    }

    return true;
}

template <class T>
int processRecords(FILE *fp, const char *kernel, int max_records)
{
    // function -> instrumentation data structure.
    dataset_t dataset;

    // Read several records at a time.
    T *records = new T[RECORDS_PER_READ];
    while(!feof(fp))
    {
        bool err = false;
        ssize_t record_count = fread(records, sizeof(T), RECORDS_PER_READ, fp);
        for (ssize_t i = 0; i < record_count; ++i)
        {
            if (!processRecord(records[i], dataset))
            {
                err = true;
                break;
            }
        }

        if (err)
            break;
    }
    delete [] records;

    // Sort by call count (std::unordered_map is not sortable).
    std::vector<dataset_pair_t> vec(dataset.begin(), dataset.end());
    std::sort(vec.begin(), vec.end(), InstrumentedFunction::comparer);

    // Print the top functions.
    int i = 0;
    for (auto it = vec.begin(); it != vec.end() && i < max_records; ++it, ++i)
    {
        std::cout << "Called " << it->second->totalCalls << " times:" << std::endl;
        std::cout << "    " << symbolToName(it->second->address, kernel) << std::endl;

        // Show top 10 callers.
        if (has_caller_data<T>::value)
        {
            counter_t &counts = it->second->callerCounts;
            std::vector<counter_pair_t> count_vec(counts.begin(), counts.end());
            std::sort(count_vec.begin(), count_vec.end(),
                [](const counter_pair_t &left, const counter_pair_t &right)
                {
                    return left.second > right.second;
                });

            int j = 0;
            for (auto it2 = count_vec.begin(); it2 != count_vec.end() && j < max_records; ++it2, ++j)
            {
                std::cout << "        -> " << it2->second << "x ";
                std::cout << symbolToName(it2->first, kernel) << std::endl;
            }
        }
    }

    return true;
}

int handleFile(const char *filename, const char* kernel, int max_records)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
        std::cerr << "Can't open '" << filename << "': " << strerror(errno) << std::endl;
        return 1;
    }

    // Read the file-wide flags to control the behavior of the main loops.
    uint8_t global_flags = 0;
    ssize_t n = fread(&global_flags, sizeof(uint8_t), 1, fp);
    if (n != 1)
    {
        std::cerr << "Failed to read global flags: " << strerror(errno) << std::endl;
        fclose(fp);
        return 1;
    }

    // Choose which type of read to perform.
    int rc = 0;
    if (global_flags & INSTRUMENT_GLOBAL_LITE)
    {
        rc = processRecords<LiteInstrumentationRecord>(fp, kernel, max_records);
    }
    else
    {
        rc = processRecords<InstrumentationRecord>(fp, kernel, max_records);
    }


    fclose(fp);
    return rc;
}

void usage()
{
    /// \todo add path to serial port output and use scripts/addr2line.py
    std::cerr << "Usage: instrument [options]" << std::endl;
    std::cerr << "Parse a given instrumentation data file and present findings." << std::endl;
    std::cerr << std::endl;
    std::cerr << "  --version, -[vV] Print version and exit successfully." << std::endl;
    std::cerr << "  --help,          Print this help and exit successfully." << std::endl;
    std::cerr << "  --input-file, -i Path to the input data file to parse." << std::endl;
    std::cerr << "  --kernel-file, -k Path to the kernel file to use for symbol resolution (default: build/kernel/kernel.debug)." << std::endl;
    std::cerr << "  --max-rows, -m   Maximum rows to output (default is 10)." << std::endl;
    std::cerr << std::endl;

}

void version()
{
    std::cerr << "instrument v1.0, Copyright (C) 2014, Pedigree Developers" << std::endl;
}

int main(int argc, char *argv[])
{
    const char *input_file = 0;
    const char *kernel_file = 0;
    int maximum = 10;
    const struct option long_options[] =
    {
        {"input-file", required_argument, 0, 'i'},
        {"kernel-path", required_argument, 0, 'k'},
        {"max-rows", optional_argument, 0, 'm'},
        {"version", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0},
    };

    opterr = 1;
    while (1)
    {
        int c = getopt_long(argc, argv, "i:m:k:vVh", long_options, NULL);
        if (c < 0)
        {
            break;
        }

        switch (c)
        {
            case 'i':
                input_file = optarg;
                break;

            case 'm':
                {
                    // Perform conversion and handle errors.
                    char *end = 0;
                    int possible_max = strtol(optarg, &end, 10);
                    if (end != optarg)
                    {
                        maximum = possible_max;
                    }
                    else
                    {
                        std::cerr << "Could not convert maximum row '" << optarg << "' to a number." << std::endl;
                        return 1;
                    }
                }
                break;

            case 'v':
            case 'V':
                version();
                return 0;

            case ':':
                std::cerr << "At least one required option was missing." << std::endl;
            case 'h':
                usage();
                return 0;

            default:
                usage();
                return 1;
        }
    }

    argc -= optind;
    argv += optind;

    if (!input_file)
    {
        usage();
        return 1;
    }

    if (!kernel_file)
    {
        kernel_file = "build/kernel/kernel.debug";
    }

    return handleFile(input_file, kernel_file, maximum);
}
