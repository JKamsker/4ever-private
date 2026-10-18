// TFile.h: file system access with the game's paths.
//
// The game names its files with '\' separators and in any case ("Data\Mesh\X.TMH" for
// "data/mesh/x.tmh" on disk). Windows takes those paths as they are. On the other systems
// CTPath::Resolve turns them into real ones: '/' separators, and each name in the case it has
// on disk, found case-insensitively in a listing of its folder that is kept until the folder
// changes. The functions below, CFile and CFileFind open their paths through it.
//////////////////////////////////////////////////////////////////////

#pragma once

#include <stdio.h>

class CTPath
{
public:
	// The path to give the system for a game path. Names that are not on disk are kept as given
	// (the file is about to be created, or does not exist).
	static CString Resolve( LPCTSTR lpszPath);

	// Resolve() is on by default where file names are case sensitive (not on Windows).
	static void EnableResolve( BOOL bEnable);
	static BOOL IsResolveEnabled();

	// Forgets the folder listings (they are also refreshed when a folder changes).
	static void ClearCache();
};

// fopen; the file stays open to other readers and writers, like CFile's shareDenyNone.
FILE *TFileOpen( LPCTSTR lpszPath, LPCTSTR lpszMode);

// A file or folder of that name exists.
BOOL TFileExists( LPCTSTR lpszPath);

BOOL TFileDelete( LPCTSTR lpszPath);
BOOL TFileRename( LPCTSTR lpszOldPath, LPCTSTR lpszNewPath);

// Creates the folder; TRUE when it exists afterwards.
BOOL TCreateFolder( LPCTSTR lpszPath);
