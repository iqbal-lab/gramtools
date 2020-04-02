"""
Tested behaviour:
    * Overlapping vcf records (same CHROM, same POS): the first is kept, the rest is ignored
    * Fasta records with no variation: they are output to the prg string in the order seen in the input fasta.

    * Adjacent records: site markers placed adjacent to each other,
    * Allele markers: in 'normal' mode, use even (allele) marker, in legacy use odd (site) marker at end of site.
"""
import filecmp
from tempfile import mkdtemp
from shutil import rmtree
from pathlib import Path
import os
import subprocess
from unittest import TestCase, mock

from gramtools.commands.build import vcf_to_prg_string
from gramtools.commands.build.vcf_to_prg_string import Vcf_to_prg
from gramtools.tests.mocks import _MockVcfRecord

base_dir = Path(__file__).parent.parent.parent.parent
data_dir = base_dir / "tests" / "data" / "vcf_to_prg_string"


@mock.patch("gramtools.commands.build.vcf_to_prg_string.load_fasta")
@mock.patch("gramtools.commands.build.vcf_to_prg_string.VariantFile", spec=True)
class Test_Filter_Records(TestCase):
    chroms = {"JAC": "AAC"}

    def test_filter_pass_record_kept(self, mock_var_file, mock_load_fasta):
        recs = [_MockVcfRecord(2, "A", "G")]  # Default filter: PASS
        mock_var_file.return_value.fetch.return_value = iter(recs)
        mock_load_fasta.return_value = self.chroms
        converter = Vcf_to_prg("", "", "")
        self.assertEqual(converter._get_string(), "A5A6G6C")

    def test_filter_nonpass_record_skipped(self, mock_var_file, mock_load_fasta):
        recs = [_MockVcfRecord(2, "A", "G", filter={"LOW_QUAL": ""})]
        mock_var_file.return_value.fetch.return_value = iter(recs)
        mock_load_fasta.return_value = self.chroms
        converter = Vcf_to_prg("", "", "")
        self.assertEqual(converter._get_string(), "AAC")


class Utility_Tester(object):
    """
    Runs python vcf to prg utility
    """

    def __init__(self, file_prefix):
        self.vcf_file = data_dir / Path(f"{file_prefix}.vcf")
        self.ref_file = data_dir / Path(f"{file_prefix}.ref.fa")
        self.expected_prg = data_dir / Path(f"{file_prefix}.prg")

        self.tmp_dir = mkdtemp()
        # Clumsily below is also the name of the binary output file
        self.outfile_prefix = Path(f"{self.tmp_dir}") / Path("prg")
        self.outfile = Path(str(self.outfile_prefix) + ".prg")

    def _run(self, mode="normal", check=False):
        """
        :param mode: either "normal" or "legacy". In legacy, variant sites encoded
        in same way as perl utility.
        :return:
        """
        converter = vcf_to_prg_string.Vcf_to_prg(
            str(self.vcf_file), str(self.ref_file), self.outfile_prefix, mode=mode
        )
        converter._write_string()

    def _cleanup(self):
        rmtree(self.tmp_dir)


class Test_VcfFormatInputs(TestCase):
    test_cases = {"bad_header_complex_variant"}

    def test_no_fileformat_tag(self):
        """
        Bad header causes pysam error throw
        """
        fname = "bad_header_complex_variant"
        tester = Utility_Tester(fname)

        with self.assertRaises(Exception):
            tester._run(check=True)


class Test_VcfToPrgString(TestCase):
    def test_routine_behaviour(self):
        test_cases = {
            "no_variants": "Vcf file has no records",
            "two_separate_snps": "The SNPs are not consecutive",
            "one_snp_two_alts": "Deal with more than one alternatives",
            "one_ins_and_one_del": "Indels",
            "complex_variant": "Mixture of indel & SNP",
            "two_snps_same_place": "Same position covered 1+ times;"
            "Expect only first vcf record kept",
            "snp_inside_del": "Overlapping records;"
            "Expect only first vcf record kept",
        }

        for fname in test_cases.keys():
            with self.subTest(fname=fname):
                tester = Utility_Tester(fname)

                tester._run(mode="legacy")
                self.assertTrue(
                    filecmp.cmp(tester.expected_prg, tester.outfile, shallow=False)
                )
                tester._cleanup()

    def test_adjacent_snps_allowed(self):
        test_cases = {
            "two_adjacent_snps": "Two SNPs at consecutive positions.",
            "two_adjacent_snps_two_alts": "Same as above,"
            " with more than one alternative",
        }

        for fname in test_cases.keys():
            with self.subTest(fname=fname):
                tester = Utility_Tester(fname)
                tester._run(mode="legacy")
                self.assertTrue(
                    filecmp.cmp(tester.expected_prg, tester.outfile, shallow=False)
                )
                tester._cleanup()

    def test_AlleleRepresentation(self):
        """
        Tests the new representation of variant sites implemented
        by default in the python utility (mode="normal")
        """
        test_cases = {
            "one_ins_and_one_del": "",
            "two_adjacent_snps": "Same as above," " with more than one alternative",
        }

        for fname in test_cases.keys():
            with self.subTest(fname=fname):
                tester = Utility_Tester(fname)

                tester._run()  # normal mode is default
                self.assertTrue(
                    filecmp.cmp(
                        os.path.join(data_dir, "even_allele_markers_" + fname + ".prg"),
                        tester.outfile,
                        shallow=False,
                    )
                )
                tester._cleanup()


class Test_RefWithNoRecords(TestCase):
    def test_one_snp(self):
        """
        Three ref records, of which the second has a SNP.
        We expect the output PRG to have the ref records in the same order as in the
        input ref. cf the .ref.fa and .prg data files
        """
        tester = Utility_Tester("one_snp")
        tester._run()
        self.assertTrue(filecmp.cmp(tester.expected_prg, tester.outfile, shallow=False))
        tester._cleanup()
