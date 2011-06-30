/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 1991-2008 OpenCFD Ltd.
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

Description
    MS Windows specific functions

\*---------------------------------------------------------------------------*/

#include "OSspecific.H"
#include "MSwindows.H"
#include "foamVersion.H"
#include "fileName.H"
#include "fileStat.H"

#include <cassert>
#include <cstdlib>
#include <fstream>

// Windows system header files
#include <io.h> // _close
#include <windows.h>
#include <signal.h>


// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

defineTypeNameAndDebug(Foam::MSwindows, 0);


// Don't abort under windows, causes abort dialog to
// popup. Instead just exit with exitCode.
static
void sigAbortHandler(int exitCode)
{
  ::exit(exitCode);
}


static
bool installAbortHandler()
{
  // If it didn't succeed there's not much we can do,
  // so don't check result.
  ::signal(SIGABRT, &sigAbortHandler);
  return true;
}


static bool const abortHandlerInstalled = installAbortHandler();


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //


//- Get last windows api error from GetLastError
Foam::string Foam::MSwindows::getLastError()
{
    // Based on an example at:
    // http://msdn2.microsoft.com/en-us/library/ms680582(VS.85).aspx

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    lpDisplayBuf = LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen(static_cast<LPCTSTR>(lpMsgBuf))+40)*sizeof(TCHAR)); 
    sprintf(static_cast<LPTSTR>(lpDisplayBuf),
            "Error %d: %s", int(dw), static_cast<LPCTSTR>(lpMsgBuf));

    const Foam::string errorMessage = static_cast<LPTSTR>(lpDisplayBuf);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);

    return errorMessage;
}


//-Declared here to avoid polluting MSwindows.H with windows.h
namespace Foam
{
namespace MSwindows
{
    //- Get windows user name
    Foam::string getUserName();

    //- Remove quotes, if any, from string
    void removeQuotes(Foam::string & arg);

    //- Convert windows directory slash (back-slash) to unix (forward-slash). 
    //- Windows is fine with unix like directory slashes.
    //- Foam's file io (see src/OpenFOAM/db/IOstreams/Sstreams/OSwrite.C) 
    //- uses back-slash as escape character and continuation, 
    //- so not an option to have windows file paths with back-slashes
    void toUnixSlash(Foam::string & arg);

    //- Auto create and then delete array when this goes out of scope
    template<class T>
    class AutoArray
    {
      T* const array_;

    public:
      AutoArray(const unsigned long arrayLength);
      ~AutoArray();

      //- Access array
      T* get();
    }; // class AutoArray


    //- Directory contents iterator
    class DirectoryIterator
    {
      WIN32_FIND_DATA findData_;
      HANDLE findHandle_;
      fileName nextName_;
      bool hasMore_;
      
    public:
      DirectoryIterator(const fileName & directory);
      ~DirectoryIterator();
      
      //- Initialization succeeded
      bool isValid() const;

      //- Has more?
      bool hasNext() const;
      
      //- Next item
      const fileName & next();
    }; // class DirectoryIterator
} // namespace MSwindows
} // namespace Foam


inline
void Foam::MSwindows::removeQuotes(Foam::string & arg)
{
    std::size_t pos;

    while (std::string::npos != (pos = arg.find('"')))
    {
        arg.erase(pos, 1);
    }
}


inline
void Foam::MSwindows::toUnixSlash(Foam::string & arg)
{
    arg.replaceAll("\\", "/");

    const string UNC("//");

    // Preserve UNC i.e., \\machine-name\...
    if (0 == arg.find(UNC)) 
    {
        arg.replace(UNC, "\\\\");
    }
}


Foam::string Foam::MSwindows::getUserName()
{
    const DWORD bufferSize = 256;
    TCHAR buffer[bufferSize];
    DWORD actualBufferSize = bufferSize;
    Foam::string nameAsString;

    bool success = ::GetUserName(buffer, &actualBufferSize);

    if (success)
    {
        nameAsString = buffer;
    }
    else 
    {
        if (ERROR_INSUFFICIENT_BUFFER == ::GetLastError() &&
            32768 > actualBufferSize) 
        {
            AutoArray<TCHAR> actualBuffer(actualBufferSize);
            ::GetUserName(actualBuffer.get(), &actualBufferSize);
            nameAsString = actualBuffer.get();
        }
    }

    return nameAsString;
}


