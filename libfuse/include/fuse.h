/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU LGPLv2.
  See the file COPYING.LIB.
*/

#ifndef _FUSE_H_
#define _FUSE_H_

#include "fuse_common.h"
#include "fuse_dirents.h"

#include <fcntl.h>
#include <time.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/uio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------- *
 * Basic FUSE API					       *
 * ----------------------------------------------------------- */

/** Handle for a FUSE filesystem */
struct fuse;

/** Structure containing a raw command */
struct fuse_cmd;

/**
 * The file system operations:
 *
 * Most of these should work very similarly to the well known UNIX
 * file system operations.  A major exception is that instead of
 * returning an error in 'errno', the operation should return the
 * negated error value (-errno) directly.
 *
 * All methods are optional, but some are essential for a useful
 * filesystem (e.g. getattr).  Open, flush, release, fsync, opendir,
 * releasedir, fsyncdir, access, create, ftruncate, fgetattr, lock,
 * init and destroy are special purpose methods, without which a full
 * featured filesystem can still be implemented.
 *
 * Almost all operations take a path which can be of any length.
 *
 * Changed in fuse 2.8.0 (regardless of API version)
 * Previously, paths were limited to a length of PATH_MAX.
 *
 * See http://fuse.sourceforge.net/wiki/ for more information.  There
 * is also a snapshot of the relevant wiki pages in the doc/ folder.
 */
struct fuse_operations {
	/** Get file attributes.
	 *
	 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
	 * ignored.	 The 'st_ino' field is ignored except if the 'use_ino'
	 * mount option is given.
	 */
	int (*getattr) (const char *, struct stat *);

	/** Read the target of a symbolic link
	 *
	 * The buffer should be filled with a null terminated string.  The
	 * buffer size argument includes the space for the terminating
	 * null character.	If the linkname is too long to fit in the
	 * buffer, it should be truncated.	The return value should be 0
	 * for success.
	 */
	int (*readlink) (const char *, char *, size_t);

	/** Create a file node
	 *
	 * This is called for creation of all non-directory, non-symlink
	 * nodes.  If the filesystem defines a create() method, then for
	 * regular files that will be called instead.
	 */
	int (*mknod) (const char *, mode_t, dev_t);

	/** Create a directory
	 *
	 * Note that the mode argument may not have the type specification
	 * bits set, i.e. S_ISDIR(mode) can be false.  To obtain the
	 * correct directory type bits use  mode|S_IFDIR
	 * */
	int (*mkdir) (const char *, mode_t);

       /** Hide files unlinked / renamed over
        *
        * Allows storing of a file handle when a file is unlinked
        * while open. Helps manage the fact the kernel usually does
        * not send fh with getattr requests.
        */
        int (*prepare_hide)(const char *name_, uint64_t *fh_);
        int (*free_hide)(const uint64_t fh_);

	/** Remove a file */
        int (*unlink) (const char *);

	/** Remove a directory */
	int (*rmdir) (const char *);

	/** Create a symbolic link */
	int (*symlink) (const char *, const char *);

	/** Rename a file */
	int (*rename) (const char *, const char *);

	/** Create a hard link to a file */
	int (*link) (const char *, const char *);

	/** Change the permission bits of a file */
	int (*chmod) (const char *, mode_t);
        int (*fchmod)(const struct fuse_file_info *, const mode_t);

	/** Change the owner and group of a file */
	int (*chown) (const char *, uid_t, gid_t);
        int (*fchown)(const struct fuse_file_info *, const uid_t, const gid_t);

	/** Change the size of a file */
	int (*truncate) (const char *, off_t);

	/** Change the access and/or modification times of a file
	 *
	 * Deprecated, use utimens() instead.
	 */
	int (*utime) (const char *, struct utimbuf *);

	/** File open operation
	 *
	 * No creation (O_CREAT, O_EXCL) and by default also no
	 * truncation (O_TRUNC) flags will be passed to open(). If an
	 * application specifies O_TRUNC, fuse first calls truncate()
	 * and then open(). Only if 'atomic_o_trunc' has been
	 * specified and kernel version is 2.6.24 or later, O_TRUNC is
	 * passed on to open.
	 *
	 * Unless the 'default_permissions' mount option is given,
	 * open should check if the operation is permitted for the
	 * given flags. Optionally open may also return an arbitrary
	 * filehandle in the fuse_file_info structure, which will be
	 * passed to all file operations.
	 *
	 * Changed in version 2.2
	 */
	int (*open) (const char *, struct fuse_file_info *);

