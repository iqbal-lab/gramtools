#include <algorithm>
#include <cstdlib>

#include "fm_index.hpp"
#include "bwt_search.hpp"
#include "ranks.hpp"
#include "kmers.hpp"


#define THREADS 25


inline bool fexists(const std::string &name) {
    ifstream f(name.c_str());
    return f.good();
}


//trim from start
static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}


// trim from end
static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}


// trim from both ends
static inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}


std::vector<std::string> split(std::string cad, std::string delim) {
    std::vector<std::string> v;
    int p, q, d;

    q = cad.size();
    p = 0;
    d = strlen(delim.c_str());
    while (p < q) {
        int posfound = cad.find(delim, p);
        std::string token;

        if (posfound >= 0)
            token = cad.substr(p, posfound - p);
        else
            token = cad.substr(p, cad.size() + 1);
        p += token.size() + d;
        trim(token);
        v.push_back(token);
    }
    return v;
}


void calc_kmer_matches(KmerIdx &kmer_idx,
                       KmerIdx &kmer_idx_rev,
                       KmerSites &kmer_sites,
                       sequence_set<std::vector<uint8_t>> &kmers_in_ref,
                       std::vector<std::vector<uint8_t>> &kmers,
                       const FM_Index &fm_index,
                       const DNA_Rank &rank_all,
                       const VariantMarkers &variants,
                       const std::vector<int> &mask_a,
                       const int k,
                       const uint64_t maxx,
                       int thread_id) {

    std::cout << "In thread: " << thread_id << std::endl;

    if (thread_id == 3) {
        std::cout << "kmer_idx.size: " << kmer_idx.size() << std::endl;
        std::cout << "kmer_idx_rev.size: " << kmer_idx_rev.size() << std::endl;
        std::cout << "kmers_in_ref.size: " << kmers_in_ref.size() << std::endl;
    }

    int j = 0;
    for (auto &kmer: kmers) {
        if (thread_id == 3) {
            std::cout << "Processing kmer: " << j << std::endl;
            std::cout << "Kmer size: " << kmer.size() << std::endl;
        }

        kmer_idx[kmer] = std::list<std::pair<uint64_t, uint64_t>>();
        kmer_idx_rev[kmer] = std::list<std::pair<uint64_t, uint64_t>>();
        kmer_sites[kmer] = std::list<std::vector<std::pair<uint32_t, std::vector<int>>>>();

        bool first_del = false;
        bool kmer_precalc_done = false;

        if (thread_id == 3) {
            std::cout << "Calling bidir_search_bwd" << std::endl;
        }

        bidir_search_bwd(fm_index,
                         0, fm_index.size(),
                         0, fm_index.size(),
                         kmer.begin(), kmer.end(),
                         kmer_idx[kmer],
                         kmer_idx_rev[kmer],
                         kmer_sites[kmer],
                         mask_a, maxx, first_del,
                         kmer_precalc_done, variants, rank_all,
                         thread_id);

        if (kmer_idx[kmer].empty())
            kmer_idx.erase(kmer);

        if (kmer_idx_rev[kmer].empty())
            kmer_idx_rev.erase(kmer);

        if (!first_del)
            kmers_in_ref.insert(kmer);

        if (thread_id == 3) {
            std::cout << "kmer_idx.size: " << kmer_idx.size() << std::endl;
            std::cout << "kmer_idx_rev.size: " << kmer_idx_rev.size() << std::endl;
            std::cout << "kmers_in_ref.size: " << kmers_in_ref.size() << std::endl;
        }
    }
}


void *worker(void *st) {
    ThreadData *th = (ThreadData *) st;

    calc_kmer_matches(*(th->kmer_idx),
                      *(th->kmer_idx_rev),
                      *(th->kmer_sites),
                      *(th->kmers_in_ref),
                      *(th->kmers),
                      *(th->fm_index),
                      *(th->rank_all),
                      *(th->variants),
                      *(th->mask_a),
                      th->k,
                      th->maxx,
                      th->thread_id);
    return NULL;
}