template<class T>
inline
Foam::MSwindows::AutoArray<T>::AutoArray(const unsigned long arrayLength)
    : array_(new T[arrayLength])
{}


template<class T>
inline
Foam::MSwindows::AutoArray<T>::~AutoArray()
{
    delete [] array_;
}


template<class T>
inline
T* Foam::MSwindows::AutoArray<T>::get()
{
    return array_;
}


inline
bool Foam::MSwindows::DirectoryIterator::isValid() const
{
    const bool valid = (INVALID_HANDLE_VALUE != findHandle_);
    return valid;
}

    
Foam::MSwindows::DirectoryIterator::DirectoryIterator(const fileName & directory)
{
    const fileName directoryContents = directory/"*";
    findHandle_ = ::FindFirstFile(directoryContents.c_str(), &findData_);
    hasMore_    = isValid();
}
        

Foam::MSwindows::DirectoryIterator::~DirectoryIterator()
{
    if (isValid()) 
    {
        ::FindClose(findHandle_);
    }
}


inline
bool Foam::MSwindows::DirectoryIterator::hasNext() const
{
    assert(isValid());

    return hasMore_;
}


inline
const Foam::fileName & Foam::MSwindows::DirectoryIterator::next()
{
    assert(hasNext());

    nextName_ = findData_.cFileName;
    hasMore_  = ::FindNextFile(findHandle_, &findData_);

    return nextName_;
}


pid_t Foam::pid()
{
#ifdef WIN32
    const DWORD processId = ::GetCurrentProcessId();
#elif WIN64
    const pid_t processId = (pid_t) ::GetCurrentProcessId();
#endif
    return processId;
}


pid_t Foam::ppid()
{
    // No equivalent under windows.

    if (MSwindows::debug)
    {
        Info<< "ppid not supported under MSwindows" << endl;
    }

    return 0;
}


pid_t Foam::pgid()
{
    // No equivalent under windows.

    if (MSwindows::debug)
    {
        Info<< "pgid not supported under MSwindows" << endl;
    }

    return 0;
}


bool Foam::env(const word& envName)
{
    const DWORD actualBufferSize = 
      ::GetEnvironmentVariable(envName.c_str(), NULL, 0);

    const bool envExists = (0 < actualBufferSize);
    return envExists;
}


Foam::string Foam::getEnv(const word& envName)
{
    Foam::string envAsString;

    const DWORD actualBufferSize = 
      ::GetEnvironmentVariable(envName.c_str(), NULL, 0);

    if (0 < actualBufferSize) 
    {
        MSwindows::AutoArray<TCHAR> actualBuffer(actualBufferSize);
        ::GetEnvironmentVariable(envName.c_str(),
                                 actualBuffer.get(),
                                 actualBufferSize);
        envAsString = actualBuffer.get();
        toUnixPath(envAsString);
    }

    return envAsString;
}


bool Foam::setEnv
(
    const word& envName,
    const std::string& value,
    const bool /*overwrite*/
)
{
    const bool success = 
      ::SetEnvironmentVariable(envName.c_str(), value.c_str());
    return success;
}


Foam::word Foam::hostName()
{
    const DWORD bufferSize = MAX_COMPUTERNAME_LENGTH + 1;
    TCHAR buffer[bufferSize];
    DWORD actualBufferSize = bufferSize;

    const bool success = 
      ::GetComputerName(buffer, &actualBufferSize);
    const word computerName = success ? buffer : word::null;
    return computerName;
}


Foam::word Foam::userName()
{
    word nameAsWord = getEnv("USERNAME");

    if (nameAsWord.empty()) 
    {
        nameAsWord = MSwindows::getUserName();
    }

    return nameAsWord;
}


Foam::fileName Foam::home()
{
    Foam::string homeDir = getEnv("HOME");

    if (homeDir.empty()) 
    {
        homeDir = getEnv("USERPROFILE");
    }

    return homeDir;
}


