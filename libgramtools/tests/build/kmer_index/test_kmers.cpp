#include "gtest/gtest.h"

#include "build/kmer_index/kmers.hpp"
#include "submod_resources.hpp"

using namespace gram::submods;

TEST(GetBoundaryMarkerIndexes, TwoVariantSites_CorrectSiteStartEndIndexes) {
  auto prg_raw = encode_prg("aca5g6c6tt7a8c8gg");
  auto prg_info = generate_prg_info(prg_raw);

  auto result = get_boundary_marker_indexes(prg_info);
  std::vector<PrgIndexRange> expected = {
      {3, 7},
      {10, 14},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetBoundaryMarkerIndexes, OneVariantSites_CorrectSiteStartEndIndexes) {
  auto prg_raw = encode_prg("acagctt7a8c8gg");
  auto prg_info = generate_prg_info(prg_raw);

  auto result = get_boundary_marker_indexes(prg_info);
  std::vector<PrgIndexRange> expected = {{7, 11}};
  EXPECT_EQ(result, expected);
}

TEST(GetBoundaryMarkerIndexes, NoVariantSites_NoSiteIndexes) {
  auto prg_raw = encode_prg("acagcttagg");
  auto prg_info = generate_prg_info(prg_raw);

  auto result = get_boundary_marker_indexes(prg_info);
  std::vector<PrgIndexRange> expected = {};
  EXPECT_EQ(result, expected);
}

TEST(GetKmerRegionRange, VariantSiteCloseToStart_CorrectKmerRegionEndIndexes) {
  auto prg_raw = encode_prg("t7a8c8acagctt");
  auto prg_info = generate_prg_info(prg_raw);

  auto end_site_marker_indexes = get_boundary_marker_indexes(prg_info);
  uint64_t max_read_size = 5;
  auto result =
      get_kmer_region_ranges(end_site_marker_indexes, max_read_size, prg_info);
  std::vector<PrgIndexRange> expected = {
      {1, 9},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetKmerRegionRange, VariantSiteCloseToEnd_CorrectKmerRegionEndIndexes) {
  auto prg_raw = encode_prg("cagcttt7a8c8acg");
  auto prg_info = generate_prg_info(prg_raw);

  auto end_site_marker_indexes = get_boundary_marker_indexes(prg_info);
  uint64_t max_read_size = 150;
  auto result =
      get_kmer_region_ranges(end_site_marker_indexes, max_read_size, prg_info);
  std::vector<PrgIndexRange> expected = {{7, 14}};
  EXPECT_EQ(result, expected);
}

TEST(GetKmerRegionRange,
     TwoVariantSites_FirstKmerRegionExtendedToBoundaryEndOfSecond) {
  auto prg_raw = encode_prg("tt5a6c6a7aa8cc8t");
  auto prg_info = generate_prg_info(prg_raw);

  auto end_site_marker_indexes = get_boundary_marker_indexes(prg_info);
  uint64_t max_read_size = 4;
  auto result =
      get_kmer_region_ranges(end_site_marker_indexes, max_read_size, prg_info);
  std::vector<PrgIndexRange> expected = {
      {2, 14},
      {8, 15},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetKmerRegionRange, GivenMaxReadSizeOne_RangeEndAtSiteBoundaryEnd) {
  auto prg_raw = encode_prg("ta5g6a6acgt");
  auto prg_info = generate_prg_info(prg_raw);

  auto end_site_marker_indexes = get_boundary_marker_indexes(prg_info);
  uint64_t max_read_size = 1;
  auto result =
      get_kmer_region_ranges(end_site_marker_indexes, max_read_size, prg_info);
  std::vector<PrgIndexRange> expected = {
      {2, 6},
  };
  EXPECT_EQ(result, expected);
}

TEST(FindSiteEndBoundary, GivenAlleleIndex_ReturnSiteEndMarkerIndex) {
  auto prg_raw = encode_prg("t7a8c8acagctt");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t within_site_index = 2;
  auto result = find_site_end_boundary(within_site_index, prg_info);
  uint64_t expected = 5;
  EXPECT_EQ(result, expected);
}

TEST(FindSiteEndBoundary,
     GivenAlleleIndexAndSiteEndingPrg_ReturnSiteEndMarkerIndex) {
  auto prg_raw = encode_prg("t7a8c8");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t within_site_index = 2;
  auto result = find_site_end_boundary(within_site_index, prg_info);
  uint64_t expected = 5;
  EXPECT_EQ(result, expected);
}

TEST(FindSiteEndBoundary, GivenMultiCharAllele_ReturnSiteEndMarkerIndex) {
  auto prg_raw = encode_prg("t7a8cacag8acag");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t within_site_index = 5;
  auto result = find_site_end_boundary(within_site_index, prg_info);
  uint64_t expected = 9;
  EXPECT_EQ(result, expected);
}

TEST(FindSiteEndBoundary, GivenAlleleMarkerIndex_ReturnSiteEndMarkerIndex) {
  auto prg_raw = encode_prg("t7a8cacag8acag");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t within_site_index = 3;
  auto result = find_site_end_boundary(within_site_index, prg_info);
  uint64_t expected = 9;
  EXPECT_EQ(result, expected);
}

TEST(FindSiteEndBoundary,
     GivenStartBoundaryMarkerIndex_ReturnEndBoundaryMarkerIndex) {
  auto prg_raw = encode_prg("t7a8cacag8acag");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t within_site_index = 1;
  auto result = find_site_end_boundary(within_site_index, prg_info);
  uint64_t expected = 9;
  EXPECT_EQ(result, expected);
}

TEST(FindSiteEndBoundary,
     GivenSiteEndingAtPrgEnd_ReturnCorrectEndBoundaryMarkerIndex) {
  auto prg_raw = encode_prg("t7a8cacag8");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t within_site_index = 1;
  auto result = find_site_end_boundary(within_site_index, prg_info);
  uint64_t expected = 9;
  EXPECT_EQ(result, expected);
}

TEST(FindSiteEndBoundary,
     GivenEndBoundaryMarkerIndex_ReturnEndBoundaryMarkerIndex) {
  auto prg_raw = encode_prg("t7a8cacag8acag");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t within_site_index = 9;
  auto result = find_site_end_boundary(within_site_index, prg_info);
  uint64_t expected = 9;
  EXPECT_EQ(result, expected);
}

TEST(GetSiteOrderedAlleles,
     GivenSiteWithMultiCharAlleles_CorrectAllelesExtracted) {
  auto prg_raw = encode_prg("tt5ga6ct6a");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t within_site_index = 2;
  auto result = get_site_ordered_alleles(within_site_index, prg_info);
  Sequences expected = {
      {3, 1},
      {2, 4},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetSiteOrderedAlleles,
     GivenBondaryEndMarkerIndex_CorrectAllelesExtracted) {
  auto prg_raw = encode_prg("tt5ga6ct6a");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t within_site_index = 8;
  auto result = get_site_ordered_alleles(within_site_index, prg_info);
  Sequences expected = {
      {3, 1},
      {2, 4},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetSiteOrderedAlleles,
     GivenSiteWithSingleCharAllele_CorrectAllelesExtracted) {
  auto prg_raw = encode_prg("tt5g6ct6a");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t within_site_index = 2;
  auto result = get_site_ordered_alleles(within_site_index, prg_info);
  Sequences expected = {
      {3},
      {2, 4},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetSiteOrderedAlleles, GivenSiteWithThreeAlleles_CorrectAllelesExtracted) {
  auto prg_raw = encode_prg("tt5g6ct6aaa6a");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t within_site_index = 2;
  auto result = get_site_ordered_alleles(within_site_index, prg_info);
  Sequences expected = {
      {3},
      {2, 4},
      {1, 1, 1},
  };
  EXPECT_EQ(result, expected);
}

TEST(InrangeLeftSites, NoSitesWithinRange_NoSiteEndIndexesReturned) {
  auto prg_raw = encode_prg("taagaact");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t outside_site_start_index = 7;
  uint64_t kmer_size = 5;
  auto result =
      sites_inrange_left(outside_site_start_index, kmer_size, prg_info);
  std::list<uint64_t> expected = {};
  EXPECT_EQ(result, expected);
}

TEST(InrangeLeftSites, SiteOutsideKmerSize_NoSiteEndIndexesReturned) {
  auto prg_raw = encode_prg("t5g6a6act");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t outside_site_start_index = 8;
  uint64_t kmer_size = 3;
  auto result =
      sites_inrange_left(outside_site_start_index, kmer_size, prg_info);
  std::list<uint64_t> expected = {};
  EXPECT_EQ(result, expected);
}

TEST(InrangeLeftSites,
     SiteStartIndexAtBoundaryEnd_SiteRecognizeBoundaryIndexReturned) {
  auto prg_raw = encode_prg("t5g6a6act");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t outside_site_start_index = 5;
  uint64_t kmer_size = 3;
  auto result =
      sites_inrange_left(outside_site_start_index, kmer_size, prg_info);
  std::list<uint64_t> expected = {5};
  EXPECT_EQ(result, expected);
}

TEST(InrangeLeftSites, SiteJustInsideKmerSize_SiteEndIndexReturned) {
  auto prg_raw = encode_prg("t5g6a6act");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t outside_site_start_index = 8;
  uint64_t kmer_size = 4;
  auto result =
      sites_inrange_left(outside_site_start_index, kmer_size, prg_info);
  std::list<uint64_t> expected = {5};
  EXPECT_EQ(result, expected);
}

TEST(InrangeLeftSites, KmerExtendsToFirstSiteMarker_SiteEndIndexReturned) {
  auto prg_raw = encode_prg("t7g8a8act");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t outside_site_start_index = 8;
  uint64_t kmer_size = 8;
  auto result =
      sites_inrange_left(outside_site_start_index, kmer_size, prg_info);
  std::list<uint64_t> expected = {5};
  EXPECT_EQ(result, expected);
}

TEST(InrangeLeftSites, KmerExtendsBeyondSite_SiteEndIndexReturned) {
  auto prg_raw = encode_prg("tgag7g8a8act");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t outside_site_start_index = 11;
  uint64_t kmer_size = 10;
  auto result =
      sites_inrange_left(outside_site_start_index, kmer_size, prg_info);
  std::list<uint64_t> expected = {8};
  EXPECT_EQ(result, expected);
}

TEST(InrangeLeftSites, KmerCoversMultipleSites_SiteEndIndexesReturned) {
  auto prg_raw = encode_prg("ta5g6a6act7g8aa8act");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t outside_site_start_index = 18;
  uint64_t kmer_size = 17;
  auto result =
      sites_inrange_left(outside_site_start_index, kmer_size, prg_info);
  std::list<uint64_t> expected = {6, 15};
  EXPECT_EQ(result, expected);
}

TEST(InrangeLeftSites,
     KmerCoverageEndsBeforeFirstSite_OnlySecondSiteEndIndexReturned) {
  auto prg_raw = encode_prg("ta5g6a6ct7g8aa8ac");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t outside_site_start_index = 16;
  uint64_t kmer_size = 5;
  auto result =
      sites_inrange_left(outside_site_start_index, kmer_size, prg_info);
  std::list<uint64_t> expected = {14};
  EXPECT_EQ(result, expected);
}

TEST(InrangeLeftSites,
     KmerCoverageExtendsJustWithinFirstSite_SiteEndIndexesReturned) {
  auto prg_raw = encode_prg("ta5g6a6ct7g8aa8ac");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t outside_site_start_index = 16;
  uint64_t kmer_size = 6;
  auto result =
      sites_inrange_left(outside_site_start_index, kmer_size, prg_info);
  std::list<uint64_t> expected = {6, 14};
  EXPECT_EQ(result, expected);
}

TEST(InrangeLeftSites,
     SecondSiteAlleleLengthsNotLimitKmerCoverage_BothSiteEndIndexesReturned) {
  auto prg_raw = encode_prg("ta5g6a6ct7gg8aa8ac");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t outside_site_start_index = 17;
  uint64_t kmer_size = 6;
  auto result =
      sites_inrange_left(outside_site_start_index, kmer_size, prg_info);
  std::list<uint64_t> expected = {6, 15};
  EXPECT_EQ(result, expected);
}

TEST(GetNonvariantRegion,
     GivenFirstSiteEndBoundaryIndex_ReturnRegionInclusiveRange) {
  auto prg_raw = encode_prg("ta5g6a6ct7gg8aa8ac");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t site_end_boundary_index = 6;
  auto result = get_nonvariant_region(site_end_boundary_index, prg_info);
  std::pair<uint64_t, uint64_t> expected = {7, 8};
  EXPECT_EQ(result, expected);
}

TEST(GetNonvariantRegion,
     GivenLastSiteEndBoundaryIndex_ReturnRegionInclusiveRange) {
  auto prg_raw = encode_prg("ta5g6a6ct7gg8aa8acc");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t site_end_boundary_index = 15;
  auto result = get_nonvariant_region(site_end_boundary_index, prg_info);
  std::pair<uint64_t, uint64_t> expected = {16, 18};
  EXPECT_EQ(result, expected);
}

TEST(GetNonvariantRegion, GivenSiteEndBoundaryIndexEndingPrg_ReturnZeroRange) {
  auto prg_raw = encode_prg("ta5g6a6");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t site_end_boundary_index = 6;
  auto result = get_nonvariant_region(site_end_boundary_index, prg_info);
  std::pair<uint64_t, uint64_t> expected = {0, 0};
  EXPECT_EQ(result, expected);
}

TEST(GetNonvariantRegion,
     GivenSiteEndBoundaryIndexJustBeforePrgEnd_ReturnRegionInclusiveRange) {
  auto prg_raw = encode_prg("ta5g6a6a");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t site_end_boundary_index = 6;
  auto result = get_nonvariant_region(site_end_boundary_index, prg_info);
  std::pair<uint64_t, uint64_t> expected = {7, 7};
  EXPECT_EQ(result, expected);
}

TEST(ExtractRightNonvariantRegion,
     GivenSiteEndBoundaryIndexBeforePrgEnd_CorrectNonvariantRegion) {
  auto prg_raw = encode_prg("ta5g6a6acgt");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t site_end_boundary_index = 6;
  auto result =
      right_intersite_nonvariant_region(site_end_boundary_index, prg_info);
  std::vector<int_Base> expected = {1, 2, 3, 4};
  EXPECT_EQ(result, expected);
}

TEST(ExtractRightNonvariantRegion,
     GivenSiteEndBoundaryIndexJustBeforePrgEnd_CorrectNonvariantRegion) {
  auto prg_raw = encode_prg("ta5g6a6a");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t site_end_boundary_index = 6;
  auto result =
      right_intersite_nonvariant_region(site_end_boundary_index, prg_info);
  std::vector<int_Base> expected = {1};
  EXPECT_EQ(result, expected);
}

TEST(ExtractRightNonvariantRegion,
     GivenSiteEndBoundaryIndexBeforeSecondSite_CorrectNonvariantRegion) {
  auto prg_raw = encode_prg("ta5g6a6acg7gg8aa8");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t site_end_boundary_index = 6;
  auto result =
      right_intersite_nonvariant_region(site_end_boundary_index, prg_info);
  std::vector<int_Base> expected = {1, 2, 3};
  EXPECT_EQ(result, expected);
}

TEST(ExtractRightNonvariantRegion,
     GivenSingleBaseNonvariantRegion_CorrectNonvariantRegion) {
  auto prg_raw = encode_prg("ta5g6a6g7gg8aa8");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t site_end_boundary_index = 6;
  auto result =
      right_intersite_nonvariant_region(site_end_boundary_index, prg_info);
  std::vector<int_Base> expected = {3};
  EXPECT_EQ(result, expected);
}

TEST(GetReverseKmersFromRegion, NoVariantSite_CorrectReverseKmers) {
  auto prg_raw = encode_prg("tagagcggaa");
  auto prg_info = generate_prg_info(prg_raw);

  PrgIndexRange kmer_region_range = {5, 7};
  uint64_t kmer_size = 3;
  auto result =
      get_region_range_reverse_kmers(kmer_region_range, kmer_size, prg_info);
  unordered_vector_set<std::vector<int_Base>> expected = {
      {3, 3, 2},
      {3, 2, 3},
      {2, 3, 1},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetReverseKmersFromRegion,
     KmerSizeKmerRangeStartsAtIndexZero_CorrectReverseKmer) {
  auto prg_raw = encode_prg("tagagcggaa");
  auto prg_info = generate_prg_info(prg_raw);

  PrgIndexRange kmer_region_range = {0, 2};
  uint64_t kmer_size = 3;
  auto result =
      get_region_range_reverse_kmers(kmer_region_range, kmer_size, prg_info);
  unordered_vector_set<std::vector<int_Base>> expected = {
      {3, 1, 4},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetReverseKmersFromRegion,
     NoVariantSite_FourCorrectReverseKmersFromPrgEnd) {
  auto prg_raw = encode_prg("tagagcggaa");
  auto prg_info = generate_prg_info(prg_raw);

  PrgIndexRange kmer_region_range = {6, 9};
  uint64_t kmer_size = 3;
  auto result =
      get_region_range_reverse_kmers(kmer_region_range, kmer_size, prg_info);
  unordered_vector_set<std::vector<int_Base>> expected = {
      {1, 1, 3},
      {1, 3, 3},
      {3, 3, 2},
      {3, 2, 3},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetReverseKmersFromRegion, GivenKmerRegionRange_CorrectReverseKmers) {
  //                2   6   10
  auto prg_raw = encode_prg("ta5g6a6acgt");
  auto prg_info = generate_prg_info(prg_raw);

  PrgIndexRange kmer_region_range = {0, 10};
  uint64_t kmer_size = 3;
  auto result =
      get_region_range_reverse_kmers(kmer_region_range, kmer_size, prg_info);
  unordered_vector_set<Sequence> expected = {
      {3, 1, 4}, {1, 1, 4}, {1, 3, 1}, {1, 1, 1},
      {4, 3, 2}, {3, 2, 1}, {2, 1, 1}, {2, 1, 3},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetReverseKmersFromRegion, GivenKmerRegion_CorrectReverseKmerFound) {
  // kmer:         |                         |
  auto prg_raw = encode_prg("atggaacggct5cg6cc6tg6tc6cg7g8a8tccccgacgat");
  auto prg_info = generate_prg_info(prg_raw);

  BuildParams parameters = {};
  parameters.kmers_size = 15;
  parameters.max_read_size = 150;
  PrgIndexRange kmer_region_range = {11, 41};
  auto reverse_kmers = get_region_range_reverse_kmers(
      kmer_region_range, parameters.kmers_size, prg_info);
  Sequence expected_reverse_kmer = {3, 3, 2, 3, 2, 4, 2, 3,
                                    3, 2, 1, 1, 3, 3, 4};
  auto result =
      reverse_kmers.find(expected_reverse_kmer) != reverse_kmers.end();
  EXPECT_TRUE(result);
}

TEST(FindSiteStartBoundary, GivenSiteEndIndex_CorrectSiteStartIndex) {
  //                       9    15
  auto prg_raw = encode_prg("ta5g6a6ga7gg8aa8cgt");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t end_boundary_index = 15;
  auto result = find_site_start_boundary(end_boundary_index, prg_info);
  uint64_t expected = 9;
  EXPECT_EQ(result, expected);
}

TEST(GetKmerSizeRegionParts, TwoSitesInRange_CorrectRegionParts) {
  //                    6       15  18
  auto prg_raw = encode_prg("ta5g6a6ga7gg8aa8cgt");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t current_range_end_index = 18;
  std::list<uint64_t> inrange_sites = {6, 15};
  uint64_t kmer_size = 3;
  auto result = get_kmer_size_region_parts(current_range_end_index,
                                           inrange_sites, kmer_size, prg_info);
  std::list<Sequences> expected = {
      {{4, 1}}, {{3}, {1}}, {{3, 1}}, {{3, 3}, {1, 1}}, {{2, 3, 4}},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetKmerSizeRegionParts,
     NonVariantTailAfterLastSite_TailIncludedAsRegionPart) {
  //                    6       15  18
  auto prg_raw = encode_prg("ta5g6a6ga7gg8aa8cgt");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t current_range_end_index = 8;
  std::list<uint64_t> inrange_sites = {6};
  uint64_t kmer_size = 5;
  auto result = get_kmer_size_region_parts(current_range_end_index,
                                           inrange_sites, kmer_size, prg_info);
  std::list<Sequences> expected = {
      {{4, 1}}, {{3}, {1}}, {{3, 1}}, {{3, 3}, {1, 1}}, {{2, 3, 4}},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetKmerSizeRegionParts,
     TwoSitesInRangeEndRegionAtSiteEnd_CorrectRegionParts) {
  //                    6       15
  auto prg_raw = encode_prg("ta5g6a6ga7gg8aa8");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t current_range_end_index = 15;
  std::list<uint64_t> inrange_sites = {6, 15};
  uint64_t kmer_size = 3;
  auto result = get_kmer_size_region_parts(current_range_end_index,
                                           inrange_sites, kmer_size, prg_info);
  std::list<Sequences> expected = {
      {{4, 1}},
      {{3}, {1}},
      {{3, 1}},
      {{3, 3}, {1, 1}},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetKmerSizeRegionParts,
     TwoSitesInRangeSingleCharAfterSiteEnd_CorrectRegionParts) {
  //                    6        15
  auto prg_raw = encode_prg("ta5g6a6ga7gg8aa8a");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t current_range_end_index = 16;
  std::list<uint64_t> inrange_sites = {6, 15};
  uint64_t kmer_size = 3;
  auto result = get_kmer_size_region_parts(current_range_end_index,
                                           inrange_sites, kmer_size, prg_info);
  std::list<Sequences> expected = {
      {{4, 1}}, {{3}, {1}}, {{3, 1}}, {{3, 3}, {1, 1}}, {{1}},
  };
  EXPECT_EQ(result, expected);
}

TEST(UpdateAlleleIndePath, GivenAllZerosAlleleIndexPath_LastIndexIncremented) {
  std::vector<uint64_t> allele_current_index = {0, 0, 0};
  std::vector<uint64_t> allele_counts = {2, 1, 2};

  update_allele_index_path(allele_current_index, allele_counts);
  auto result = allele_current_index;
  std::vector<uint64_t> expected = {0, 0, 1};
  EXPECT_EQ(result, expected);
}

TEST(UpdateAlleleIndePath, GivenAlleleIndexPath_FirstIndexIncremented) {
  std::vector<uint64_t> allele_current_index = {0, 0, 1};
  std::vector<uint64_t> allele_counts = {2, 1, 2};

  update_allele_index_path(allele_current_index, allele_counts);
  auto result = allele_current_index;
  std::vector<uint64_t> expected = {1, 0, 0};
  EXPECT_EQ(result, expected);
}

TEST(UpdateAlleleIndePath, GivenAlleleIndexPath_LastIndexIncremented) {
  std::vector<uint64_t> allele_current_index = {1, 0, 0};
  std::vector<uint64_t> allele_counts = {2, 1, 2};

  update_allele_index_path(allele_current_index, allele_counts);
  auto result = allele_current_index;
  std::vector<uint64_t> expected = {1, 0, 1};
  EXPECT_EQ(result, expected);
}

TEST(UpdateAlleleIndePath, ThreeAllelesInLastPlace_LastIndexIncremented) {
  std::vector<uint64_t> allele_current_index = {1, 0, 1};
  std::vector<uint64_t> allele_counts = {2, 1, 3};

  update_allele_index_path(allele_current_index, allele_counts);
  auto result = allele_current_index;
  std::vector<uint64_t> expected = {1, 0, 2};
  EXPECT_EQ(result, expected);
}

TEST(UpdateAlleleIndePath, ThreeAllelesInMidPlace_MidIndexIncremented) {
  std::vector<uint64_t> allele_current_index = {1, 0, 2};
  std::vector<uint64_t> allele_counts = {2, 2, 3};

  update_allele_index_path(allele_current_index, allele_counts);
  auto result = allele_current_index;
  std::vector<uint64_t> expected = {1, 1, 0};
  EXPECT_EQ(result, expected);
}

TEST(UpdateAlleleIndePath, CantUpdateFurther_ReturnFalse) {
  std::vector<uint64_t> allele_current_index = {1, 1, 2};
  std::vector<uint64_t> allele_counts = {2, 2, 3};

  auto result = update_allele_index_path(allele_current_index, allele_counts);
  EXPECT_FALSE(result);
}

TEST(GetRegionPartsReverseKmers, GivenKmerSizeRegionParts_CorrectReverseKmers) {
  std::list<Sequences> region_parts = {
      {{3}, {1}},
      {{3, 1}},
      {{2}, {4}},
  };
  uint64_t kmer_size = 4;

  auto result = get_region_parts_reverse_kmers(region_parts, kmer_size);
  unordered_vector_set<Sequence> expected = {
      {4, 1, 3, 1},
      {2, 1, 3, 1},
      {4, 1, 3, 3},
      {2, 1, 3, 3},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetRegionPartsReverseKmers,
     GivenThreeCharAlleleInLastRegion_CorrectReverseKmers) {
  std::list<Sequences> region_parts = {
      {{3}, {1}},
      {{3, 1}},
      {{2}, {4, 4, 2}},
  };
  uint64_t kmer_size = 4;

  auto result = get_region_parts_reverse_kmers(region_parts, kmer_size);
  unordered_vector_set<Sequence> expected = {
      {4, 1, 3, 1}, {4, 1, 3, 3}, {2, 1, 3, 3},
      {2, 1, 3, 1}, {4, 4, 1, 3}, {2, 4, 4, 1},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetRegionPartsReverseKmers,
     MidRegionContainsTwoAlleles_CorrectReverseKmers) {
  std::list<Sequences> region_parts = {
      {{3}, {1}},
      {{3, 1}, {2}},
      {{2}},
  };
  uint64_t kmer_size = 4;

  auto result = get_region_parts_reverse_kmers(region_parts, kmer_size);
  unordered_vector_set<Sequence> expected = {
      {2, 1, 3, 3},
      {2, 1, 3, 1},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetRegionPartsReverseKmers,
     MidRegionContainsThreeAlleles_CorrectReverseKmers) {
  std::list<Sequences> region_parts = {
      {{3}, {1}},
      {{3, 1}, {2, 3}, {1}},
      {{2}},
  };
  uint64_t kmer_size = 4;

  auto result = get_region_parts_reverse_kmers(region_parts, kmer_size);
  unordered_vector_set<Sequence> expected = {
      {2, 1, 3, 3},
      {2, 1, 3, 1},
      {2, 3, 2, 3},
      {2, 3, 2, 1},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetRegionPartsReverseKmers,
     SingleRegionWithSingleCharAllele_NoReverseKmer) {
  std::list<Sequences> region_parts = {
      {{3}},
  };
  uint64_t kmer_size = 4;

  auto result = get_region_parts_reverse_kmers(region_parts, kmer_size);
  unordered_vector_set<Sequence> expected = {};
  EXPECT_EQ(result, expected);
}

TEST(GetPathReverseKmers, GivenPath_CorrectReverseKmers) {
  Sequence path = {3, 3, 1, 2};
  uint64_t kmer_size = 3;
  auto result = get_path_reverse_kmers(path, kmer_size);
  unordered_vector_set<Sequence> expected = {
      {2, 1, 3},
      {1, 3, 3},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetPathReverseKmers, GivenTooShortPath_NoKmers) {
  Sequence path = {3, 3, 1};
  uint64_t kmer_size = 4;
  auto result = get_path_reverse_kmers(path, kmer_size);
  unordered_vector_set<Sequence> expected = {};
  EXPECT_EQ(result, expected);
}

TEST(GetPathReverseKmers, GivenKmerSizePath_CorrectReverseKmer) {
  Sequence path = {3, 3, 1};
  uint64_t kmer_size = 3;
  auto result = get_path_reverse_kmers(path, kmer_size);
  unordered_vector_set<Sequence> expected = {
      {1, 3, 3},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetPathReverseKmers, GivenPath_CorrectReverseKmerExtracted) {
  Sequence path = {1, 4, 3, 3, 1, 1, 2, 3, 3, 2, 4, 2, 3, 2,
                   3, 3, 4, 2, 2, 2, 2, 3, 1, 2, 3, 1, 4};
  uint64_t kmer_size = 15;
  auto reverse_kmers = get_path_reverse_kmers(path, kmer_size);
  Sequence expected_reverse_kmer = {3, 3, 2, 3, 2, 4, 2, 3,
                                    3, 2, 1, 1, 3, 3, 4};
  auto result =
      reverse_kmers.find(expected_reverse_kmer) != reverse_kmers.end();
  EXPECT_TRUE(result);
}

TEST(ExtractVariantReverseKmers, GivenInrangeSite_CorrectReverseKmers) {
  //                2   6   10
  auto prg_raw = encode_prg("ta5g6a6acgt");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t current_index = 10;
  std::list<uint64_t> inrange_sites = {6};
  uint64_t kmer_size = 3;
  auto result = get_sites_reverse_kmers(current_index, inrange_sites, kmer_size,
                                        prg_info);
  unordered_vector_set<Sequence> expected = {
      {3, 1, 4}, {1, 1, 4}, {1, 3, 1}, {1, 1, 1},
      {4, 3, 2}, {3, 2, 1}, {2, 1, 1}, {2, 1, 3},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetSitesReverseKmers, SingleSiteInRange_CorrectReverseKmers) {
  //                2   6   10
  auto prg_raw = encode_prg("ta5g6a6acgt");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t current_index = 10;
  std::list<uint64_t> inrange_sites = {6};
  uint64_t kmer_size = 3;
  auto result = get_sites_reverse_kmers(current_index, inrange_sites, kmer_size,
                                        prg_info);
  unordered_vector_set<Sequence> expected = {
      {3, 1, 4}, {1, 1, 4}, {1, 3, 1}, {1, 1, 1},
      {4, 3, 2}, {3, 2, 1}, {2, 1, 1}, {2, 1, 3},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetSitesReverseKmers, SiteStartsAtPrgStart_CorrectReverseKmers) {
  auto prg_raw = encode_prg("5g6a6acgt");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t current_index = 8;
  std::list<uint64_t> inrange_sites = {4};
  uint64_t kmer_size = 3;
  auto result = get_sites_reverse_kmers(current_index, inrange_sites, kmer_size,
                                        prg_info);
  unordered_vector_set<Sequence> expected = {
      {4, 3, 2},
      {3, 2, 1},
      {2, 1, 1},
      {2, 1, 3},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetSitesReverseKmers, SiteEndsAtPrgEnd_CorrectReverseKmers) {
  auto prg_raw = encode_prg("acgt5c6a6");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t current_index = 8;
  std::list<uint64_t> inrange_sites = {8};
  uint64_t kmer_size = 3;
  auto result = get_sites_reverse_kmers(current_index, inrange_sites, kmer_size,
                                        prg_info);
  unordered_vector_set<Sequence> expected = {
      {1, 4, 3},
      {2, 4, 3},
      {4, 3, 2},
      {3, 2, 1},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetSitesReverseKmers, SingleSiteMultiCharAllele_CorrectReverseKmers) {
  auto prg_raw = encode_prg("acgt5cc6a6");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t current_index = 9;
  std::list<uint64_t> inrange_sites = {9};
  uint64_t kmer_size = 3;
  auto result = get_sites_reverse_kmers(current_index, inrange_sites, kmer_size,
                                        prg_info);
  unordered_vector_set<Sequence> expected = {
      {1, 4, 3}, {2, 4, 3}, {2, 2, 4}, {4, 3, 2}, {3, 2, 1},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetSitesReverseKmers, TwoSitesNoCrossingKmers_CorrectReverseKmers) {
  auto prg_raw = encode_prg("gt5c6a6tt7g8a8");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t current_index = 13;
  std::list<uint64_t> inrange_sites = {6, 13};
  uint64_t kmer_size = 3;
  auto result = get_sites_reverse_kmers(current_index, inrange_sites, kmer_size,
                                        prg_info);
  unordered_vector_set<Sequence> expected = {
      {1, 4, 4}, {3, 4, 4}, {4, 4, 1}, {4, 4, 2},
      {4, 1, 4}, {4, 2, 4}, {1, 4, 3}, {2, 4, 3},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetSitesReverseKmers, TwoSitesWithCrossingKmers_CorrectReverseKmers) {
  auto prg_raw = encode_prg("5c6a6t7g8a8");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t current_index = 10;
  std::list<uint64_t> inrange_sites = {4, 10};
  uint64_t kmer_size = 3;
  auto result = get_sites_reverse_kmers(current_index, inrange_sites, kmer_size,
                                        prg_info);
  unordered_vector_set<Sequence> expected = {
      {1, 4, 1},
      {3, 4, 1},
      {1, 4, 2},
      {3, 4, 2},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetSitesReverseKmers, SingleSiteSingleKmerFromAllele_CorrectReverseKmer) {
  auto prg_raw = encode_prg("5c6atg6");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t current_index = 6;
  std::list<uint64_t> inrange_sites = {6};
  uint64_t kmer_size = 3;
  auto result = get_sites_reverse_kmers(current_index, inrange_sites, kmer_size,
                                        prg_info);
  unordered_vector_set<Sequence> expected = {
      {3, 4, 1},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetSitesReverseKmers, SingleSiteTwoKmersFromAllele_CorrectReverseKmer) {
  auto prg_raw = encode_prg("5c6atgc6");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t current_index = 6;
  std::list<uint64_t> inrange_sites = {7};
  uint64_t kmer_size = 3;
  auto result = get_sites_reverse_kmers(current_index, inrange_sites, kmer_size,
                                        prg_info);
  unordered_vector_set<Sequence> expected = {
      {2, 3, 4},
      {3, 4, 1},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetSitesReverseKmers, GivenInrangeSite_CorrectNewCurrentIndex) {
  //                2   6   10
  auto prg_raw = encode_prg("ta5g6a6acgt");
  auto prg_info = generate_prg_info(prg_raw);

  uint64_t current_index = 10;
  std::list<uint64_t> inrange_sites = {6};
  uint64_t kmer_size = 3;
  get_sites_reverse_kmers(current_index, inrange_sites, kmer_size, prg_info);
  uint64_t result = current_index;
  uint64_t expected = 1;
  EXPECT_EQ(result, expected);
}

TEST(CombineOverlappingRegions,
     SetOfRangesAllEncapsulatedWithinFirstRange_CorrectSingleRange) {
  std::vector<PrgIndexRange> kmer_region_ranges = {
      {1, 6},
      {3, 4},
      {2, 4},
      {2, 3},
  };

  auto result = combine_overlapping_regions(kmer_region_ranges);
  std::vector<PrgIndexRange> expected = {
      {1, 6},
  };
  EXPECT_EQ(result, expected);
}

TEST(CombineOverlappingRegions,
     ExactlyTwoNonOverlappingRanges_CorrectTwoRanges) {
  std::vector<PrgIndexRange> kmer_region_ranges = {
      {1, 6},
      {3, 7},
      {8, 9},
      {2, 3},
  };

  auto result = combine_overlapping_regions(kmer_region_ranges);
  std::vector<PrgIndexRange> expected = {
      {1, 7},
      {8, 9},
  };
  EXPECT_EQ(result, expected);
}

TEST(CombineOverlappingRegions, TwoRangesEqualEndStart_CorrectRange) {
  std::vector<PrgIndexRange> kmer_region_ranges = {
      {2, 3},
      {3, 4},
  };

  auto result = combine_overlapping_regions(kmer_region_ranges);
  std::vector<PrgIndexRange> expected = {
      {2, 4},
  };
  EXPECT_EQ(result, expected);
}

TEST(CombineOverlappingRegions, StartOfSecondInMidOfFirst_SingleRange) {
  std::vector<PrgIndexRange> kmer_region_ranges = {
      {2, 4},
      {3, 5},
  };

  auto result = combine_overlapping_regions(kmer_region_ranges);
  std::vector<PrgIndexRange> expected = {
      {2, 5},
  };
  EXPECT_EQ(result, expected);
}

TEST(CombineOverlappingRegions, CommonStart_SingleRegionWithLargestEnd) {
  std::vector<PrgIndexRange> kmer_region_ranges = {
      {2, 4},
      {2, 5},
  };

  auto result = combine_overlapping_regions(kmer_region_ranges);
  std::vector<PrgIndexRange> expected = {
      {2, 5},
  };
  EXPECT_EQ(result, expected);
}

TEST(CombineOverlappingRegions, EmptyRange_EmptyRange) {
  std::vector<PrgIndexRange> kmer_region_ranges = {};

  auto result = combine_overlapping_regions(kmer_region_ranges);
  std::vector<PrgIndexRange> expected = {};
  EXPECT_EQ(result, expected);
}

TEST(GetReversedKmers,
     GivenRandomlyArrangedReverseKmers_KmersReversedAndSortedByRightMostBase) {
  ordered_vector_set<Sequence> kmers = {
      {2, 4, 1},
      {1, 3, 5},
      {1, 3, 4},
      {3, 4, 5},
  };

  std::vector<Sequence> result = reverse(kmers);
  Sequences expected = {
      {4, 3, 1},
      {5, 3, 1},
      {1, 4, 2},
      {5, 4, 3},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetReversedKmers, GivenSingleReverseKmer_CorrectReversedKmer) {
  ordered_vector_set<Sequence> kmers = {
      {2, 4, 1},
  };

  std::vector<Sequence> result = reverse(kmers);
  Sequences expected = {
      {1, 4, 2},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetReversedKmers, SortingReverseKmerFromRightToLeft_CorrectReversedKmers) {
  ordered_vector_set<Sequence> kmers = {
      {1, 3, 5},
      {2, 4, 1},
  };

  std::vector<Sequence> result = reverse(kmers);
  Sequences expected = {
      {5, 3, 1},
      {1, 4, 2},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetPrefixDiffs, GivenKmersDifferInLeftMostBaseOnly_CorrectPrefixDiffs) {
  std::vector<Sequence> kmers = {
      {1, 3, 1},
      {2, 3, 1},
      {3, 3, 1},
      {4, 3, 1},
  };

  auto result = get_prefix_diffs(kmers);
  std::vector<Sequence> expected = {
      {1, 3, 1},
      {2},
      {3},
      {4},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetPrefixDiffs, GivenKmerDifferInRightMostBaseOnly_CorrectPrefixDiffs) {
  std::vector<Sequence> kmers = {
      {1, 3, 1},
      {2, 3, 1},
      {1, 3, 2},
  };

  auto result = get_prefix_diffs(kmers);
  std::vector<Sequence> expected = {
      {1, 3, 1},
      {2},
      {1, 3, 2},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetPrefixDiffs, GivenMixOfOrderedKmers_CorrectPrefixDiffs) {
  std::vector<Sequence> kmers = {
      {1, 3, 1}, {2, 3, 1}, {1, 3, 2}, {1, 4, 2}, {3, 4, 2},
  };

  auto result = get_prefix_diffs(kmers);
  std::vector<Sequence> expected = {
      {1, 3, 1}, {2}, {1, 3, 2}, {1, 4}, {3},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetAllReverseKmers, GivenOverkillMaxReadSize_AllPossibleKmersReturned) {
  auto prg_raw = encode_prg("ta5g6a6acgt");
  auto prg_info = generate_prg_info(prg_raw);

  BuildParams parameters = {};
  parameters.kmers_size = 3;
  parameters.max_read_size = 10;

  auto result = get_prg_reverse_kmers(parameters, prg_info);
  ordered_vector_set<Sequence> expected = {
      {3, 1, 4}, {1, 1, 4}, {1, 3, 1}, {1, 1, 1},
      {4, 3, 2}, {3, 2, 1}, {2, 1, 1}, {2, 1, 3},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetAllReverseKmers,
     KmerPossibleAfterVariantSite_ReverseKmerIncludedInResult) {
  auto prg_raw = encode_prg("cta5g6a6acgt");
  auto prg_info = generate_prg_info(prg_raw);

  BuildParams parameters = {};
  parameters.kmers_size = 3;
  parameters.max_read_size = 10;

  auto result = get_prg_reverse_kmers(parameters, prg_info);
  ordered_vector_set<Sequence> expected = {
      {3, 1, 4}, {1, 1, 4}, {1, 3, 1}, {1, 1, 1}, {4, 3, 2},
      {3, 2, 1}, {2, 1, 1}, {2, 1, 3}, {1, 4, 2},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetAllReverseKmers, SecondVariantSiteEndsAtPrgEnd_CorrectReverseKmers) {
  auto prg_raw = encode_prg("cta5g6a6acgt7cc8t8");
  auto prg_info = generate_prg_info(prg_raw);

  BuildParams parameters = {};
  parameters.kmers_size = 3;
  parameters.max_read_size = 10;

  auto result = get_prg_reverse_kmers(parameters, prg_info);
  ordered_vector_set<Sequence> expected = {
      {3, 1, 4}, {1, 1, 4}, {1, 3, 1}, {1, 1, 1}, {4, 3, 2}, {3, 2, 1},
      {2, 1, 1}, {2, 1, 3}, {1, 4, 2}, {2, 4, 3}, {2, 2, 4}, {4, 4, 3},
  };
  EXPECT_EQ(result, expected);
}

/**
 * @note the expected elements are written out in an easier to read
 * right-to-left order; but are stored in ordered fashion ({2,1,1,4,2} first).
 */
TEST(GetAllReverseKmers, KmersOverlappingTwoVariantSites_CorrectReverseKmers) {
  auto prg_raw = encode_prg("cta5g6a6cgt7cc8t8");
  auto prg_info = generate_prg_info(prg_raw);

  BuildParams parameters = {};
  parameters.kmers_size = 5;
  parameters.max_read_size = 10;

  auto result = get_prg_reverse_kmers(parameters, prg_info);
  ordered_vector_set<Sequence> expected = {
      {4, 4, 3, 2, 1}, {4, 4, 3, 2, 3}, {2, 2, 4, 3, 2}, {2, 4, 3, 2, 1},
      {2, 4, 3, 2, 3}, {4, 3, 2, 1, 1}, {4, 3, 2, 3, 1}, {3, 2, 1, 1, 4},
      {3, 2, 3, 1, 4}, {2, 1, 1, 4, 2}, {2, 3, 1, 4, 2},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetAllReverseKmers, TwoLeftMostKmersWithinRange_TwoLeftMostKmersIncluded) {
  auto prg_raw = encode_prg("ta5g6a6acgt");
  auto prg_info = generate_prg_info(prg_raw);

  BuildParams parameters = {};
  parameters.kmers_size = 3;
  parameters.max_read_size = 3;

  auto result = get_prg_reverse_kmers(parameters, prg_info);
  ordered_vector_set<Sequence> expected_absent = {
      {4, 3, 2},
      {3, 2, 1},
  };
  for (const auto &reverse_kmer : expected_absent) {
    auto found_flag = result.find(reverse_kmer) != result.end();
    EXPECT_TRUE(found_flag);
  }
}

TEST(GetAllReverseKmers, MaxReadSizeLessThanKmerSize_AlleleKmersReturned) {
  auto prg_raw = encode_prg("ta5g6a6acgt");
  auto prg_info = generate_prg_info(prg_raw);

  BuildParams parameters = {};
  parameters.kmers_size = 3;
  parameters.max_read_size = 1;

  auto result = get_prg_reverse_kmers(parameters, prg_info);
  ordered_vector_set<Sequence> expected = {
      {1, 1, 1}, {1, 1, 4}, {3, 1, 4}, {1, 3, 1}, {2, 1, 1},
      {2, 1, 3}, {3, 1, 4}, {3, 2, 1}, {4, 3, 2},
  };
  EXPECT_EQ(result, expected);
}

TEST(GetAllReverseKmers, GivenPrg_CorrectReverseKmerFound) {
  //               |                         |
  auto prg_raw = encode_prg("atggaacggct5cg6cc6tg6tc6cg7g8a8tccccgacgat");
  auto prg_info = generate_prg_info(prg_raw);

  BuildParams parameters = {};
  parameters.kmers_size = 15;
  parameters.max_read_size = 150;

  auto reverse_kmers = get_prg_reverse_kmers(parameters, prg_info);
  Sequence expected_reverse_kmer = {3, 3, 2, 3, 2, 4, 2, 3,
                                    3, 2, 1, 1, 3, 3, 4};
  auto result =
      reverse_kmers.find(expected_reverse_kmer) != reverse_kmers.end();
  EXPECT_TRUE(result);
}

TEST(GetAllReverseKmers,
     GivenPrgWithLongNonVariantTail_PreviouslyAbsentKmerFound) {
  // kmer          |                         |
  auto prg_raw =
      encode_prg("atggaacggct5cg6cc6tg6tc6cg7g8a8tccccgacgattccccgacga");
  auto prg_info = generate_prg_info(prg_raw);

  BuildParams parameters = {};
  parameters.kmers_size = 15;
  parameters.max_read_size = 20;

  auto kmers = get_prg_reverse_kmers(parameters, prg_info);
  Sequence expected_kmer = {3, 3, 2, 3, 2, 4, 2, 3, 3, 2, 1, 1, 3, 3, 4};
  auto result = kmers.find(expected_kmer) != kmers.end();
  EXPECT_TRUE(result);
}

TEST(GetAllOrderedKmers, GivenPrg_CorrectForwardKmerFound) {
  //               |                         |
  auto prg_raw = encode_prg("atggaacggct5cg6cc6tg6tc6cg7g8a8tccccgacgat");
  auto prg_info = generate_prg_info(prg_raw);

  BuildParams parameters = {};
  parameters.kmers_size = 15;
  parameters.max_read_size = 150;

  auto kmers = get_all_kmers(parameters, prg_info);
  Sequence expected_kmer = {4, 3, 3, 1, 1, 2, 3, 3, 2, 4, 2, 3, 2, 3, 3};
  auto result =
      std::find(kmers.begin(), kmers.end(), expected_kmer) != kmers.end();
  EXPECT_TRUE(result);
}

TEST(GetKmerPrefixDiffs,
     GivenPrgAndTargetKmer_CorrespondingPrefixDiffEntryFound) {
  //               |                         |
  auto prg_raw = encode_prg("atggaacggct5cg6cc6tg6tc6cg7g8a8tccccgacgat");
  auto prg_info = generate_prg_info(prg_raw);

  BuildParams parameters = {};
  parameters.kmers_size = 15;
  parameters.max_read_size = 150;

  auto kmers = get_all_kmers(parameters, prg_info);
  Sequence kmer = {4, 3, 3, 1, 1, 2, 3, 3, 2, 4, 2, 3, 2, 3, 3};
  auto kmer_it = std::find(kmers.begin(), kmers.end(), kmer);
  auto index = std::distance(kmers.begin(), kmer_it);

  auto prefix_diffs =
      get_all_kmer_and_compute_prefix_diffs(parameters, prg_info);
  auto result = prefix_diffs[index];
  Sequence expected = {4, 3, 3, 1, 1, 2, 3, 3, 2, 4, 2, 3};
  EXPECT_EQ(result, expected);
}

TEST(GetAllKmers, GenerateAllKmersLengthThree_CorrectOrder) {
  auto prg_raw = encode_prg("acgt");
  auto prg_info = generate_prg_info(prg_raw);
  BuildParams parameters = {};
  parameters.kmers_size = 3;

  auto result = get_all_kmers(parameters, prg_info);

  std::vector<Sequence> expected = {
      {1, 1, 1}, {2, 1, 1}, {3, 1, 1}, {4, 1, 1}, {1, 2, 1}, {2, 2, 1},
      {3, 2, 1}, {4, 2, 1}, {1, 3, 1}, {2, 3, 1}, {3, 3, 1}, {4, 3, 1},
      {1, 4, 1}, {2, 4, 1}, {3, 4, 1}, {4, 4, 1}, {1, 1, 2}, {2, 1, 2},
      {3, 1, 2}, {4, 1, 2}, {1, 2, 2}, {2, 2, 2}, {3, 2, 2}, {4, 2, 2},
      {1, 3, 2}, {2, 3, 2}, {3, 3, 2}, {4, 3, 2}, {1, 4, 2}, {2, 4, 2},
      {3, 4, 2}, {4, 4, 2}, {1, 1, 3}, {2, 1, 3}, {3, 1, 3}, {4, 1, 3},
      {1, 2, 3}, {2, 2, 3}, {3, 2, 3}, {4, 2, 3}, {1, 3, 3}, {2, 3, 3},
      {3, 3, 3}, {4, 3, 3}, {1, 4, 3}, {2, 4, 3}, {3, 4, 3}, {4, 4, 3},
      {1, 1, 4}, {2, 1, 4}, {3, 1, 4}, {4, 1, 4}, {1, 2, 4}, {2, 2, 4},
      {3, 2, 4}, {4, 2, 4}, {1, 3, 4}, {2, 3, 4}, {3, 3, 4}, {4, 3, 4},
      {1, 4, 4}, {2, 4, 4}, {3, 4, 4}, {4, 4, 4},
  };

  for (uint64_t i = 0; i < result.size(); ++i) {
    auto result_kmer = result[i];
    auto expected_kmer = expected[i];
    EXPECT_EQ(result_kmer, expected_kmer);
  }
}

TEST(GenerateKmers, GenerateAllKmersOfSizeThree_CorrectSpotCheck) {
  auto kmers = generate_all_kmers(3);
  std::vector<Sequence> expected_kmers = {
      {1, 1, 1}, {1, 1, 2}, {1, 1, 3}, {1, 1, 4}, {1, 2, 1},
      {1, 2, 2}, {1, 2, 3}, {1, 2, 4}, {1, 3, 1}, {3, 3, 3},
      {4, 4, 2}, {1, 4, 2}, {4, 4, 4},
  };

  for (const auto &expected_kmer : expected_kmers) {
    auto result =
        std::find(kmers.begin(), kmers.end(), expected_kmer) != kmers.end();
    EXPECT_TRUE(result);
  }
}