	/** Read data from an open file
	 *
	 * Read should return exactly the number of bytes requested except
	 * on EOF or error, otherwise the rest of the data will be
	 * substituted with zeroes.	 An exception to this is when the
	 * 'direct_io' mount option is specified, in which case the return
	 * value of the read system call will reflect the return value of
	 * this operation.
	 *
	 * Changed in version 2.2
	 */
	int (*read) (const char *, char *, size_t, off_t,
		     struct fuse_file_info *);

	/** Write data to an open file
	 *
	 * Write should return exactly the number of bytes requested
	 * except on error.	 An exception to this is when the 'direct_io'
	 * mount option is specified (see read operation).
	 *
	 * Changed in version 2.2
	 */
	int (*write) (const char *, const char *, size_t, off_t,
		      struct fuse_file_info *);

	/** Get file system statistics
	 *
	 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
	 *
	 * Replaced 'struct statfs' parameter with 'struct statvfs' in
	 * version 2.5
	 */
	int (*statfs) (const char *, struct statvfs *);

	/** Possibly flush cached data
	 *
	 * BIG NOTE: This is not equivalent to fsync().  It's not a
	 * request to sync dirty data.
	 *
	 * Flush is called on each close() of a file descriptor.  So if a
	 * filesystem wants to return write errors in close() and the file
	 * has cached dirty data, this is a good place to write back data
	 * and return any errors.  Since many applications ignore close()
	 * errors this is not always useful.
	 *
	 * NOTE: The flush() method may be called more than once for each
	 * open().	This happens if more than one file descriptor refers
	 * to an opened file due to dup(), dup2() or fork() calls.	It is
	 * not possible to determine if a flush is final, so each flush
	 * should be treated equally.  Multiple write-flush sequences are
	 * relatively rare, so this shouldn't be a problem.
	 *
	 * Filesystems shouldn't assume that flush will always be called
	 * after some writes, or that if will be called at all.
	 *
	 * Changed in version 2.2
	 */
	int (*flush) (const char *, struct fuse_file_info *);

	/** Release an open file
	 *
	 * Release is called when there are no more references to an open
	 * file: all file descriptors are closed and all memory mappings
	 * are unmapped.
	 *
	 * For every open() call there will be exactly one release() call
	 * with the same flags and file descriptor.	 It is possible to
	 * have a file opened more than once, in which case only the last
	 * release will mean, that no more reads/writes will happen on the
	 * file.  The return value of release is ignored.
	 *
	 * Changed in version 2.2
	 */
	int (*release) (const char *, struct fuse_file_info *);

	/** Synchronize file contents
	 *
	 * If the datasync parameter is non-zero, then only the user data
	 * should be flushed, not the meta data.
	 *
	 * Changed in version 2.2
	 */
	int (*fsync) (const char *, int, struct fuse_file_info *);

	/** Set extended attributes */
	int (*setxattr) (const char *, const char *, const char *, size_t, int);

	/** Get extended attributes */
	int (*getxattr) (const char *, const char *, char *, size_t);

	/** List extended attributes */
	int (*listxattr) (const char *, char *, size_t);

	/** Remove extended attributes */
	int (*removexattr) (const char *, const char *);

	/** Open directory
	 *
	 * Unless the 'default_permissions' mount option is given,
	 * this method should check if opendir is permitted for this
	 * directory. Optionally opendir may also return an arbitrary
	 * filehandle in the fuse_file_info structure, which will be
	 * passed to readdir, closedir and fsyncdir.
	 *
	 * Introduced in version 2.3
	 */
	int (*opendir) (const char *, struct fuse_file_info *);