Foam::fileName Foam::home(const word& userName)
{
    return home();
}


Foam::fileName Foam::cwd()
{
    Foam::string currentDirectory;

    const DWORD actualBufferSize = 
      ::GetCurrentDirectory(0, NULL);

    if (0 < actualBufferSize) 
    {
        MSwindows::AutoArray<TCHAR> actualBuffer(actualBufferSize);
        ::GetCurrentDirectory(actualBufferSize,
                              actualBuffer.get());   
        currentDirectory = actualBuffer.get();
        MSwindows::toUnixSlash(currentDirectory);
    }
    else 
    {
        FatalErrorIn("cwd()")
            << "Couldn't get the current working directory"
            << exit(FatalError);
    }

    return currentDirectory;
}


bool Foam::chDir(const fileName& dir)
{
    const bool success = ::SetCurrentDirectory(dir.c_str());
    return success; 
}


Foam::fileName Foam::findEtcFile(const fileName& name, bool mandatory)
{
    // Search user files:
    // ~~~~~~~~~~~~~~~~~~
    fileName searchDir = home()/".OpenFOAM";
    if (isDir(searchDir))
    {
        // Check for user file in ~/.OpenFOAM/VERSION
        fileName fullName = searchDir/FOAMversion/name;
        if (isFile(fullName))
        {
            return fullName;
        }

        // Check for version-independent user file in ~/.OpenFOAM
        fullName = searchDir/name;
        if (isFile(fullName))
        {
            return fullName;
        }
    }


    // Search site files:
    // ~~~~~~~~~~~~~~~~~~
    searchDir = getEnv("WM_PROJECT_INST_DIR");
    if (isDir(searchDir))
    {
        // Check for site file in $WM_PROJECT_INST_DIR/site/VERSION
        fileName fullName = searchDir/"site"/FOAMversion/name;
        if (isFile(fullName))
        {
            return fullName;
        }

        // Check for version-independent site file in $WM_PROJECT_INST_DIR/site
        fullName = searchDir/"site"/name;
        if (isFile(fullName))
        {
            return fullName;
        }
    }

    // Search installation files:
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~
    searchDir = getEnv("WM_PROJECT_DIR");
    if (isDir(searchDir))
    {
        // Check for shipped OpenFOAM file in $WM_PROJECT_DIR/etc
        fileName fullName = searchDir/"etc"/name;
        if (isFile(fullName))
        {
            return fullName;
        }
    }

    // Not found
    // abort if the file is mandatory, otherwise return null
    if (mandatory)
    {
        cerr<< "--> FOAM FATAL ERROR in Foam::findEtcFile() :"
               " could not find mandatory file\n    '"
            << name.c_str() << "'\n\n" << std::endl;
        ::exit(1);
    }

    // Return null-constructed fileName rather than fileName::null
    // to avoid cyclic dependencies in the construction of globals
    return fileName();
}


bool Foam::mkDir(const fileName& pathName, const mode_t mode)
{
    if (pathName.empty())
    {
        return false;
    }


    bool success = ::CreateDirectory(pathName.c_str(), NULL);

    if (success)
    {
        Foam::chMod(pathName, mode);
    }
    else 
    {
        const DWORD error = ::GetLastError();

        switch (error)
        {
            case ERROR_ALREADY_EXISTS:
            {
                success = true;
                break;
            }
            case ERROR_PATH_NOT_FOUND:
            {
                // Part of the path does not exist so try to create it
                const fileName& parentName = pathName.path();

                if (parentName.size() && mkDir(parentName, mode))
                {
                    success = mkDir(pathName, mode);
                }
                
                break;
            }  
        }

        if (!success) 
        {
            FatalErrorIn("mkDir(const fileName&, mode_t)")
              << "Couldn't create directory: " << pathName
              << " " << MSwindows::getLastError()
              << exit(FatalError);
        }
    }

    return success;
}


// Set the file mode
bool Foam::chMod(const fileName& name, const mode_t m)
{
    const int success = _chmod(name.c_str(), m);
    return success;
}


