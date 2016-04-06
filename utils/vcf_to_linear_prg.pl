#!/usr/bin/perl -w
use strict;

use Getopt::Long; 


my %vars = ( "vcf" => "zam",
	     "ref" => "",
             "min_freq" =>0,
	     "outfile"=>"",
             "help"=>'');


## For 1000 genomes VCF, can use
## /Net/banyan/data0/users/zam/results/20150429_build_1000g_for_gramtools/ALL.wgs.phase3_shapeit2_mvncall_integrated_v5a.20130502.sites.vcf
## For human ref genome my  /Net/birch/data/zam/ref/hs/fasta/grc37/Homo_sapiens.GRCh37.60.dna.WHOLE_GENOME.fa";
&GetOptions(
    ##mandatory args
    'vcf:s' =>\$vars{"vcf"},
    'ref:s' =>\$vars{"ref"},
    'outfile:s' =>\$vars{"outfile"},
    'min_freq:s' => \$vars{"min_freq"},
    'help'  =>\$vars{"help"},
    );


check_args(\%vars);

#test_cluster_func();

## load the reference into memory:
my %refseq = ();#chr--> long string
my @chroms=();#collect list of chromosomes
get_ref_seq($vars{"ref"}, \%refseq, \@chroms); 

my $output_fh;
open($output_fh, ">".$vars{"outfile"})||die("Unable to open output file\n");

## parse the VCF and print a linearised PRG in gramtools format
my $last_varnumber = print_linearised_poa(\%refseq, $vars{"ref"}, 
					  $vars{"vcf"}, $vars{"min_freq"},
					  \@chroms, $output_fh);
close($output_fh);

print "Finished printing linear PRG. Final number in alphabet is  $last_varnumber\n";

sub test_cluster_func
{
    my @arr1 = (1,2);
    my @arr2=(3,4,5);
    my @arr3=(6,7);
    my @arr=(\@arr1, \@arr2, \@arr3);
    my $res = recursive_get_haplotypes(\@arr);
    foreach my $c (@$res)
    {
	print "$c\n";
    }
}