	/** Read directory
	 *
	 * This supersedes the old getdir() interface.  New applications
	 * should use this.
	 *
	 * The filesystem may choose between two modes of operation:
	 *
	 * 1) The readdir implementation ignores the offset parameter, and
	 * passes zero to the filler function's offset.  The filler
	 * function will not return '1' (unless an error happens), so the
	 * whole directory is read in a single readdir operation.  This
	 * works just like the old getdir() method.
	 *
	 * 2) The readdir implementation keeps track of the offsets of the
	 * directory entries.  It uses the offset parameter and always
	 * passes non-zero offset to the filler function.  When the buffer
	 * is full (or an error happens) the filler function will return
	 * '1'.
	 *
	 * Introduced in version 2.3
	 */
        int (*readdir)(struct fuse_file_info *,
                       fuse_dirents_t *);

        int (*readdir_plus)(struct fuse_file_info *,
                            fuse_dirents_t *);


	/** Release directory
	 *
	 * Introduced in version 2.3
	 */
	int (*releasedir) (const char *, struct fuse_file_info *);

	/** Synchronize directory contents
	 *
	 * If the datasync parameter is non-zero, then only the user data
	 * should be flushed, not the meta data
	 *
	 * Introduced in version 2.3
	 */
	int (*fsyncdir) (const char *, int, struct fuse_file_info *);

	/**
	 * Initialize filesystem
	 *
	 * The return value will passed in the private_data field of
	 * fuse_context to all file operations and as a parameter to the
	 * destroy() method.
	 *
	 * Introduced in version 2.3
	 * Changed in version 2.6
	 */
	void *(*init) (struct fuse_conn_info *conn);

	/**
	 * Clean up filesystem
	 *
	 * Called on filesystem exit.
	 *
	 * Introduced in version 2.3
	 */
	void (*destroy) (void *);

	/**
	 * Check file access permissions
	 *
	 * This will be called for the access() system call.  If the
	 * 'default_permissions' mount option is given, this method is not
	 * called.
	 *
	 * This method is not called under Linux kernel versions 2.4.x
	 *
	 * Introduced in version 2.5
	 */
	int (*access) (const char *, int);

	/**
	 * Create and open a file
	 *
	 * If the file does not exist, first create it with the specified
	 * mode, and then open it.
	 *
	 * If this method is not implemented or under Linux kernel
	 * versions earlier than 2.6.15, the mknod() and open() methods
	 * will be called instead.
	 *
	 * Introduced in version 2.5
	 */
	int (*create) (const char *, mode_t, struct fuse_file_info *);

	/**
	 * Change the size of an open file
	 *
	 * This method is called instead of the truncate() method if the
	 * truncation was invoked from an ftruncate() system call.
	 *
	 * If this method is not implemented or under Linux kernel
	 * versions earlier than 2.6.15, the truncate() method will be
	 * called instead.
	 *
	 * Introduced in version 2.5
	 */
	int (*ftruncate) (const char *, off_t, struct fuse_file_info *);

	/**
	 * Get attributes from an open file
	 *
	 * This method is called instead of the getattr() method if the
	 * file information is available.
	 *
	 * Currently this is only called after the create() method if that
	 * is implemented (see above).  Later it may be called for
	 * invocations of fstat() too.
	 *
	 * Introduced in version 2.5
	 */
	int (*fgetattr) (const char *, struct stat *, struct fuse_file_info *);

	/**
	 * Perform POSIX file locking operation
	 *
	 * The cmd argument will be either F_GETLK, F_SETLK or F_SETLKW.
	 *
	 * For the meaning of fields in 'struct flock' see the man page
	 * for fcntl(2).  The l_whence field will always be set to
	 * SEEK_SET.
	 *
	 * For checking lock ownership, the 'fuse_file_info->owner'
	 * argument must be used.
	 *
	 * For F_GETLK operation, the library will first check currently
	 * held locks, and if a conflicting lock is found it will return
	 * information without calling this method.	 This ensures, that
	 * for local locks the l_pid field is correctly filled in.	The
	 * results may not be accurate in case of race conditions and in
	 * the presence of hard links, but it's unlikely that an
	 * application would rely on accurate GETLK results in these
	 * cases.  If a conflicting lock is not found, this method will be
	 * called, and the filesystem may fill out l_pid by a meaningful
	 * value, or it may leave this field zero.
	 *
	 * For F_SETLK and F_SETLKW the l_pid field will be set to the pid
	 * of the process performing the locking operation.
	 *
	 * Note: if this method is not implemented, the kernel will still
	 * allow file locking to work locally.  Hence it is only
	 * interesting for network filesystems and similar.
	 *
	 * Introduced in version 2.6
	 */
	int (*lock) (const char *, struct fuse_file_info *, int cmd,
		     struct flock *);

