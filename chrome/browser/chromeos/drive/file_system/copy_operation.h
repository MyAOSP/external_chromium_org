// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DRIVE_FILE_SYSTEM_COPY_OPERATION_H_
#define CHROME_BROWSER_CHROMEOS_DRIVE_FILE_SYSTEM_COPY_OPERATION_H_

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/drive/file_system_interface.h"
#include "chrome/browser/google_apis/gdata_errorcode.h"

namespace base {
class FilePath;
}  // namespace base

namespace google_apis {
class ResourceEntry;
}  // namespace google_apis

namespace drive {

class JobScheduler;
class ResourceEntry;

namespace internal {
class FileCache;
}  // namespace internal

namespace file_system {

class CreateFileOperation;
class MoveOperation;
class OperationObserver;

// This class encapsulates the drive Copy function.  It is responsible for
// sending the request to the drive API, then updating the local state and
// metadata to reflect the new state.
class CopyOperation {
 public:
  CopyOperation(JobScheduler* job_scheduler,
                FileSystemInterface* file_system,
                internal::ResourceMetadata* metadata,
                internal::FileCache* cache,
                scoped_refptr<base::SequencedTaskRunner> blocking_task_runner,
                OperationObserver* observer);
  virtual ~CopyOperation();

  // Performs the copy operation on the file at drive path |src_file_path|
  // with a target of |dest_file_path|. Invokes |callback| when finished with
  // the result of the operation. |callback| must not be null.
  virtual void Copy(const base::FilePath& src_file_path,
                    const base::FilePath& dest_file_path,
                    const FileOperationCallback& callback);

  // Initiates transfer of |remote_src_file_path| to |local_dest_file_path|.
  // |remote_src_file_path| is the virtual source path on the Drive file system.
  // |local_dest_file_path| is the destination path on the local file system.
  //
  // Must be called from *UI* thread. |callback| is run on the calling thread.
  // |callback| must not be null.
  virtual void TransferFileFromRemoteToLocal(
      const base::FilePath& remote_src_file_path,
      const base::FilePath& local_dest_file_path,
      const FileOperationCallback& callback);

  // Initiates transfer of |local_src_file_path| to |remote_dest_file_path|.
  // |local_src_file_path| must be a file from the local file system.
  // |remote_dest_file_path| is the virtual destination path within Drive file
  // system.
  //
  // Must be called from *UI* thread. |callback| is run on the calling thread.
  // |callback| must not be null.
  virtual void TransferFileFromLocalToRemote(
      const base::FilePath& local_src_file_path,
      const base::FilePath& remote_dest_file_path,
      const FileOperationCallback& callback);

 private:
  // Stores |local_file_path| in cache and mark as dirty so that SyncClient will
  // upload the content to |remote_dest_file_path|.
  void ScheduleTransferRegularFile(const base::FilePath& local_file_path,
                                   const base::FilePath& remote_dest_file_path,
                                   const FileOperationCallback& callback);
  void ScheduleTransferRegularFileAfterCreate(
      const base::FilePath& local_file_path,
      const base::FilePath& remote_dest_file_path,
      const FileOperationCallback& callback,
      FileError error);
  void ScheduleTransferRegularFileAfterGetResourceEntry(
      const base::FilePath& local_file_path,
      const FileOperationCallback& callback,
      FileError error,
      scoped_ptr<ResourceEntry> entry);

  // Invoked upon completion of GetFileByPath initiated by
  // TransferFileFromRemoteToLocal. If GetFileByPath reports no error, calls
  // CopyLocalFileOnBlockingPool to copy |local_file_path| to
  // |local_dest_file_path|.
  //
  // Can be called from UI thread. |callback| is run on the calling thread.
  // |callback| must not be null.
  void OnGetFileCompleteForTransferFile(
      const base::FilePath& local_dest_file_path,
      const FileOperationCallback& callback,
      FileError error,
      const base::FilePath& local_file_path,
      scoped_ptr<ResourceEntry> entry);