// Return the file mode
mode_t Foam::mode(const fileName& name)
{
    fileStat fileStatus(name);

    const mode_t m = fileStatus.isValid() ?
      fileStatus.status().st_mode : 0;
    return m;
}


// Return the file type: FILE or DIRECTORY
Foam::fileName::Type Foam::type(const fileName& name)
{
    fileName::Type fileType = fileName::UNDEFINED;
    const DWORD attrs = ::GetFileAttributes(name.c_str());

    if (attrs != INVALID_FILE_ATTRIBUTES) 
    {
        fileType = (attrs & FILE_ATTRIBUTE_DIRECTORY) ?
	  fileName::DIRECTORY :
	  fileName::FILE;
    }

    return fileType;
}


static
bool 
isGzFile(const Foam::fileName& name)
{
    Foam::string gzName(name);
    gzName += ".gz";
    const DWORD attrs = ::GetFileAttributes(gzName.c_str());
    const bool success = (attrs != INVALID_FILE_ATTRIBUTES);

    return success;
}


// Does the name exist in the filing system?
bool Foam::exists(const fileName& name, const bool checkGzip)
{
    const DWORD attrs = ::GetFileAttributes(name.c_str());
    const bool success = (attrs != INVALID_FILE_ATTRIBUTES) || 
                         (checkGzip && isGzFile(name));

    return success;
}


// Does the directory exist
bool Foam::isDir(const fileName& name)
{
    const DWORD attrs = ::GetFileAttributes(name.c_str());
    bool success = (attrs != INVALID_FILE_ATTRIBUTES) &&
                   (attrs & FILE_ATTRIBUTE_DIRECTORY);

    return success;
}


// Does the file exist
bool Foam::isFile(const fileName& name, const bool checkGzip)
{
    const DWORD attrs = ::GetFileAttributes(name.c_str());
    const bool success = ((attrs != INVALID_FILE_ATTRIBUTES) && 
			  !(attrs & FILE_ATTRIBUTE_DIRECTORY)) || 
                         (checkGzip && isGzFile(name));

    return success;
}


// Return size of file
off_t Foam::fileSize(const fileName& name)
{
    fileStat fileStatus(name);

    const off_t fileSize = fileStatus.isValid() ?
      fileStatus.status().st_size : -1;
    return fileSize;
}


// Return time of last file modification
time_t Foam::lastModified(const fileName& name)
{
    fileStat fileStatus(name);

    const time_t modifiedTime = fileStatus.isValid() ?
      fileStatus.status().st_mtime : 0;
    return modifiedTime;
}


// Read a directory and return the entries as a string list
Foam::fileNameList Foam::readDir
(
    const fileName& directory,
    const fileName::Type type,
    const bool filtergz
)
{
    // Initial filename list size
    // also used as increment if initial size found to be insufficient
    const int maxNnames = 100;

    if (MSwindows::debug)
    {
        Info<< "readDir(const fileName&, const fileType, const bool filtergz)"
            << " : reading directory " << directory << endl;
    }

    // Setup empty string list MAXTVALUES long
    fileNameList dirEntries(maxNnames);

    // Temporary variables and counters
    label nEntries = 0;

    MSwindows::DirectoryIterator dirIt(directory);

    if (dirIt.isValid())
    {
        while (dirIt.hasNext())
        {
            const fileName & fName = dirIt.next();

            // ignore files begining with ., i.e. '.', '..' and '.*'
            if (fName.size() > 0 && fName[size_t(0)] != '.')
            {
                word fileNameExt = fName.ext();

                if
                (
                    (type == fileName::DIRECTORY)
                 ||
                    (
                        type == fileName::FILE
                        && fName[fName.size()-1] != '~'
                        && fileNameExt != "bak"
                        && fileNameExt != "BAK"
                        && fileNameExt != "old"
                        && fileNameExt != "save"
                    )
                )
                {
                    if ((directory/fName).type() == type)
                    {
                        if (nEntries >= dirEntries.size())
                        {
                            dirEntries.setSize(dirEntries.size() + maxNnames);
                        }

                        if (filtergz && fileNameExt == "gz")
                        {
                            dirEntries[nEntries++] = fName.lessExt();
                        }
                        else
                        {
                            dirEntries[nEntries++] = fName;
                        }
                    }
                }
            }
        }
    }
    else if (MSwindows::debug)
    {
        Info<< "readDir(const fileName&, const fileType, "
               "const bool filtergz) : cannot open directory "
            << directory << endl;
    }

    // Reset the length of the entries list
    dirEntries.setSize(nEntries);
    
    return dirEntries;
}