	/**
	 * Change the access and modification times of a file with
	 * nanosecond resolution
	 *
	 * This supersedes the old utime() interface.  New applications
	 * should use this.
	 *
	 * See the utimensat(2) man page for details.
	 *
	 * Introduced in version 2.6
	 */
	int (*utimens)(const char *, const struct timespec tv[2]);
        int (*futimens)(const struct fuse_file_info *ffi_, const struct timespec tv_[2]);

	/**
	 * Map block index within file to block index within device
	 *
	 * Note: This makes sense only for block device backed filesystems
	 * mounted with the 'blkdev' option
	 *
	 * Introduced in version 2.6
	 */
	int (*bmap) (const char *, size_t blocksize, uint64_t *idx);

	/**
	 * Flag indicating that the filesystem can accept a NULL path
	 * as the first argument for the following operations:
	 *
	 * read, write, flush, release, fsync, readdir, releasedir,
	 * fsyncdir, ftruncate, fgetattr, lock, ioctl and poll
	 *
	 * If this flag is set these operations continue to work on
	 * unlinked files even if "-ohard_remove" option was specified.
	 */
	unsigned int flag_nullpath_ok:1;

	/**
	 * Flag indicating that the path need not be calculated for
	 * the following operations:
	 *
	 * read, write, flush, release, fsync, readdir, releasedir,
	 * fsyncdir, ftruncate, fgetattr, lock, ioctl and poll
	 *
	 * Closely related to flag_nullpath_ok, but if this flag is
	 * set then the path will not be calculaged even if the file
	 * wasn't unlinked.  However the path can still be non-NULL if
	 * it needs to be calculated for some other reason.
	 */
	unsigned int flag_nopath:1;

	/**
	 * Flag indicating that the filesystem accepts special
	 * UTIME_NOW and UTIME_OMIT values in its utimens operation.
	 */
	unsigned int flag_utime_omit_ok:1;

	/**
	 * Reserved flags, don't set
	 */
	unsigned int flag_reserved:29;

	/**
	 * Ioctl
	 *
	 * flags will have FUSE_IOCTL_COMPAT set for 32bit ioctls in
	 * 64bit environment.  The size and direction of data is
	 * determined by _IOC_*() decoding of cmd.  For _IOC_NONE,
	 * data will be NULL, for _IOC_WRITE data is out area, for
	 * _IOC_READ in area and if both are set in/out area.  In all
	 * non-NULL cases, the area is of _IOC_SIZE(cmd) bytes.
	 *
	 * If flags has FUSE_IOCTL_DIR then the fuse_file_info refers to a
	 * directory file handle.
	 *
	 * Introduced in version 2.8
	 */
	int (*ioctl) (const char *fusepath,
                      int cmd,
                      void *arg,
		      struct fuse_file_info *ffi,
                      unsigned int flags,
                      void *data,
                      uint32_t *out_bufsz);

	/**
	 * Poll for IO readiness events
	 *
	 * Note: If ph is non-NULL, the client should notify
	 * when IO readiness events occur by calling
	 * fuse_notify_poll() with the specified ph.
	 *
	 * Regardless of the number of times poll with a non-NULL ph
	 * is received, single notification is enough to clear all.
	 * Notifying more times incurs overhead but doesn't harm
	 * correctness.
	 *
	 * The callee is responsible for destroying ph with
	 * fuse_pollhandle_destroy() when no longer in use.
	 *
	 * Introduced in version 2.8
	 */
	int (*poll) (const char *, struct fuse_file_info *,
		     struct fuse_pollhandle *ph, unsigned *reventsp);

	/** Write contents of buffer to an open file
	 *
	 * Similar to the write() method, but data is supplied in a
	 * generic buffer.  Use fuse_buf_copy() to transfer data to
	 * the destination.
	 *
	 * Introduced in version 2.9
	 */
	int (*write_buf) (const char *, struct fuse_bufvec *buf, off_t off,
			  struct fuse_file_info *);

