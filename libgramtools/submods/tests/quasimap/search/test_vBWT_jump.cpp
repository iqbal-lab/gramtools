/**
 * @file
 * Unit tests for vBWT backward searching.
 * Terminology:
 *  - A variant locus is where you find variant **markers**;
 *  = pairs of site & allele markers.
 *  - A site 'entry' (resp. 'exit') is the 3' (resp. 5') part
 *  of a site in the linear PRG; because we are mapping backwards.
 *
 * Test suites:
 *  - VarSiteBSearch: backward searching with var site markers.
 *
 *  - MarkerSearch: checking finding and positioning variant markers in the PRG string
 *  - MarkerSAIntervals: Recovering SA Interval of variant markers.
 *
 *  - VariantLocus_Path: checking search recovers right variant site/allele combinations.
 *  - EndInLocus: checking when search ends inside variant locus.
 *  - StartEndInLocus: search starts and end inside VariantLocus.
 *
 *  - Search: test that is not sub-classified [TODO]
 */
#include <cctype>

#include "gtest/gtest.h"

#include "src_common/generate_prg.hpp"
#include "prg/prg.hpp"
#include "kmer_index/build.hpp"
#include "quasimap/quasimap.hpp"


using namespace gram;

/*
PRG: GCGCT5C6G6A6AGTCCT
i	BWT	SA	text_suffix
0	G	18
1	6	12	A G T C C T
2	6	10	A 6 A G T C C T
3	G	15	C C T
4	T	1	C G C T 5 C 6 G 6 A 6 A G T C C T
5	C	16	C T
6	T	3	C T 5 C 6 G 6 A 6 A G T C C T
7	5	6	C 6 G 6 A 6 A G T C C T
8	0	0	G C G C T 5 C 6 G 6 A 6 A G T C C T
9	C	2	G C T 5 C 6 G 6 A 6 A G T C C T
10	A	13	G T C C T
11	6	8	G 6 A 6 A G T C C T
12	C	17	T
13	T	14	T C C T
14	C	4	T 5 C 6 G 6 A 6 A G T C C T
15	G	5	5 C 6 G 6 A 6 A G T C C T
16	A	11	6 A G T C C T
17	T	9	6 A 6 A G T C C T
18	C	7	6 G 6 A 6 A G T C C T
*/

TEST(MarkerSearch, GivenCharA_FindLeftMarkers_AndSeedSearchStates){
    auto prg_raw = encode_prg("gcgct5c6g6a6agtcct");
    auto prg_info = generate_prg_info(prg_raw);
    // first char: a
    SearchState initial_search_state = {
            SA_Interval {1, 2}
    };

    auto result = left_markers_search(initial_search_state,
                                      prg_info);
    MarkersSearchResults expected = {
            {6, 0},
            {5, 3},
    };
    EXPECT_EQ(result, expected);

    // Expect three: one for exiting the site; two for entering.
    const auto &markers_search_states = search_state_vBWT_jumps(initial_search_state,
                                                                prg_info);
    EXPECT_EQ(markers_search_states.size(), 2);
}

// The convention is as follows: if the position marks a site exit, the marker will be a site marker,
// and if it marks a site entry, the marker will be an allele marker.
TEST(MarkerSearch, TestSiteMarker_Entry_or_Exit){
    auto prg_raw = encode_prg("gcgct5C6g6a6Agtcct");
    auto prg_info = generate_prg_info(prg_raw);

    const Marker marker_char = 6;

    // TEST 1: char a at site entry point
    SearchState search_state = {
            SA_Interval {1, 1}
    };

    auto result = left_markers_search(search_state,
                                      prg_info);

    auto variant_marker = result[0].first;
    EXPECT_TRUE(is_allele_marker(variant_marker));

    // TEST 2: char c at site exit point
    search_state = {
            SA_Interval{7, 7}
    };
    result = left_markers_search(search_state, prg_info);
    variant_marker = result[0].first;
    EXPECT_TRUE(is_site_marker(variant_marker));
}