// Copy, recursively if necessary, the source top the destination
bool Foam::cp(const fileName& src, const fileName& dest)
{
    // Make sure source exists.
    if (!exists(src))
    {
        return false;
    }

    fileName destFile(dest);

    // Check type of source file.
    if (src.type() == fileName::FILE)
    {
        // If dest is a directory, create the destination file name.
        if (destFile.type() == fileName::DIRECTORY)
        {
            destFile = destFile/src.name();
        }

        // Make sure the destination directory exists.
        if (!isDir(destFile.path()) && !mkDir(destFile.path()))
        {
            return false;
        }

        // Open and check streams.
        // Use binary mode in case we read binary.
        // Causes windows reading to fail if we don't.
        std::ifstream srcStream(src.c_str(), 
                                ios_base::in|ios_base::binary);      
        if (!srcStream) 
        {
            return false;
        }

        // Use binary mode in case we write binary.
        // Causes windows reading to fail if we don't.
        std::ofstream destStream(destFile.c_str(), 
                                 ios_base::out|ios_base::binary);
        if (!destStream)
        {
            return false;
        }

        // Copy character data.
        char ch;
        while (srcStream.get(ch))
        {
            destStream.put(ch);
        }

        // Final check.
        if (!srcStream.eof() || !destStream)
        {
            return false;
        }
    }
    else if (src.type() == fileName::DIRECTORY)
    {
        // If dest is a directory, create the destination file name.
        if (destFile.type() == fileName::DIRECTORY)
        {
            destFile = destFile/src.component(src.components().size() -1);
        }

        // Make sure the destination directory extists.
        if (!isDir(destFile) && !mkDir(destFile))
        {
            return false;
        }

        // Copy files
        fileNameList contents = readDir(src, fileName::FILE, false);
        forAll(contents, i)
        {
            if (MSwindows::debug)
            {
                Info<< "Copying : " << src/contents[i] 
                    << " to " << destFile/contents[i] << endl;
            }

            // File to file.
            cp(src/contents[i], destFile/contents[i]);
        }

        // Copy sub directories.
        fileNameList subdirs = readDir(src, fileName::DIRECTORY);
        forAll(subdirs, i)
        {
            if (MSwindows::debug)
            {
                Info<< "Copying : " << src/subdirs[i]
                    << " to " << destFile << endl;
            }

            // Dir to Dir.
            cp(src/subdirs[i], destFile);
        }
    }

    return true;
}


// Create a softlink. destFile should not exist. Returns true if successful.
bool Foam::ln(const fileName& src, const fileName& dest)
{
    // Seems that prior to Vista softlinking was poorly supported.
    // Vista does a better job, but requires adminstrator privileges.
    // Skip for now.

    if (MSwindows::debug)
    {
        Info<< "MSwindows does not support ln - softlinking" << endl;
    }

    return false;
}


// Rename srcFile destFile
bool Foam::mv(const fileName& srcFile, const fileName& destFile)
{
    if (MSwindows::debug)
    {
        Info<< "Move : " << srcFile << " to " << destFile << endl;
    }

    const fileName destName = 
      ((destFile.type() == fileName::DIRECTORY)
       && (srcFile.type() != fileName::DIRECTORY)) ?
      destFile/srcFile.name() :
      destFile;

    const bool success = 
      (0 == std::rename(srcFile.c_str(), destName.c_str()));

    return success;
}