	/** Store data from an open file in a buffer
	 *
	 * Similar to the read() method, but data is stored and
	 * returned in a generic buffer.
	 *
	 * No actual copying of data has to take place, the source
	 * file descriptor may simply be stored in the buffer for
	 * later data transfer.
	 *
	 * The buffer must be allocated dynamically and stored at the
	 * location pointed to by bufp.  If the buffer contains memory
	 * regions, they too must be allocated using malloc().  The
	 * allocated memory will be freed by the caller.
	 *
	 * Introduced in version 2.9
	 */
	int (*read_buf) (const char *, struct fuse_bufvec **bufp,
			 size_t size, off_t off, struct fuse_file_info *);
	/**
	 * Perform BSD file locking operation
	 *
	 * The op argument will be either LOCK_SH, LOCK_EX or LOCK_UN
	 *
	 * Nonblocking requests will be indicated by ORing LOCK_NB to
	 * the above operations
	 *
	 * For more information see the flock(2) manual page.
	 *
	 * Additionally fi->owner will be set to a value unique to
	 * this open file.  This same value will be supplied to
	 * ->release() when the file is released.
	 *
	 * Note: if this method is not implemented, the kernel will still
	 * allow file locking to work locally.  Hence it is only
	 * interesting for network filesystems and similar.
	 *
	 * Introduced in version 2.9
	 */
	int (*flock) (const char *, struct fuse_file_info *, int op);

	/**
	 * Allocates space for an open file
	 *
	 * This function ensures that required space is allocated for specified
	 * file.  If this function returns success then any subsequent write
	 * request to specified range is guaranteed not to fail because of lack
	 * of space on the file system media.
	 *
	 * Introduced in version 2.9.1
	 */
	int (*fallocate) (const char *, int, off_t, off_t,
			  struct fuse_file_info *);

  /**
   * Copy a range of data from one file to another
   *
   * Performs an optimized copy between two file descriptors without
   * the
   * additional cost of transferring data through the FUSE kernel
   * module
   * to user space (glibc) and then back into the FUSE filesystem
   * again.
   *
   * In case this method is not implemented, glibc falls back to
   * reading
   * data from the source and writing to the destination. Effectively
   * doing an inefficient copy of the data.
   */
  ssize_t (*copy_file_range)(const char            *path_in,
                             struct fuse_file_info *fi_in,
                             off_t                  offset_in,
                             const char            *path_out,
                             struct fuse_file_info *fi_out,
                             off_t                  offset_out,
                             size_t                 size,
                             int                    flags);
};

/** Extra context that may be needed by some filesystems
 *
 * The uid, gid and pid fields are not filled in case of a writepage
 * operation.
 */
struct fuse_context {
	/** Pointer to the fuse object */
	struct fuse *fuse;

	/** User ID of the calling process */
	uid_t uid;

	/** Group ID of the calling process */
	gid_t gid;

	/** Thread ID of the calling process */
	pid_t pid;

	/** Private filesystem data */
	void *private_data;

	/** Umask of the calling process (introduced in version 2.8) */
	mode_t umask;
};

/**
 * Main function of FUSE.
 *
 * This is for the lazy.  This is all that has to be called from the
 * main() function.
 *
 * This function does the following:
 *   - parses command line options (-d -s and -h)
 *   - passes relevant mount options to the fuse_mount()
 *   - installs signal handlers for INT, HUP, TERM and PIPE
 *   - registers an exit handler to unmount the filesystem on program exit
 *   - creates a fuse handle
 *   - registers the operations
 *   - calls either the single-threaded or the multi-threaded event loop
 *
 * Note: this is currently implemented as a macro.
 *
 * @param argc the argument counter passed to the main() function
 * @param argv the argument vector passed to the main() function
 * @param op the file system operation
 * @param user_data user data supplied in the context during the init() method
 * @return 0 on success, nonzero on failure
 */

int fuse_main(int argc,
              char *argv[],
              const struct fuse_operations *op,
              void *user_data);

/* ----------------------------------------------------------- *
 * More detailed API					       *
 * ----------------------------------------------------------- */

/**
 * Create a new FUSE filesystem.
 *
 * @param ch the communication channel
 * @param args argument vector
 * @param op the filesystem operations
 * @param op_size the size of the fuse_operations structure
 * @param user_data user data supplied in the context during the init() method
 * @return the created FUSE handle
 */