sub print_linearised_poa_for_one_chr
{
    my ($href_refsequence, $reff, $vcf_file, 
	$chrom, $nextvar, $min_freq, $o_fh)= @_;


    ## Assume the VCF is  sorted. There are two reasons
    ## that prevent us from treating all records independently
    ## 1. variants with no space between - we combine
    ##    and print all possile haplotypes
    ## 2. long deletions on top of SNPs - here we ignore the SNPs


    my %clusters=();# if a variant is in a cluster, 
                    # if is first in cluster, have
                    # pos->ref,alt1,alt2,... (all possible haplos)
                    # if is a later one, have
                    # pos->0 (which will tell us to ignore it)


    ##this function dies if a later record is underneath a prior
    get_clusters($chrom, $vcf_file, \%clusters, $min_freq);


    if (!exists $href_refsequence->{$chrom})
    {
	die("Cannot find sequence for chromosome $chrom");
    }

    my $seq = $href_refsequence->{$chrom};
    open(VCF, $vcf_file)||die("Cannot open VCF file $vcf_file");
    my $curr_pos=1; ## 1-based

    while (<VCF>)
    {
	my $lyne  = $_;
	chomp $lyne;

	if ($lyne !~ /^\#/)
	{
	    ## I will work entirely in 1-based coordinates, 
	    ## except at the point of extracting substrings.


	    my @sp = split(/\t/, $lyne);

	    if ($sp[4] !~ /^[ACGTacgt]+$/)
	    {
		## excluding lines which do not 
		## properly specify the alternate allele.
		next;
	    }


	    my $info = $sp[7];
	    if ($min_freq>0)
	    {

		if ($info =~ /\;AF=([0123456789\.]+)/)
		{
		    my $freq = $1;

		    if ($freq<$min_freq)
		    {
			next; #ignore this variant if too rare
		    }
		}
		else
		{
		    #if no allele frequency annotation, do not filter by frequency
		    
		}
	    }

	    if ($sp[0] eq $chrom)
	    {

		if ($curr_pos < $sp[1] )
		{
		    my $len = $sp[1]-$curr_pos;
		    print $o_fh substr($seq, $curr_pos-1, $len);
		    #$curr_pos=$sp[1];
		}

		#replace N with C
		$sp[3]=~ s/[^ACGTacgt]/C/g;


		if (exists $clusters{$sp[1]})
		{
		    if ($clusters{$sp[1]} eq "0")
		    {
			#print "SKIP THIS GUY ";
			#print $sp[1];
			#print "\n";
			next;## either this is a late record in a cluster (so ignore)
			     ## or it is a line in the VCF that overlaps a previous one
		    }
		    else
		    {
			
			##modify the ref/alt alleles to represent all possible haplotypes in the cluster
			$sp[3]=$clusters{$sp[1]}->[0];
			my $str="";
			my $k;
			for ($k=1; $k<scalar(@{$clusters{$sp[1]}}); $k++)
			{
			    $str=$str.($clusters{$sp[1]}->[$k]);
			    if ($k<scalar(@{$clusters{$sp[1]}})-1)
			    {
				$str=$str.",";
			    }
			}
			$sp[4]=$str;

		    }
		}


		print $o_fh $nextvar;#left marker before the site starts
		print $o_fh $sp[3];		##print the ref allele first
		print $o_fh $nextvar+1;#even numbers between alleles

		##Now work our way through the alternate alleles
		if ($sp[4]=~ /,/)
		{
		    my @sp2 = split(/,/, $sp[4]);
		    my $i;
		    for ($i=0; $i<scalar(@sp2); $i++)
		    {
			my $allele = $sp2[$i];
			$allele =~ s/[^ACGTacgt]/C/g;
			print $o_fh $allele;
			if ($i<scalar(@sp2)-1)
			{
			    print $o_fh $nextvar+1;#even number between alleles
			}
			else
			{
			    print $o_fh $nextvar;#last one goes back to nextvar (odd)
			}
		    }
		}
		else #we have just one alternate allele
		{
		    $sp[4]=~ s/[^ACGTacgt]/C/g;
		    print $o_fh $sp[4];
		    print $o_fh $nextvar;
		}
		$nextvar+=2;
		$curr_pos=$sp[1]+length($sp[3]);
	    }
	    else
	    {
		#ignore
	    }
	}
    }
    close(VCF);

    if ($curr_pos<length($seq)+1)
    {
	print $o_fh substr($seq, $curr_pos, length($seq)-$curr_pos-1);
    }




    return $nextvar;
}


sub get_clusters
{

    my ($chr, $vcf_f, $href_cluster, $min_frequency)=@_;

    # Read through the file once. At a given record, notice the start/end coords on the ref
    # Move to next record - if there is >=1 bp between them, forget the previous one.
    # However, if they overlap, or abut, then collect them - we are going to make all possible haplotypes.
    # Complicated cases are a) long things with stuff under

    my @alleles=();
    my $last_start=-1;#start/end of ref allele 
    my $last_end=-1;
    my $last_ref="";
    my $last_alt="";
    my $not_first_var_on_chrom=0;
    my $currently_in_cluster=0;
    my $current_cluster_start=0;


    open(VCFF, $vcf_f)||die("Cannot open $vcf_f");
    while (<VCFF>)
    {
	my $vcfline = $_;
	chomp $vcfline;
	
	if ($vcfline !~ /^\#/)
	{
	    my @fields = split(/\t/, $vcfline);
	    if ($fields[6] ne "PASS")
	    {
		next;
	    }

	    if ($fields[0] eq $chr)
	    {

		if ($fields[7] =~ /\;AF=([0123456789\.]+)/)
		{
		    my $freq = $1;
		    if ($freq<$min_frequency)
		    {
			next; #ignore this variant if too rare
		    }
		}

		my $pos = $fields[1];
		my $ref = $fields[3];
		my $alt = $fields[4];
		if ($not_first_var_on_chrom==1)
		{
		    if ($pos<$last_start)
		    {
			die("Badly srted VCF. chr $chr, pos $pos we have a variant BEFORE the previous line\n");
		    }
		    elsif ($pos==$last_start)
		    {
			die("Multiple records in this VCF starting at same line\n");
		    }

		    if ($pos<=$last_end)
		    {
			## this is a case of overlapping variants.
			$href_cluster->{$pos}=0; ##basically tell downstream stuff to ignore this variant
			next;
		    }
		    if ($pos==$last_end+1)
		    {
			#abutting variants - cluster started at prev variant or even earlier

			if ($currently_in_cluster==0)
			{
			    ##cluster started at previous record
			    $currently_in_cluster=1;
			    $current_cluster_start=$last_start;
			    push @alleles, get_haplo_array($last_ref, $last_alt); 			
			}
			else
			{
			    #another record in an ongoing cluster
			}
			$href_cluster->{$pos}=0;
			push @alleles, get_haplo_array($ref, $alt); 			
			$currently_in_cluster=1;
		    }
		    else 
                      # there is a gap between current 
		      # variant and previous one. No cluster any more
		    {
			if ($currently_in_cluster==1)
			{
			    #we have just got to the end of 
			    #a cluster. Update the hash
			    #with a list of all possible haplotypes.
			    my $temp
			    =recursive_get_haplotypes(\@alleles);

			    $href_cluster->{$current_cluster_start}=$temp;

			}
			$currently_in_cluster=0;

			@alleles=();


		    }
		}
		$last_start = $pos;
		$last_end = $pos+length($ref)-1;
		$last_ref=$fields[3];
		$last_alt=$fields[4];
	
		$not_first_var_on_chrom=1
	    }
	}
	
    }
    close(VCFF);




}




sub get_haplo_array
{
    my ($refall, $altall) = @_;
    my @v = ();
    push @v, $refall;
    if ($altall =~ /,/)
    {
	my @all = split(/,/, $altall);
	my $i;
	for ($i=0; $i<scalar(@all); $i++)
	{
	    push @v, $all[$i];
	}
    }
    else
    {
	push @v, $altall;
    }
    
    return \@v;
}


# pass in an array ref. Every element on that array
# is itself an array(ref) of ref and then alt alleles.
sub recursive_get_haplotypes
{
    my ($array_ref) = @_;

    if (scalar (@$array_ref)==1)
    {
	#then the alleles themselves are the haplotypes
	return $array_ref->[0];
    }

    

    ##take last variant off array, and call everything before
    ## that "prev"; will recurse

    my @arr = @$array_ref;
    my $last_aref = pop @arr;
    my $results_for_prev = recursive_get_haplotypes(\@arr);

    my $i;
    my $j;
    my @results=();
    for ($i=0; $i<scalar(@$results_for_prev); $i++)
    {
	for($j=0; $j<scalar(@$last_aref); $j++)
	{
	    #take each allele in the last variant
	    my $seq=$last_aref->[$j];
	    #create a new haplotype by extending the current 
	    #one with this allele
	    my $new_haplo = ($results_for_prev->[$i]).$seq;
	    push @results, $new_haplo;
	}
    }

    return \@results;
}



sub print_linearised_poa
{
    my ($href_refseq, $reference, $vcf, $min_f, $aref_chroms, $out_fh) = @_;

    my @chrs = @$aref_chroms; #(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,"X","Y");

    my $next_var_number = 5;
    foreach my $chr (@chrs)
    {
	## so this is not ideal - parse the VCF once for each chromosome
	$next_var_number = 
	    print_linearised_poa_for_one_chr($href_refseq, $reference, 
					     $vcf, $chr, 
					     $next_var_number, $min_f, $out_fh);
    }

    return $next_var_number-1;##will have auto-incremented
}
sub get_ref_seq
{
    my ($fasta, $href, $aref_chroms) = @_;

    my $chr = "";
    my $seq = "";
    open(FILE, $fasta)||die();
    my $first=1;

    while (<FILE>)
    {
	my $line = $_;
	chomp $line;

	if ($line =~ /^>(\S+)/)
	{
	    if ($first !=1)
	    {
		$seq =~ s/[^ACGTacgt]/C/g;
		$href->{$chr}=$seq;
	    }
	    $first=0;
	    $chr = $1;
	    push @$aref_chroms, $chr;
	    $seq="";
	}
	else
	{
	    $seq .= $line;
	    #$href->{$chr}=($href->{$chr}).$line;
	}
    }
    close(FILE);

    ##now do the final chromosome in the file
    ##replacing N with C
    $seq =~ s/[^ACGTacgt]/C/g;
    $href->{$chr}=$seq;
}



sub check_args
{
    my ($href) = @_;

    if ($href->{"help"})
    {
	print "Usage: perl vcf_to_linear_prg.pl --vcf <VCF> --ref species.fasta --min_freq 0.01\n";
	print "\n";
	print "This script is not super-sophisticated - it builds a lin-PRG\n";
	print "as it sweeps once through the VCF\n";
	print "If it meets a new VCF record that overlaps an old one,\nit will ignore it.\n";
	print "The most important consequence is that it won't encode SNPs \"underneath\" a long deletion\n";
	exit(0);
    }

    if ($href->{"vcf"} eq "")
    {
	die("You must specify a VCF file with --vcf \n");
    }

    if ($href->{"outfile"} eq "")
    {
	die("You must specify an output file with --outfile \n");
    }

    if ($href->{"ref"} eq "")
    {
	die("You must specify a reference fasta file with --ref \n");
    }
	
    if (!(-e $href->{"vcf"}))
    {
	print "Specified VCF file ";
	print $href->{"vcf"};
	die(" does not exist");
    }


    if (!(-e $href->{"ref"}))
    {
	print "Specified reference fasta file ";
	print $href->{"ref"};
	die(" does not exist");
    }

    ##let's just avoid any mess with tiny numbers
    if ($href->{"min_freq"}<0.0001)
    {
	$href->{"min_freq"}=0;
    }

}