//- Rename to a corresponding backup file
//  If the backup file already exists, attempt with "01" .. "99" index
bool Foam::mvBak(const fileName& src, const std::string& ext)
{
    if (MSwindows::debug)
    {
        Info<< "mvBak : " << src << " to extension " << ext << endl;
    }

    if (exists(src, false))
    {
        const int maxIndex = 99;
        char index[3];

        for (int n = 0; n <= maxIndex; n++)
        {
            fileName dstName(src + "." + ext);
            if (n)
            {
                sprintf(index, "%02d", n);
                dstName += index;
            }

            // avoid overwriting existing files, except for the last
            // possible index where we have no choice
            if (!exists(dstName, false) || n == maxIndex)
            {
                return (0 == std::rename(src.c_str(), dstName.c_str()));
            }

        }
    }

    // fall-through: nothing to do
    return false;
}


// Remove a file returning true if successful otherwise false
bool Foam::rm(const fileName& file)
{
    if (MSwindows::debug)
    {
        Info<< "Removing : " << file << endl;
    }

    bool success = (0 == std::remove(file.c_str()));

    // If deleting plain file name failed try with .gz
    if (!success) 
    {
        const Foam::string fileGz = file + ".gz";
        success = (0 == std::remove(fileGz.c_str()));
    }

    return success;
}


// Remove a dirctory and it's contents
bool Foam::rmDir(const fileName& directory)
{
    if (MSwindows::debug)
    {
        Info<< "rmdir(const fileName&) : "
            << "removing directory " << directory << endl;
    }

    bool success = true;

    // Need to destroy DirectorIterator prior to
    // removing directory otherwise fails on Windows XP
    {
      MSwindows::DirectoryIterator dirIt(directory);

      while (success && dirIt.hasNext())
      {
          const fileName & fName = dirIt.next(); 

          if (fName != "." && fName != "..")
          {
              fileName path = directory/fName;

              if (path.type() == fileName::DIRECTORY)
              {
                  success = rmDir(path);

                  if (!success)
                  {
                      WarningIn("rmdir(const fileName&)")
                        << "failed to remove directory " << fName
                        << " while removing directory " << directory
                        << endl;
                  }
              }
              else
              {
                  success = rm(path);

                  if (!success)
                  {
                      WarningIn("rmdir(const fileName&)")
                        << "failed to remove file " << fName
                        << " while removing directory " << directory
                        << endl;
                  }
              }
          }
      }
    }
        
    if (success) 
    {
        success = ::RemoveDirectory(directory.c_str());

        if (!success) 
        {
            WarningIn("rmdir(const fileName&)")
                << "failed to remove directory " << directory << endl;
        }
    }

    return success;
}


//- Sleep for the specified number of seconds
unsigned int Foam::sleep(const unsigned int s)
{
    const DWORD milliseconds = s * 1000;

    ::Sleep(milliseconds);

    return 0;
}


void Foam::fdClose(const int fd)
{
    const int result = ::_close(fd);

    if (0 != result)
    {
        FatalErrorIn
        (
            "Foam::fdClose(const int fd)"
        )   << "close error on " << fd << endl
            << abort(FatalError);    
    }
}


//- Check if machine is up by pinging given port
bool Foam::ping
(
    const word& destName,
    const label destPort,
    const label timeOut
)
{
    // Appears that socket calls require adminstrator privileges.
    // Skip for now.

    if (MSwindows::debug)
    {
        Info<< "MSwindows does not support ping" << endl;
    }

    return false;
}


//- Check if machine is up by ping port 22 = ssh and 222 = rsh
bool Foam::ping(const word& hostname, const label timeOut)
{
    return ping(hostname, 222, timeOut) || ping(hostname, 22, timeOut);
}


int Foam::system(const std::string& command)
{
    return std::system(command.c_str());
}