struct fuse *fuse_new(int devfuse_fd, struct fuse_args *args,
		      const struct fuse_operations *op, size_t op_size,
		      void *user_data);

/**
 * Destroy the FUSE handle.
 *
 * The communication channel attached to the handle is also destroyed.
 *
 * NOTE: This function does not unmount the filesystem.	 If this is
 * needed, call fuse_unmount() before calling this function.
 *
 * @param f the FUSE handle
 */
void fuse_destroy(struct fuse *f);

/**
 * Exit from event loop
 *
 * @param f the FUSE handle
 */
void fuse_exit(struct fuse *f);

void fuse_config_set_entry_timeout(struct fuse  *fuse_,
                                   const double  entry_timeout_);
void fuse_config_set_negative_entry_timeout(struct fuse  *fuse_,
                                            const double  entry_timeout_);
void fuse_config_set_attr_timeout(struct fuse  *fuse_,
                                  const double  attr_timeout_);

int    fuse_config_num_threads(const struct fuse *fuse_);
double fuse_config_get_entry_timeout(const struct fuse *fuse_);
double fuse_config_get_negative_entry_timeout(const struct fuse *fuse_);
double fuse_config_get_attr_timeout(const struct fuse *fuse_);

/**
 * FUSE event loop with multiple threads
 *
 * Requests from the kernel are processed, and the appropriate
 * operations are called.  Request are processed in parallel by
 * distributing them between multiple threads.
 *
 * Calling this function requires the pthreads library to be linked to
 * the application.
 *
 * @param f the FUSE handle
 * @return 0 if no error occurred, -1 otherwise
 */
int fuse_loop_mt(struct fuse *f);

/**
 * Get the current context
 *
 * The context is only valid for the duration of a filesystem
 * operation, and thus must not be stored and used later.
 *
 * @return the context
 */
struct fuse_context *fuse_get_context(void);

/**
 * Check if the current request has already been interrupted
 *
 * @return 1 if the request has been interrupted, 0 otherwise
 */
int fuse_interrupted(void);

/**
 * The real main function
 *
 * Do not call this directly, use fuse_main()
 */
int fuse_main_real(int argc, char *argv[], const struct fuse_operations *op,
		   size_t op_size, void *user_data);

/**
 * Start the cleanup thread when using option "remember".
 *
 * This is done automatically by fuse_loop_mt()
 * @param fuse struct fuse pointer for fuse instance
 * @return 0 on success and -1 on error
 */
int fuse_start_cleanup_thread(struct fuse *fuse);

/**
 * Stop the cleanup thread when using option "remember".
 *
 * This is done automatically by fuse_loop_mt()
 * @param fuse struct fuse pointer for fuse instance
 */
void fuse_stop_cleanup_thread(struct fuse *fuse);

/**
 * Iterate over cache removing stale entries
 * use in conjunction with "-oremember"
 *
 * NOTE: This is already done for the standard sessions
 *
 * @param fuse struct fuse pointer for fuse instance
 * @return the number of seconds until the next cleanup
 */
int fuse_clean_cache(struct fuse *fuse);

/*
 * Stacking API
 */

/**
 * Fuse filesystem object
 *
 * This is opaque object represents a filesystem layer
 */
struct fuse_fs;

/*
 * These functions call the relevant filesystem operation, and return
 * the result.
 *
 * If the operation is not defined, they return -ENOSYS, with the
 * exception of fuse_fs_open, fuse_fs_release, fuse_fs_opendir,
 * fuse_fs_releasedir and fuse_fs_statfs, which return 0.
 */

int fuse_fs_getattr(struct fuse_fs *fs, const char *path, struct stat *buf);
int fuse_fs_fgetattr(struct fuse_fs *fs, const char *path, struct stat *buf,
		     struct fuse_file_info *fi);
int fuse_fs_rename(struct fuse_fs *fs, const char *oldpath,
		   const char *newpath);
int fuse_fs_unlink(struct fuse_fs *fs, const char *path);
int fuse_fs_rmdir(struct fuse_fs *fs, const char *path);
int fuse_fs_symlink(struct fuse_fs *fs, const char *linkname,
		    const char *path);
