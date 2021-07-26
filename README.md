# unitedat

Unify data sets which consist of separate files with a common header repeated in each one.

## Why?

Many data sets are distributed as a collection of CSV files. For example, each year's data might be in its own CSV file. Each of these CSV files share a common header (typically one line, but there might be more).

It is frequently convenient to concatenate these into a single data file before moving further into the data ingestion/transformation pipeline. Naive `cat` does not work because it leaves us with header lines at various offsets into the concatenated file.

One can solve this using [various tools in the shell](https://unix.stackexchange.com/q/60577/2371) and, in fact, I've written various similar aliases/shell/batch scripts over the years. I've also wanted a small binary I can quickly build on any system with a C compiler so that I can get the same behavior everywhere. I also wanted something that can handle Unicode and long file names on Windows.

## Example

Consider [VAERS](https://vaers.hhs.gov/data/datasets.html). You can either download individual years or all of the data in one big ZIP file. For regularly updating copies, I recommend processing "All Years Data" once to set up your data store and then downloading fresh copies of the most recent year every Friday morning. Or you can set up a regular job that checks the value of "Last updated" on that page and kicks off the scehem if you are really in a rush to get it as soon as the data are updated. &hellip; I digress.

Let's say you've downloaded "[All Years Data](https://vaers.hhs.gov/eSubDownload/index.jsp?fn=AllVAERSDataCSVS.zip)" (note, that is gated using a CAPTCHA). The ZIP archive includes three sets of files:
```text
????VAERSSYMPTOMS.csv
????VAERSDATA.csv
????VAERSVAX.csv
```
Where the leading `????` correspond to the four digit year. There are also files with the prefix `NonDomestic`, but, right now, I am not interested in them.

Each group of files share a common one-line header. Then you can create a unified single file for each section using the command like this:
```text
$ unitedat ????VAERSDATA.csv > VAERSDATA.csv

$ wc -l ????VAERSDATA.csv | grep total
  1130155 total

$ wc -l VAERSDATA.csv
1130124 VAERSDATA.csv

$ wc -l ????VAERSDATA.csv | grep -v total | wc -l
32
```
There are 32 files with a single line header in each. Therefore, the unified file has 31 fewer lines than all the files combined.

## Building

### Microsoft Visual Studio C compiler

```text
nmake -f Make_mvc.mak
```
### GCC

```text
make -f Make_gcc.mak
```

The `install` target requires that the directory `$SITE_DIR/bin` exists.

## Tests

There are no unit tests, but there are a few tests to make sure the executable is doing reasonable things. Running these tests require Perl's [Test2::Suite](https://metacpan.org/pod/Test2::Suite), [Capture::Tiny](https://metacpan.org/pod/Capture::Tiny), and [Path::Tiny](https://metacpan.org/pod/Path::Tiny).

## Caveats

* So far, it only works on my machine :-)
* I have not tried it with [MinGW](http://mingw-w64.org/doku.php).

## Bugs

Plenty. Please [report bugs on GitHub](https://github.com/nanis/unitedat/issues).

## Copyright & License

Copyright &copy; 2021 A. Sinan Unur

[Apache License 2.0](https://github.com/nanis/unitedat/blob/main/LICENSE)
