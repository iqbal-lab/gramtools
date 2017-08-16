#include <sdsl/suffix_arrays.hpp>

#include "kmers.hpp"
#include "bwt_search.hpp"
#include "map.hpp"
#include "bidir_search_bwd.hpp"

/*
 * starts at end of the read, want to do fwd in the future
 *
 *
 * adds sa intervals (gives number of matches: sa interval gives you the position of each match in the prg via the suffix array)
 * sites -> variant markers within the sa interval: (everything between odd numbers)
 *  pair: site and allele
 * alleles: separated by even numbers above 5
 *
 *
 * Sites &sites
 * * std::pair -> one variant site
 * ** uint64_t -> variant site (the odd number character)
 * ** std::vector<int> -> each int is one allele, subset of alleles in variant site
 * (an index, starts at 1: 1 is first allele, 2 is second allele)
 *
 * * Site -> close variants sites expect reads to cross over,
 * tracks order of crossed (by read) variant sites if variant sites close together
 *
 *
 * * std::list -> the list tracks each match of the read, elements of the list are a "match"
 *
 * sa_intervals <--one-to-one--> sites
*/
void bidir_search_bwd(SA_Intervals &sa_intervals,
                      uint64_t left, uint64_t right,
                      Sites &sites,
                      bool &delete_first_interval,
                      const std::vector<uint8_t>::iterator fasta_pattern_begin,
                      const std::vector<uint8_t>::iterator fasta_pattern_end,
                      const std::vector<int> &allele_mask,
                      const uint64_t maxx,
                      const bool kmer_precalc_done,
                      const DNA_Rank &rank_all,
                      const FM_Index &fm_index,
                      const int thread_id) {

    // deals with empty (first in mapping) sa interval
    if (sa_intervals.empty()) {
        sa_intervals.emplace_back(std::make_pair(left, right));

        Site empty_pair_vector;
        sites.push_back(empty_pair_vector);
    }

    auto fasta_pattern_it = fasta_pattern_end;
    while (fasta_pattern_it > fasta_pattern_begin) {
        if (sa_intervals.empty())
            return;

        --fasta_pattern_it;
        const uint8_t next_char = *fasta_pattern_it;
        assert((next_char >= 1) && (next_char <= 4));

        assert(!sa_intervals.empty());
        auto sa_interval_it = sa_intervals.begin();

        assert(!sites.empty());
        auto sites_it = sites.begin();

        if (kmer_precalc_done or fasta_pattern_it != fasta_pattern_end - 1) {
            // loop sa interval (matches of current substring)
            auto count_sa_intervals = sa_intervals.size();
            for (auto j = 0; j < count_sa_intervals; j++) {

                process_matches_overlapping_variants(sa_intervals, sa_interval_it, sites, sites_it,
                                                     delete_first_interval, maxx,
                                                     allele_mask, fm_index, thread_id);
            }
        }

        assert(!sa_intervals.empty());
        sa_interval_it = sa_intervals.begin();

        assert(!sites.empty());
        sites_it = sites.begin();

        delete_first_interval = match_next_charecter(delete_first_interval,
                                                     sa_intervals,
                                                     sa_interval_it,
                                                     sites,
                                                     sites_it,
                                                     next_char,
                                                     rank_all,
                                                     fm_index, thread_id);
    }
}


void process_matches_overlapping_variants(SA_Intervals &sa_intervals,
                                          SA_Intervals::iterator &sa_interval_it,
                                          Sites &sites,
                                          Sites::iterator &sites_it,
                                          const bool delete_first,
                                          const uint64_t maxx,
                                          const std::vector<int> &allele_mask,
                                          const FM_Index &fm_index,
                                          const int thread_id) {

    // check for edge of variant site
    const auto sa_interval_start = (*sa_interval_it).first;
    const auto sa_interval_end = (*sa_interval_it).second;
    auto marker_positions = fm_index.wavelet_tree.range_search_2d(sa_interval_start,
                                                                  sa_interval_end - 1,
                                                                  5, maxx).second;

    uint64_t previous_marker = 0;
    uint64_t last_begin = 0;
    bool second_to_last = false;

    for (auto marker_position = marker_positions.begin(), zend = marker_positions.end();
         marker_position != zend;
         ++marker_position) {

        uint64_t marker_idx = (*marker_position).first;
        uint64_t marker = (*marker_position).second;

        uint64_t left_new;
        uint64_t right_new;
        bool ignore;

        add_sa_interval_for_skip(previous_marker, sa_interval_it,
                                 last_begin, second_to_last,
                                 marker_idx, marker,
                                 left_new, right_new,
                                 ignore);

        // takes all suffix at edge of variant and adds variant charecter to them
        // ac6cc6at5agt -> 5ac6cc6at5agt
        // last -> is end of variant site marker or not
        bool last = skip(left_new, right_new, maxx, marker, fm_index);

        if (!last && (marker % 2 == 1)) {
            last_begin = marker;
            if ((marker_position + 1 != zend) && (marker == (*(marker_position + 1)).second))
                second_to_last = true;
        }

        update_sites_crossed_by_reads(sa_intervals, sa_interval_it, left_new, right_new, sites, sites_it,
                                      second_to_last,
                                      ignore, last, last_begin, allele_mask, delete_first, marker, marker_idx, fm_index);

        previous_marker = marker;
    }
    ++sa_interval_it;
    ++sites_it;
}