int fuse_fs_link(struct fuse_fs *fs, const char *oldpath, const char *newpath);
int fuse_fs_release(struct fuse_fs *fs,	 const char *path,
		    struct fuse_file_info *fi);
int fuse_fs_open(struct fuse_fs *fs, const char *path,
		 struct fuse_file_info *fi);
int fuse_fs_read(struct fuse_fs *fs, const char *path, char *buf, size_t size,
		 off_t off, struct fuse_file_info *fi);
int fuse_fs_read_buf(struct fuse_fs *fs, const char *path,
		     struct fuse_bufvec **bufp, size_t size, off_t off,
		     struct fuse_file_info *fi);
int fuse_fs_write(struct fuse_fs *fs, const char *path, const char *buf,
		  size_t size, off_t off, struct fuse_file_info *fi);
int fuse_fs_write_buf(struct fuse_fs *fs, const char *path,
		      struct fuse_bufvec *buf, off_t off,
		      struct fuse_file_info *fi);
int fuse_fs_fsync(struct fuse_fs *fs, const char *path, int datasync,
		  struct fuse_file_info *fi);
int fuse_fs_flush(struct fuse_fs *fs, const char *path,
		  struct fuse_file_info *fi);
int fuse_fs_statfs(struct fuse_fs *fs, const char *path, struct statvfs *buf);
int fuse_fs_opendir(struct fuse_fs *fs, const char *path,
		    struct fuse_file_info *fi);
int fuse_fs_readdir(struct fuse_fs        *fs,
		    struct fuse_file_info *fi,
                    fuse_dirents_t        *buf);
int fuse_fs_fsyncdir(struct fuse_fs *fs, const char *path, int datasync,
		     struct fuse_file_info *fi);
int fuse_fs_releasedir(struct fuse_fs *fs, const char *path,
		       struct fuse_file_info *fi);
int fuse_fs_create(struct fuse_fs *fs, const char *path, mode_t mode,
		   struct fuse_file_info *fi);
int fuse_fs_lock(struct fuse_fs *fs, const char *path,
		 struct fuse_file_info *fi, int cmd, struct flock *lock);
int fuse_fs_flock(struct fuse_fs *fs, const char *path,
		  struct fuse_file_info *fi, int op);
int fuse_fs_chmod(struct fuse_fs *fs, const char *path, mode_t mode);
int fuse_fs_chown(struct fuse_fs *fs, const char *path, uid_t uid, gid_t gid);
int fuse_fs_truncate(struct fuse_fs *fs, const char *path, off_t size);
int fuse_fs_ftruncate(struct fuse_fs *fs, const char *path, off_t size,
		      struct fuse_file_info *fi);
int fuse_fs_utimens(struct fuse_fs *fs, const char *path,
		    const struct timespec tv[2]);
int fuse_fs_access(struct fuse_fs *fs, const char *path, int mask);
int fuse_fs_readlink(struct fuse_fs *fs, const char *path, char *buf,
		     size_t len);
int fuse_fs_mknod(struct fuse_fs *fs, const char *path, mode_t mode,
		  dev_t rdev);
int fuse_fs_mkdir(struct fuse_fs *fs, const char *path, mode_t mode);
int fuse_fs_setxattr(struct fuse_fs *fs, const char *path, const char *name,
		     const char *value, size_t size, int flags);
int fuse_fs_getxattr(struct fuse_fs *fs, const char *path, const char *name,
		     char *value, size_t size);
int fuse_fs_listxattr(struct fuse_fs *fs, const char *path, char *list,
		      size_t size);
int fuse_fs_removexattr(struct fuse_fs *fs, const char *path,
			const char *name);
int fuse_fs_bmap(struct fuse_fs *fs, const char *path, size_t blocksize,
		 uint64_t *idx);
int fuse_fs_ioctl(struct fuse_fs *fs, const char *path, int cmd, void *arg,
		  struct fuse_file_info *fi, unsigned int flags,
                  void *data, uint32_t *out_bufsz);
int fuse_fs_poll(struct fuse_fs *fs, const char *path,
		 struct fuse_file_info *fi, struct fuse_pollhandle *ph,
		 unsigned *reventsp);