  // Copies a hosted document with |resource_id| to the directory at |dir_path|
  // and names the copied document as |new_name|.
  //
  // Can be called from UI thread. |callback| is run on the calling thread.
  // |callback| must not be null.
  void CopyHostedDocumentToDirectory(const base::FilePath& dir_path,
                                     const std::string& resource_id,
                                     const base::FilePath::StringType& new_name,
                                     const FileOperationCallback& callback);

  // Callback for handling document copy attempt.
  // |callback| must not be null.
  void OnCopyHostedDocumentCompleted(
      const base::FilePath& dir_path,
      const FileOperationCallback& callback,
      google_apis::GDataErrorCode status,
      scoped_ptr<google_apis::ResourceEntry> resource_entry);

  // Moves a file or directory at |file_path| in the root directory to
  // another directory at |dir_path|. This function does nothing if
  // |dir_path| points to the root directory.
  //
  // Can be called from UI thread. |callback| is run on the calling thread.
  // |callback| must not be null.
  void MoveEntryFromRootDirectory(const base::FilePath& directory_path,
                                  const FileOperationCallback& callback,
                                  FileError error,
                                  const base::FilePath& file_path);

  // Part of Copy(). Called after GetResourceEntryPairByPaths() is
  // complete. |callback| must not be null.
  void CopyAfterGetResourceEntryPair(const base::FilePath& dest_file_path,
                                 const FileOperationCallback& callback,
                                 scoped_ptr<EntryInfoPairResult> result);

  // Invoked upon completion of GetFileByPath initiated by Copy. If
  // GetFileByPath reports no error, calls TransferRegularFile to transfer
  // |local_file_path| to |remote_dest_file_path|.
  //
  // Can be called from UI thread. |callback| is run on the calling thread.
  void OnGetFileCompleteForCopy(const base::FilePath& remote_dest_file_path,
                                const FileOperationCallback& callback,
                                FileError error,
                                const base::FilePath& local_file_path,
                                scoped_ptr<ResourceEntry> entry);

  // Part of TransferFileFromLocalToRemote(). Called after
  // GetResourceEntryByPath() is complete.
  void TransferFileFromLocalToRemoteAfterGetResourceEntry(
      const base::FilePath& local_src_file_path,
      const base::FilePath& remote_dest_file_path,
      const FileOperationCallback& callback,
      FileError error,
      scoped_ptr<ResourceEntry> entry);

  // Initiates transfer of |local_file_path| with |resource_id| to
  // |remote_dest_file_path|. |local_file_path| must be a file from the local
  // file system, |remote_dest_file_path| is the virtual destination path within
  // Drive file system. If |resource_id| is a non-empty string, the transfer is
  // handled by CopyDocumentToDirectory. Otherwise, the transfer is handled by
  // TransferRegularFile.
  //
  // Must be called from *UI* thread. |callback| is run on the calling thread.
  // |callback| must not be null.
  void TransferFileForResourceId(const base::FilePath& local_file_path,
                                 const base::FilePath& remote_dest_file_path,
                                 const FileOperationCallback& callback,
                                 const std::string& resource_id);

  JobScheduler* job_scheduler_;
  FileSystemInterface* file_system_;
  internal::ResourceMetadata* metadata_;
  internal::FileCache* cache_;
  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;
  OperationObserver* observer_;

  // Uploading a new file is internally implemented by creating a dirty file.
  scoped_ptr<CreateFileOperation> create_file_operation_;
  // Copying a hosted document is internally implemented by using a move.
  scoped_ptr<MoveOperation> move_operation_;

  // WeakPtrFactory bound to the UI thread.
  // Note: This should remain the last member so it'll be destroyed and
  // invalidate the weak pointers before any other members are destroyed.
  base::WeakPtrFactory<CopyOperation> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CopyOperation);
};

}  // namespace file_system
}  // namespace drive

#endif  // CHROME_BROWSER_CHROMEOS_DRIVE_FILE_SYSTEM_COPY_OPERATION_H_