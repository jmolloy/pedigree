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
#include <string.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <utilities/MemoryTracing.h>

using namespace MemoryTracing;

#define RECORDS_PER_READ        128

typedef std::unordered_map<uint32_t, std::shared_ptr<AllocationTraceEntry>> dataset_t;
typedef std::pair<uint32_t, std::shared_ptr<AllocationTraceEntry>> dataset_pair_t;

struct CallerCountEntry
{
    CallerCountEntry() : count(0), size(0), entry()
    {}

    size_t count;
    size_t size;
    std::shared_ptr<AllocationTraceEntry> entry;
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

std::string symbolToName(uintptr_t function)
{
    char buf[256];
    snprintf(buf, 256, "python scripts/addr2line.py build/serial.txt %lx", function);

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

bool processRecord(AllocationTraceEntry &record, dataset_t &dataset)
{
    if (record.data.type == Free)
    {
        auto it = dataset.find(record.data.ptr);
        if (it == dataset.end())
        {
            /// \todo add more info
            std::cerr << "A double free has been detected." << std::endl;
            return false;
        }

        dataset.erase(it);
        return true;
    }
    else if (record.data.type == Allocation)
    {
        auto it = dataset.find(record.data.ptr);
        if (it != dataset.end())
        {
            /// \todo add more info
            std::cerr << "A pointer was allocated twice." << std::endl;
            return false;
        }

        auto entry = std::make_shared<AllocationTraceEntry>();
        *entry = record;

        dataset.insert(std::make_pair(record.data.ptr, entry));
    }

    return true;
}

int processRecords(FILE *fp, int max_records, bool reverse, bool common_callsites)
{
    // Dataset of pointers -> metadata.
    dataset_t dataset;

    // Total allocation counter.
    size_t totalAllocs = 0;

    // Read several records at a time.
    AllocationTraceEntry *records = new AllocationTraceEntry[RECORDS_PER_READ];
    while(!feof(fp))
    {
        bool err = false;
        ssize_t record_count = fread(records, sizeof(AllocationTraceEntry), RECORDS_PER_READ, fp);
        for (ssize_t i = 0; i < record_count; ++i)
        {
            if (records[i].data.type == Allocation)
            {
                ++totalAllocs;
            }

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

    std::cout << "This data file contains " << totalAllocs << " allocations in total." << std::endl;

    // All that remains in the dataset is the unfreed memory.
    std::vector<dataset_pair_t> vec(dataset.begin(), dataset.end());
    std::cout << "There are " << dataset.size() << " unfreed allocations." << std::endl;

    std::cout << std::endl;

    // What about the top N callers?
    if (common_callsites)
    {
        std::unordered_map<uint32_t, std::shared_ptr<CallerCountEntry>> entries;
        for (auto it : vec)
        {
            uint32_t which = it.second->data.bt[1];
            if (entries.find(which) == entries.end())
            {
                entries[which] = std::make_shared<CallerCountEntry>();
                entries[which]->count = 1;
                entries[which]->size = it.second->data.sz;
                entries[which]->entry = it.second;
            }
            else
            {
                ++(entries[which]->count);
                entries[which]->size += it.second->data.sz;
            }
        }

        std::vector<std::pair<uint32_t, std::shared_ptr<CallerCountEntry>>>
            entries_vec(entries.begin(), entries.end());
        if (reverse)
        {
            std::sort(entries_vec.begin(), entries_vec.end(), [] (
                    const std::pair<uint32_t, std::shared_ptr<CallerCountEntry>> &left,
                    const std::pair<uint32_t, std::shared_ptr<CallerCountEntry>> &right) -> bool {
                return left.second->count < right.second->count;
            });
        }
        else
        {
            std::sort(entries_vec.begin(), entries_vec.end(), [] (
                    const std::pair<uint32_t, std::shared_ptr<CallerCountEntry>> &left,
                    const std::pair<uint32_t, std::shared_ptr<CallerCountEntry>> &right) -> bool {
                return left.second->count > right.second->count;
            });
        }

        int i = 0;
        for (auto it = entries_vec.begin(); it != entries_vec.end() && i < max_records; ++it, ++i)
        {
            std::cout << " With " << it->second->count << " entries, a total of " << it->second->size << " bytes:" << std::endl;

            for (size_t i = 0; i < num_backtrace_entries; ++i)
            {
                if (!it->second->entry->data.bt[i])
                {
                    break;
                }

                uintptr_t caller = extendPointer(it->second->entry->data.bt[i]);
                std::cout << "    " << symbolToName(caller) << std::endl;
            }

            std::cout << std::endl;
        }
    }
    else
    {
        // Sort by call count (std::unordered_map is not sortable).
        if (reverse)
        {
            std::sort(vec.begin(), vec.end(), [] (const dataset_pair_t &left, const dataset_pair_t &right) -> bool {
                return left.second->data.sz < right.second->data.sz;
            });
        }
        else
        {
            std::sort(vec.begin(), vec.end(), [] (const dataset_pair_t &left, const dataset_pair_t &right) -> bool {
                return left.second->data.sz > right.second->data.sz;
            });
        }

        // Print the top functions.
        int i = 0;
        for (auto it = vec.begin(); it != vec.end() && i < max_records; ++it, ++i)
        {
            std::cout << it->second->data.sz << " bytes left unfreed @" << std::hex << extendPointer(it->first) << std::dec << "." << std::endl;
            for (size_t i = 0; i < num_backtrace_entries; ++i)
            {
                if (!it->second->data.bt[i])
                {
                    break;
                }

                uintptr_t caller = extendPointer(it->second->data.bt[i]);
                std::cout << "    " << symbolToName(caller) << std::endl;
            }

            std::cout << std::endl;
        }
    }

    size_t totalUnfreed = 0;
    for (auto it = vec.begin(); it != vec.end(); ++it)
    {
        totalUnfreed += it->second->data.sz;
    }

    std::cout << "A total of " << totalUnfreed << " bytes remained unfreed." << std::endl;

    return true;
}

int handleFile(const char *filename, int max_records, bool reverse, bool common_callsites)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
        std::cerr << "Can't open '" << filename << "': " << strerror(errno) << std::endl;
        return 1;
    }

    int rc = processRecords(fp, max_records, reverse, common_callsites);

    fclose(fp);
    return rc;
}

void usage()
{
    /// \todo add path to serial port output and use scripts/addr2line.py
    std::cerr << "Usage: memorytracer [options]" << std::endl;
    std::cerr << "Parse a given memory trace data file and present findings." << std::endl;
    std::cerr << std::endl;
    std::cerr << "  --version, -[vV] Print version and exit successfully." << std::endl;
    std::cerr << "  --help,          Print this help and exit successfully." << std::endl;
    std::cerr << "  --input-file, -i Path to the input data file to parse." << std::endl;
    std::cerr << "  --max-rows, -m   Maximum rows to output (default is 10)." << std::endl;
    std::cerr << "  --callers, -c    Show most common callers with unfreed allocations, rather than the unfreed allocations themselves." << std::endl;
    std::cerr << "  --reverse, -r    Show results in reverse (smallest first, or lower count first)." << std::endl;
    std::cerr << std::endl;

}

void version()
{
    std::cerr << "memorytracer v1.0, Copyright (C) 2014, Pedigree Developers" << std::endl;
}

int main(int argc, char *argv[])
{
    const char *input_file = 0;
    bool reverse = false;
    bool callers = false;
    int maximum = 10;
    const struct option long_options[] =
    {
        {"input-file", required_argument, 0, 'i'},
        {"max-rows", optional_argument, 0, 'm'},
        {"version", no_argument, 0, 'v'},
        {"callers", no_argument, 0, 'c'},
        {"reverse", no_argument, 0, 'r'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0},
    };

    opterr = 1;
    while (1)
    {
        int c = getopt_long(argc, argv, "i:m:vVhcr", long_options, NULL);
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

            case 'r':
                reverse = true;
                break;

            case 'c':
                callers = true;
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

    return handleFile(input_file, maximum, reverse, callers);
}