void gen_precalc_kmers(const FM_Index &fm_index,
                       const std::vector<int> &mask_a,
                       const std::string &kmer_fname,
                       const uint64_t maxx,
                       const int k,
                       const VariantMarkers &variants,
                       const DNA_Rank &rank_all) {

    pthread_t threads[THREADS];
    struct ThreadData td[THREADS];
    std::vector<std::vector<uint8_t>> kmers[THREADS];

    std::ifstream kfile;
    std::string line;
    kfile.open(kmer_fname);

    int i = 0;
    while (std::getline(kfile, line)) {
        std::vector<uint8_t> kmer;
        for (const auto &c: line)
            switch (c) {
                case 'A':
                case 'a':
                    kmer.push_back(1);
                    break;
                case 'C':
                case 'c':
                    kmer.push_back(2);
                    break;
                case 'G':
                case 'g':
                    kmer.push_back(3);
                    break;
                case 'T':
                case 't':
                    kmer.push_back(4);
                    break;
            }
        kmers[i++].push_back(kmer);
        i %= THREADS;
    }

    sequence_map<std::vector<uint8_t>, std::list<std::pair<uint64_t, uint64_t>>> kmer_idx[THREADS], kmer_idx_rev[THREADS];
    sequence_map<std::vector<uint8_t>, std::list<std::vector<std::pair<uint32_t, std::vector<int>>>>> kmer_sites[THREADS];
    sequence_set<std::vector<uint8_t>> kmers_in_ref[THREADS];

    std::cout << "Creating threads" << std::endl;

    for (int i = 0; i < THREADS; i++) {
        td[i].variants = &variants;
        td[i].fm_index = &fm_index;
        td[i].k = k;
        td[i].kmer_idx = &kmer_idx[i];
        td[i].kmer_idx_rev = &kmer_idx_rev[i];
        td[i].kmer_sites = &kmer_sites[i];
        td[i].mask_a = &mask_a;
        td[i].maxx = maxx;
        td[i].kmers_in_ref = &kmers_in_ref[i];
        td[i].kmers = &kmers[i];
        td[i].rank_all = &rank_all;
        td[i].thread_id = i;

        std::cout << "Starting thread: " << i << std::endl;
        pthread_create(&threads[i], NULL, worker, &td[i]);
    }

    std::ofstream precalc_file;
    precalc_file.open(std::string(kmer_fname) + ".precalc");

    std::cout << "Joining threads" << std::endl;

    for (int i = 0; i < THREADS; i++) {
        void *status;
        pthread_join(threads[i], &status);
        for (auto obj: kmer_idx[i]) {
            auto k = obj.first;
            for (auto n: k) precalc_file << (int) n << ' ';
            precalc_file << '|';

            if (kmers_in_ref[i].count(k)) {
                precalc_file << 1;
            } else {
                precalc_file << 0;
            }
            precalc_file << '|';
            for (auto o: kmer_idx[i][k]) precalc_file << o.first << ' ' << o.second << ' ';
            precalc_file << '|';
            for (auto o: kmer_idx_rev[i][k]) precalc_file << o.first << ' ' << o.second << ' ';
            precalc_file << '|';
            for (auto o: kmer_sites[i][k]) {
                for (auto v: o) {
                    precalc_file << v.first << ' ';
                    for (auto n: v.second) precalc_file << (int) n << ' ';
                    precalc_file << '@';
                }
                precalc_file << '|';
            }

            precalc_file << std::endl;
        }
    }
}


void read_precalc_kmers(std::string fil,
                        sequence_map<std::vector<uint8_t>, std::list<std::pair<uint64_t, uint64_t>>> &kmer_idx,
                        sequence_map<std::vector<uint8_t>, std::list<std::pair<uint64_t, uint64_t>>> &kmer_idx_rev,
                        sequence_map<std::vector<uint8_t>, std::list<std::vector<std::pair<uint32_t, std::vector<int>>>>> &kmer_sites,
                        sequence_set<std::vector<uint8_t>> &kmers_in_ref) {

    std::ifstream kfile;
    std::string line;
    kfile.open(fil);

    while (std::getline(kfile, line)) {
        std::vector<std::string> parts = split(line, "|");
        std::list<std::pair<uint64_t, uint64_t>> idx, idx_rev;
        std::list<std::vector<std::pair<uint32_t, std::vector<int>>>> sites;

        std::vector<uint8_t> kmer;
        for (auto c: split(parts[0], " ")) kmer.push_back(std::stoi(c));

        if (!parts[1].compare("1"))
            kmers_in_ref.insert(kmer);

        std::vector<std::string> idx_str = split(parts[2], " ");
        std::vector<std::string> idx_rev_str = split(parts[3], " ");
        for (unsigned int i = 0; i < idx_str.size() / 2; i++)
            idx.push_back(std::pair<uint64_t, uint64_t>(std::stoi(idx_str[i * 2]), std::stoi(idx_str[i * 2 + 1])));
        for (unsigned int i = 0; i < idx_rev_str.size() / 2; i++)
            idx_rev.push_back(
                    std::pair<uint64_t, uint64_t>(std::stoi(idx_rev_str[i * 2]), std::stoi(idx_rev_str[i * 2 + 1])));

        int flag = 0;
        if (!idx.empty()) {
            kmer_idx[kmer] = idx;
            flag = 1;
        }
        if (!idx_rev.empty())
            kmer_idx_rev[kmer] = idx_rev;

        if (flag == 1) {
            for (unsigned int i = 4; i < parts.size(); i++) {
                std::vector<std::pair<uint32_t, std::vector<int>>> v;
                for (auto pair_i_v: split(parts[i], "@")) {
                    std::vector<std::string> strvec = split(pair_i_v, " ");
                    if (strvec.size()) {
                        int first = std::stoi(strvec[0]);

                        std::vector<int> rest;
                        for (unsigned int i = 1; i < strvec.size(); i++)
                            if (strvec[i].size())
                                rest.push_back(std::stoi(strvec[i]));

                        v.push_back(std::pair<uint32_t, std::vector<int>>(first, rest));
                    }
                }
                sites.push_back(v);
            }
            kmer_sites[kmer] = sites;
        }
    }
}


KmersData get_kmers(const FM_Index &fm_index,
                    const std::vector<int> &mask_a,
                    const std::string &kmer_fname,
                    const uint64_t maxx,
                    const int k,
                    const VariantMarkers &variants,
                    const DNA_Rank &rank_all) {

    if (!fexists(std::string(kmer_fname) + ".precalc")) {
        std::cout << "Precalculated kmers not found, calculating them using "
                  << THREADS << " threads" << std::endl;
        gen_precalc_kmers(fm_index, mask_a, kmer_fname, maxx, k, variants, rank_all);
        std::cout << "Finished precalculating kmers" << std::endl;
    }

    std::cout << "Reading K-mers" << std::endl;
    KmersData kmers;
    read_precalc_kmers(std::string(kmer_fname) + ".precalc", kmers.index,
                       kmers.index_reverse, kmers.sites,
                       kmers.in_reference);
    return kmers;
}