//- Open shared library
void* Foam::dlOpen(const fileName& libName)
{
    //Lets check if this is a list of libraries to be loaded
    //NOTE: should only be used for "force loading libraries"
    if (libName.find_first_of(' ')!=Foam::string::npos)
    {
      void *moduleh=NULL;
      Foam::string libsToLoad=libName;
      libsToLoad.removeRepeated(' ');
      libsToLoad.removeTrailing(' '); //removes spaces from both ends
      libsToLoad += ' ';

      if (MSwindows::debug)
      {
          Info<< "Libraries to be loaded: " <<  libsToLoad << endl;
      }

      //generate the word list
      size_t stposstr=0, found=libsToLoad.find_first_of(' ');
      while (found!=Foam::string::npos)
      {
          Foam::string libToLoad = libsToLoad.substr(stposstr,found-stposstr);
          moduleh = openLibrary(libToLoad); //FIX: module handle is ignored and maybe it shouldn't
          stposstr=found+1; found=libsToLoad.find_first_of(' ',stposstr);
      }

      return moduleh;
    }
    else
    {
      const char* dllExt = ".dll";

      // Assume libName is of the form, lib<name>.so
      Foam::string winLibName(libName);
      winLibName.replace(".so", dllExt);
      void* libHandle = ::LoadLibrary(winLibName.c_str());

      if (NULL == libHandle)
      {
          // Assumes libName = name
          winLibName = "lib";
          winLibName += libName;
          winLibName += dllExt;
        
          libHandle = ::LoadLibrary(winLibName.c_str());
      }
      
      if (NULL == libHandle) 
      {
          WarningIn("openLibrary(const fileName& libName)")
            << "LoadLibrary failed. "
            << MSwindows::getLastError()
            << endl;
      }
      
      if (MSwindows::debug)
      {
          Info<< "Library " <<  libName << " loaded "
              << (libHandle != NULL ? "with success!" : "without success.")
              << endl;
      }

      return libHandle;
    }
}


//- Close shared library
void Foam::dlClose(void* libHandle)
{
    const bool success = 
      ::FreeLibrary(static_cast<HMODULE>(libHandle));
  
    if (!success) 
    {
        WarningIn("closeLibrary(void* const libHandle)")
            << "FreeLibrary failed. " 
            << MSwindows::getLastError()
            << endl;
    }
}


//- Lookup a symbol in a dlopened library using handle to library
void* Foam::dlSym(void* handle, const std::string& symbol)
{
    if (MSwindows::debug)
    {
        std::cout
            << "dlSym(void*, const std::string&)"
            << " : dlsym of " << symbol << std::endl;
    }

    // get address of symbol
    void* fun = ::GetProcAddress(handle, symbol.c_str());

    if(fun == NULL)
    {
        WarningIn("dlSym(void*, const std::string&)")
            << "Cannot lookup symbol " << symbol << " : " << MSwindows::getLastError()
            << endl;
    }

    return fun;
}


//- Report if symbol in a dlopened library could be found
bool Foam::dlSymFound(void* handle, const std::string& symbol)
{
    if (handle && !symbol.empty())
    {
        if (MSwindows::debug)
        {
            std::cout
                << "dlSymFound(void*, const std::string&)"
                << " : dlsym of " << symbol << std::endl;
        }

        // symbol can be found if there was no error
        return dlSym(handle, symbol.c_str()) != NULL;
    }
    else
    {
        return false;
    }
}


static int collectLibsCallback
(
    struct dl_phdr_info *info,
    size_t size,
    void *data
)
{
    Foam::DynamicList<Foam::fileName>* ptr =
        reinterpret_cast<Foam::DynamicList<Foam::fileName>*>(data);
    ptr->append(info->dlpi_name);
    return 0;
}


//- Return all loaded libraries
Foam::fileNameList Foam::dlLoaded()
{
    DynamicList<fileName> libs;
    dl_iterate_phdr(collectLibsCallback, &libs);
    if (POSIX::debug)
    {
        std::cout
            << "dlLoaded()"
            << " : determined loaded libraries :" << libs.size() << std::endl;
    }
    return libs;
}

Foam::string Foam::toUnixPath(const string & path)
{
    string unixPath(path);
    MSwindows::toUnixSlash(unixPath);
    MSwindows::removeQuotes(unixPath);

    return unixPath;
}

//It's easier to include it here...
#include "random.c"

//- Random functions
void Foam::osRandomSeed(const label seed)
{
    srandom((unsigned int)seed);
}

Foam::label Foam::osRandomInteger()
{
    return random();
}

Foam::scalar Foam::osRandomDouble()
{
    return (scalar)random();
}


// ************************************************************************* //