TEST(MarkerSearch, GivenCharG_ReturnOneCorrectSearchResults) {
    auto prg_raw = encode_prg("gcgct5c6g6a6agtcct");
    auto prg_info = generate_prg_info(prg_raw);
    // first char: g
    SearchState initial_search_state = {
            SA_Interval {8, 11}
    };

    auto result = left_markers_search(initial_search_state,
                                      prg_info);
    MarkersSearchResults expected = {
            {5, 2},
    };
    EXPECT_EQ(result, expected);
}


TEST(MarkerJump, SingleCharAllele_CorrectSkipToSiteStartBoundaryMarker) {
    auto prg_raw = encode_prg("gcgct5c6g6a6agtcct");
    auto prg_info = generate_prg_info(prg_raw);
    // first char: g
    SearchState initial_search_state = {
            SA_Interval {8, 11}
    };
    const auto &markers_search_states = search_state_vBWT_jumps(initial_search_state,
                                                                prg_info);
    const auto &first_markers_search_state = markers_search_states.front();

    const auto &result = first_markers_search_state.sa_interval;
    SA_Interval expected = {15, 15};
    EXPECT_EQ(result, expected);
}


TEST(MarkerSearch, GivenCharG_NoMarkersToLeft){
    auto prg_raw = encode_prg("gcgct5c6g6a6agtcct");
    auto prg_info = generate_prg_info(prg_raw);
    // first char: g
    SearchState initial_search_state = {
            SA_Interval {8, 11}
    };
    const auto &markers_search_states = search_state_vBWT_jumps(initial_search_state,
                                                                prg_info);
    const auto &result = markers_search_states.size();
    auto expected = 1;
    EXPECT_EQ(result, expected);
}


TEST(MarkerSearch, GivenCharC_JumptoSiteStart) {
    auto prg_raw = encode_prg("gcgct5c6g6a6agtcct");
    auto prg_info = generate_prg_info(prg_raw);
    // first char: c
    SearchState initial_search_state = {
            SA_Interval {3, 7}
    };
    const auto &markers_search_states = search_state_vBWT_jumps(initial_search_state,
                                                                prg_info);
    const auto &first_markers_search_state = markers_search_states.front();

    EXPECT_EQ(markers_search_states.size(), 1);
    auto &result = first_markers_search_state.sa_interval;
    SA_Interval expected = {15, 15};
    EXPECT_EQ(result, expected);
}


TEST(MarkerSAIntervals, AlleleMarkerAnd3Alleles_correctSAInterval) {
    auto prg_raw = encode_prg("gcgct5c6g6a6agtcct");
    auto prg_info = generate_prg_info(prg_raw);
    Marker allele_marker = 6;

    auto result = get_allele_marker_sa_interval(allele_marker, prg_info);
    SA_Interval expected = {16, 18};
    EXPECT_EQ(result, expected);
}


TEST(MarkerSAIntervals,  AlleleMarkerAnd2Alleles_correctSAInterval) {
    auto prg_raw = encode_prg("aca5g6t6catt");
    auto prg_info = generate_prg_info(prg_raw);

    auto result = get_allele_marker_sa_interval(6, prg_info);
    SA_Interval expected = {11, 12};
    EXPECT_EQ(result, expected);
}


/*
PRG: 7G8C8G9T10A10
i	BWT	SA	text_suffix
0	10	11	1
1	10	9	0 A 1
2	8	3	C 8 G 9 T 1 0 A 1
3	7	1	G 8 C 8 G 9 T 1 0 A 1
4	8	5	G 9 T 1 0 A 1
5	9	7	T 1 0 A 1
6	0	0	7 G 8 C 8 G 9 T 1 0 A 1
7	G	2	8 C 8 G 9 T 1 0 A 1
8	C	4	8 G 9 T 1 0 A 1
9	G	6	9 T 1 0 A 1
10	A	10	A 1
11	T	8	1 0 A 1
*/
TEST(MarkerSAIntervals, GivenPrgWithNonContinuousAlphabet_CorrectAlleleMarkerEndBoundary) {
    auto prg_raw = encode_prg("7g8c8g9t10a10");
    auto prg_info = generate_prg_info(prg_raw);

    auto result = get_allele_marker_sa_interval(8, prg_info);
    SA_Interval expected = {7, 8};
    EXPECT_EQ(result, expected);
}


