http://sourceforge.net/projects/posixamake

POSIX amake is a software-build tool, which adds automatic
dependency analysis and target caching to GNU Make. It
monitors and records commands executed, programs executed,
shared libraries opened, files accessed, and
environment-variable values. It maintains a
multiuser/multihost cache of previously built targets.  A
GDBM or an SQL database stores target metadata (e.g.,
command and dependency checksums), while a regular
filesystem stores target files and their siblings.

The target-cache feature is optional, being enabled by a
makefile variable. It has been used to build and rebuild the
Linux kernel.

The amake-0.3.tar.gz code is stable, in the sense that it is
currently being used by a team of firmware developers at
HP. The amake-0.4.tar.gz code adds target caching, and
should be treated as alpha quality.

The distribution is still being rearranged, to simplify
deployment. Currently, the distribution includes a pristine
GNU/Make distribution, against which changes are applied. It
also includes a test suite.

buff@cs.boisestate.edu

------------------------------------------------------------

I ve built amake on a few GNU/Linux distributions. It may or
may not build in other environments.

------------------------------------------------------------

The script M.boot builds amake, with make.

The script M.make builds amake with amake. You ll want to
move the contents of the doc, lib, and bin directories
somewhere appropriate.

The script M.tcd builds tcd, with amake. You ll want to move
the contents of the bin directory somewhere appropriate.

The script bin/tidinit suggests how the target-cache
database and target-cache filesystem might be initialized.

The script M.test runs the regression test suite, which is
not very portable

------------------------------------------------------------

The Linux kernel in this distribution is here because I
changed its build process to use amake, rather than KBuild.

On my machine, stock make builds the kernel in about 35
minutes. An up-to-date build requires about 1 minute.

On my machine, building the same kernel, with an old laptop
as an NFS server (for the target cache), an amake "put"
build takes about 80 minutes, an amake "get" build takes
about 30 minutes, and an amake "utd" (up-to-date) build
takes about 1.5 minutes. These builds are done using
checksums.
