Daniel Jeong (dsj58)

Partner: Kevin Zhang (kz225)

Different Cases for Directories:
1. only directories in the command line
2. directories contain directories in them
3. directories contain file names
4. directories within directories with files etc.

Different Cases for Files:
1. empty files
2. files with multiple spaces
3. files began with spaces/newlines
4. Made sure suffix was checked before reading file

Threads:
1. different amount of threads for each type
2. default thread options
3. Joining threads when they should end

JSD:
1. Ensure completely different files have jsd of 1
2. Ensure duplicate files have jsd of 0;
3. Ensure correct jsd

Sorting:
1. Sorted each file's WFD in alphabetical order (makes it easy to compute JSD later)
2. Made sure final JSD list was sorted using qsort