/*
PRG: GCGCT5C6G6T6AGTCCT
i	BWT	SA	text_suffix
0	T	18
1	6	12	A G T C C T
2	T	15	C C T
3	G	1	C G C T 5 C 6 G 6 T 6 A G T C C T
4	C	16	C T
5	G	3	C T 5 C 6 G 6 T 6 A G T C C T
6	5	6	C 6 G 6 T 6 A G T C C T
7	0	0	G C G C T 5 C 6 G 6 T 6 A G T C C T
8	C	2	G C T 5 C 6 G 6 T 6 A G T C C T
9	A	13	G T C C T
10	6	8	G 6 T 6 A G T C C T
11	C	17	T
12	G	14	T C C T
13	C	4	T 5 C 6 G 6 T 6 A G T C C T
14	6	10	T 6 A G T C C T
15	T	5	5 C 6 G 6 T 6 A G T C C T
16	T	11	6 A G T C C T
17	C	7	6 G 6 T 6 A G T C C T
18	G	9	6 T 6 A G T C C T

 */


TEST(MarkerSearch, AtSiteEntry_JumpTargetAlleles) {
    auto prg_raw = encode_prg("gcgct5c6g6t6agtcct");
    auto prg_info = generate_prg_info(prg_raw);

    // first char: a
    SearchState initial_search_state = {
            SA_Interval {1, 1}
    };
    const auto &markers_search_states = search_state_vBWT_jumps(initial_search_state,
                                                                prg_info);

    EXPECT_EQ(markers_search_states.size(), 1);

    std::vector<Marker> result;
    auto ss = markers_search_states.front().sa_interval;
    SA_Interval expected_sa{16,18};
    EXPECT_EQ(ss, expected_sa);

    for (int i = ss.first; i<=ss.second;i++){
        auto index = prg_info.fm_index[i];
       result.push_back(prg_info.encoded_prg[index]);
    }
    std::vector<Marker> expected = {6, 6, 6};
    EXPECT_EQ(result, expected);
}


TEST(VariantLocus_Path, AtSiteEntry_VariantPathOfAllAlleles) {
    auto prg_raw = encode_prg("gcgct5c6g6t6Agtcct");
    auto prg_info = generate_prg_info(prg_raw);

    // first char: a
    SearchState initial_search_state = {
            SA_Interval {1, 1}
    };
    const auto &markers_search_states = search_state_vBWT_jumps(initial_search_state,
                                                                prg_info);

    std::vector<VariantLocus> result;
    for (const auto &search_state: markers_search_states)
        result.push_back(search_state.traversing_path.front());

    std::vector<VariantLocus> expected = {
            {5, ALLELE_UNKNOWN},
    };
    EXPECT_EQ(result, expected);
}


TEST(ExitASite, ThirdAlleleSingleChar_SkipToSiteStartBoundaryMarker) {
    auto prg_raw = encode_prg("gcgct5c6g6t6agtcct");
    auto prg_info = generate_prg_info(prg_raw);

    // first char: t
    SearchState initial_search_state = {
            SA_Interval {11, 14}
    };
    const auto &markers_search_states = search_state_vBWT_jumps(initial_search_state,
                                                                prg_info);
    EXPECT_EQ(markers_search_states.size(), 1);
    auto result = markers_search_states.front();
    SearchState expected = {
            SA_Interval {15, 15},
            VariantSitePath {VariantLocus{5,3}},
            VariantSitePath {},
            SearchVariantSiteState::outside_variant_site,
    };
    EXPECT_EQ(result, expected);
}