int fuse_fs_fallocate(struct fuse_fs *fs, const char *path, int mode,
		 off_t offset, off_t length, struct fuse_file_info *fi);
void fuse_fs_init(struct fuse_fs *fs, struct fuse_conn_info *conn);
void fuse_fs_destroy(struct fuse_fs *fs);

int fuse_fs_prepare_hide(struct fuse_fs *fs, const char *path, uint64_t *fh);
int fuse_fs_free_hide(struct fuse_fs *fs, uint64_t fh);
ssize_t fuse_fs_copy_file_range(struct fuse_fs *fs,
                                const char *path_in,
                                struct fuse_file_info *fi_in, off_t off_in,
                                const char *path_out,
                                struct fuse_file_info *fi_out, off_t off_out,
                                size_t len, int flags);

int fuse_notify_poll(struct fuse_pollhandle *ph);

/**
 * Create a new fuse filesystem object
 *
 * This is usually called from the factory of a fuse module to create
 * a new instance of a filesystem.
 *
 * @param op the filesystem operations
 * @param op_size the size of the fuse_operations structure
 * @param user_data user data supplied in the context during the init() method
 * @return a new filesystem object
 */
struct fuse_fs *fuse_fs_new(const struct fuse_operations *op, size_t op_size,
			    void *user_data);

/**
 * Filesystem module
 *
 * Filesystem modules are registered with the FUSE_REGISTER_MODULE()
 * macro.
 *
 * If the "-omodules=modname:..." option is present, filesystem
 * objects are created and pushed onto the stack with the 'factory'
 * function.
 */
struct fuse_module {
	/**
	 * Name of filesystem
	 */
	const char *name;

	/**
	 * Factory for creating filesystem objects
	 *
	 * The function may use and remove options from 'args' that belong
	 * to this module.
	 *
	 * For now the 'fs' vector always contains exactly one filesystem.
	 * This is the filesystem which will be below the newly created
	 * filesystem in the stack.
	 *
	 * @param args the command line arguments
	 * @param fs NULL terminated filesystem object vector
	 * @return the new filesystem object
	 */
	struct fuse_fs *(*factory)(struct fuse_args *args,
				   struct fuse_fs *fs[]);

	struct fuse_module *next;
	struct fusemod_so *so;
	int ctr;
};

/**
 * Register a filesystem module
 *
 * This function is used by FUSE_REGISTER_MODULE and there's usually
 * no need to call it directly
 */
void fuse_register_module(struct fuse_module *mod);

/**
 * Register filesystem module
 *
 * For the parameters, see description of the fields in 'struct
 * fuse_module'
 */
#define FUSE_REGISTER_MODULE(name_, factory_)				  \
	static __attribute__((constructor)) void name_ ## _register(void) \
	{								  \
		static struct fuse_module mod =				  \
			{ #name_, factory_, NULL, NULL, 0 };		  \
		fuse_register_module(&mod);				  \
	}


/* ----------------------------------------------------------- *
 * Advanced API for event handling, don't worry about this...  *
 * ----------------------------------------------------------- */

/* NOTE: the following functions are deprecated, and will be removed
   from the 3.0 API.  Use the lowlevel session functions instead */

/** Function type used to process commands */
typedef void (*fuse_processor_t)(struct fuse *, struct fuse_cmd *, void *);

/** This is the part of fuse_main() before the event loop */
struct fuse *fuse_setup(int argc, char *argv[],
			const struct fuse_operations *op, size_t op_size,
			char **mountpoint,
			void *user_data);

/** This is the part of fuse_main() after the event loop */
void fuse_teardown(struct fuse *fuse, char *mountpoint);

/** Multi threaded event loop, which calls the custom command
    processor function */
int fuse_loop_mt_proc(struct fuse *f, fuse_processor_t proc, void *data);

/** Return the exited flag, which indicates if fuse_exit() has been
    called */
int fuse_exited(struct fuse *f);

/** This function is obsolete and implemented as a no-op */
void fuse_set_getcontext_func(struct fuse_context *(*func)(void));

/** Get session from fuse object */
struct fuse_session *fuse_get_session(struct fuse *f);

#ifdef __cplusplus
}
#endif

#endif /* _FUSE_H_ */
