eubar
=====

eubar et al. are a suite of simple tools for archiving files, with file
contents (by default) stored in one file and file metadata (including
references to their contents) in another.

eubar is unusual in several regards.

First, you must tell it exactly which files to archive; it doesn't perform any
kind of directory traversal, file filtering, etc.

Second, eubar itself doesn't have any file extraction capability; you must use
the accompanying scripts eubout and eubotar to extract the contents of a single
file or to create a tar archive, respectively.

Third, no advanced archive features are implemented -- compression, encryption,
indexing, etc.  These functions, if desired, may be supplied by external
programs.  For convenience, file contents may (if desired) be hashed using the
very fast BLAKE2b algorithm.

libtar (for tar file generation) and libb2 (for BLAKE2b hashing) are
prerequisites.

