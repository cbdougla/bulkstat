# bulkstat
Generates a pipe delimited file from find output

This program will take the output of find (find . -type f) and generate a pipe-delimited file that can be
imported into a spreadsheet or database.

I have used this to keep track of network volumes' space utilization.  
I showed this to a friend and he suggested that others might also find it useful.

I am not much of a programmer so I suspect it's pretty rough by most standards but it works for me.

`[cbd][jordan][/devl/cbd/sourcecode/bulkstat]$ ./bulkstat -h

Usage:  bulkstat [-i <infile>] [-o <outfile>] [-d <delimiter>] [-n <numlines>] [-bDhv]
        b: Set max buffer chunk size (default 512 bytes)
        d: Set output delimiter to argument instead of '|'
        g: Output group ID of file owner
        D: Print debugging info. (Set to a number for more info)
        H: Print header at top of list
        n: Rolling count of number of lines processes
        p: Output separate pathname into dirname and filename fields
        P: Output all three dirname, filename, and pathname fields
        s: Include current date field in output file
        u: Output user ID of file owner
        v: Print version and exit

        h: This help screen

Program will use STDIN and/or STDOUT if no file options are given

A typical example is:  find . -type f | bulkstat > bulkstat.csv`


`[cbd][jordan][/devl/cbd/sourcecode/bulkstat]$ find . -type f | ./bulkstat -H
Path|Bytes|Access Date|Mod Date|Create Date
./bulkstat_new.c|8560|2016-08-19|2016-08-19|2016-08-19
./bulkstat|15714|2016-08-19|2016-08-19|2016-08-19
./bulkstat.c~|8958|2016-08-19|2016-08-19|2016-08-19
./bulkstat.c|9038|2016-08-19|2016-08-19|2016-08-19
./bulkstat.csv|37377|2016-08-19|2016-08-19|2016-08-19
./compile-bulkstat|49|2016-08-19|2016-08-19|2016-08-19`