TEST(ExitASite, SecondAlleleSingleChar_SkipToSiteStartBoundaryMarker) {
    auto prg_raw = encode_prg("gcgct5c6g6t6agtcct");
    auto prg_info = generate_prg_info(prg_raw);

    // first char: g
    SearchState initial_search_state = {
            SA_Interval {7, 10}
    };
    const auto &markers_search_states = search_state_vBWT_jumps(initial_search_state,
                                                                prg_info);
    EXPECT_EQ(markers_search_states.size(), 1);
    auto result = markers_search_states.front();
    SearchState expected = {
            SA_Interval {15, 15},
            VariantSitePath {VariantLocus{5, 2}},
            VariantSitePath {},
            SearchVariantSiteState::outside_variant_site,
    };
    EXPECT_EQ(result, expected);
}


TEST(ExitASite, FirstAlleleSingleChar_SkipToSiteStartBoundaryMarker) {
    auto prg_raw = encode_prg("gcgct5c6g6t6agtcct");
    auto prg_info = generate_prg_info(prg_raw);

    // first char: c
    SearchState initial_search_state = {
            SA_Interval {2, 6}
    };
    const auto &markers_search_states = search_state_vBWT_jumps(initial_search_state,
                                                                prg_info);
    EXPECT_EQ(markers_search_states.size(), 1);
    auto result = markers_search_states.front();
    SearchState expected = {
            SA_Interval {15, 15},
            VariantSitePath {VariantLocus{5,1}},
            VariantSitePath {},
            SearchVariantSiteState::outside_variant_site,
    };
    EXPECT_EQ(result, expected);
}


TEST(VarSiteBSearch, ReadCrossesTwoVarSites) {
    auto prg_raw = encode_prg("gct5c6g6T6AG7T8c8cta");
    auto prg_info = generate_prg_info(prg_raw);

    Pattern kmer = encode_dna_bases("tagt");
    Patterns kmers = {kmer};
    auto kmer_size = 4;
    auto kmer_index = index_kmers(kmers, kmer_size, prg_info);

    auto read = encode_dna_bases("cttagt");

    auto search_states = search_read_backwards(read, kmer, kmer_index, prg_info);
    EXPECT_EQ(search_states.size(), 1);

    const auto &search_state = search_states.front();
    auto result = search_state.traversed_path;
    VariantSitePath expected = {
            VariantLocus {7, 1},
            VariantLocus {5, 3},
    };
    EXPECT_EQ(result, expected);
}


TEST(StartEndInLocus, OneMappingEncapsulatedByAllele) {
    auto prg_raw = encode_prg("t5c6gCTTAGT6aa");
    auto prg_info = generate_prg_info(prg_raw);

    Pattern kmer = encode_dna_bases("tagt");
    Patterns kmers = {kmer};
    auto kmer_size = 4;
    auto kmer_index = index_kmers(kmers, kmer_size, prg_info);

    auto read = encode_dna_bases("cttagt");

    auto search_states = search_read_backwards(read, kmer, kmer_index, prg_info);
    EXPECT_EQ(search_states.size(), 1);

    const auto &search_state = search_states.front();
    auto result = search_state.variant_site_state;
    SearchVariantSiteState expected = SearchVariantSiteState::within_variant_site;
    EXPECT_EQ(result, expected);

    VariantLocus cov = {5, 2};
    EXPECT_EQ(search_state.traversed_path.front(), cov);
}


TEST(StartEndInLocus, TwoMappingsEncapsulatedByAllele_StateIsWithinVariantSite) {
    auto prg_raw = encode_prg("t5c6gcttagtacgcttagt6aa");
    auto prg_info = generate_prg_info(prg_raw);

    Pattern kmer = encode_dna_bases("tagt");
    Patterns kmers = {kmer};
    auto kmer_size = 4;
    auto kmer_index = index_kmers(kmers, kmer_size, prg_info);

    auto read = encode_dna_bases("cttagt");

    auto search_states = search_read_backwards(read, kmer, kmer_index, prg_info);
    EXPECT_EQ(search_states.size(), 1);

    const auto &search_state = search_states.front();
    auto result = search_state.variant_site_state;
    SearchVariantSiteState expected = SearchVariantSiteState::within_variant_site;
    EXPECT_EQ(result, expected);
}