void add_sa_interval_for_skip(uint64_t previous_marker,
                              SA_Intervals::iterator &sa_interval_it,
                              uint64_t &last_begin,
                              bool &second_to_last,
                              const uint64_t marker_idx,
                              const uint64_t marker,
                              uint64_t &left_new,
                              uint64_t &right_new,
                              bool &ignore) {

    left_new = (*sa_interval_it).first;
    right_new = (*sa_interval_it).second;
    ignore = ((marker == previous_marker) && (marker % 2 == 0)) ||
             ((marker % 2 == 0)
              && (marker == previous_marker + 1)
              && (marker == last_begin + 1));

    // if marker start or end of variant region
    if ((marker % 2 == 1) && (marker != previous_marker)) {
        second_to_last = false;
        last_begin = 0;
    }
    if (marker % 2 == 1) {
        left_new = marker_idx;
        right_new = marker_idx + 1;
    }
}


bool match_next_charecter(bool delete_first_interval,
                          SA_Intervals &sa_intervals,
                          SA_Intervals::iterator &sa_interval_it,
                          Sites &sites,
                          Sites::iterator &sites_it,
                          const uint8_t next_char,
                          const DNA_Rank &rank_all,
                          const FM_Index &fm_index,
                          const int thread_id) {

    // adds next charecter in the read; deletes sa intervals which dont match the new next character
    while (sa_interval_it != sa_intervals.end()
           && sites_it != sites.end()) {

        //calculate sum to return- can do this in top fcns
        const auto next_char_interval = bidir_search(next_char, sa_interval_it, rank_all, fm_index);

        uint64_t next_char_interval_size = next_char_interval.second - next_char_interval.first;

        if (next_char_interval_size != 0) {
            // Reduce SA interval to next_char interval
            sa_interval_it->first = next_char_interval.first;
            sa_interval_it->second = next_char_interval.second;

            ++sa_interval_it;
            ++sites_it;

            continue;
        }

        delete_first_interval = sa_interval_it == sa_intervals.begin();

        assert(!sites.empty());
        assert(sites_it != sites.end());

        sa_interval_it = sa_intervals.erase(sa_interval_it);
        sites_it = sites.erase(sites_it);

    }

    return delete_first_interval;
}


VariantSite get_variant_site_edge(std::vector<int> &allele,
                                  const uint64_t marker,
                                  const uint64_t marker_idx,
                                  const std::vector<int> &allele_mask,
                                  const bool last,
                                  const FM_Index &fm_index) {
    uint64_t site_edge_marker;

    bool marker_is_site_edge = marker % 2 == 1;
    if (marker_is_site_edge) {
        site_edge_marker = marker;
        if (!last)
            allele.push_back(1);
    } else {
        site_edge_marker = marker - 1;
        allele.push_back(allele_mask[fm_index[marker_idx]]);
    }
    return std::make_pair(site_edge_marker, allele);
}


void update_sites_crossed_by_reads(SA_Intervals &sa_intervals,
                                   SA_Intervals::iterator &sa_interval_it,
                                   uint64_t left_new,
                                   uint64_t right_new,
                                   Sites &sites,
                                   Sites::iterator &sites_it,
                                   bool &second_to_last,
                                   bool ignore,
                                   bool last,
                                   uint64_t &last_begin,
                                   const std::vector<int> &allele_mask,
                                   const bool &delete_first,
                                   const uint64_t marker,
                                   const uint64_t marker_idx,
                                   const FM_Index &fm_index) {

    if (sa_interval_it == sa_intervals.begin() && !delete_first && !ignore) {
        auto sa_interval = std::make_pair(left_new, right_new);
        sa_intervals.push_back(sa_interval);

        std::vector<int> allele_empty;
        allele_empty.reserve(3500);
        auto variant_site = get_variant_site_edge(allele_empty,
                                                  marker,
                                                  marker_idx,
                                                  allele_mask,
                                                  last, fm_index);
        Site site(1, variant_site);
        sites.push_back(site);
        sites.back().reserve(100);

        return;
    }

    assert(!sites.empty());
    assert(sites_it != sites.end());

    // there will be entries with pair.second empty (corresp to allele)
    // coming from crossing the last marker
    // can delete them here or in top a fcn when calculating coverages
    if (ignore) {
        if ((marker == last_begin + 1) && second_to_last) {
            auto vec_item = *(std::prev(sites.end(), 2));
            const auto variant_site = get_variant_site_edge(vec_item.back().second,
                                                            marker,
                                                            marker_idx,
                                                            allele_mask,
                                                            last,
                                                            fm_index);
            vec_item.back() = variant_site;
        } else {
            const auto variant_site = get_variant_site_edge(sites.back().back().second,
                                                            marker,
                                                            marker_idx,
                                                            allele_mask,
                                                            last,
                                                            fm_index);
            sites.back().back() = variant_site;
        }
        return;
    }

    *sa_interval_it = std::make_pair(left_new, right_new);

    VariantSite &k = (*sites_it).back();
    auto i = k.first;
    if (i == marker || i == marker - 1) {
        const auto variant_site = get_variant_site_edge((*sites_it).back().second,
                                                        marker,
                                                        marker_idx,
                                                        allele_mask,
                                                        last,
                                                        fm_index);
        (*sites_it).back() = variant_site;
    } else {
        std::vector<int> allele_empty;
        allele_empty.reserve(3500);
        const auto variant_site = get_variant_site_edge(allele_empty,
                                                        marker,
                                                        marker_idx,
                                                        allele_mask,
                                                        last,
                                                        fm_index);
        (*sites_it).emplace_back(variant_site);
    }
}
