/********************************************************************
*
* This file is part of the MFCExt-Library
*
* Copyright (C) 1999-2000 Sven Wiegand
* Copyright (C) 2000-$CurrentYear$ ToolsCenter
* 
* This library is free software; you can redistribute it and/or
* modify, but leave the headers intact and do not remove any 
* copyrights from the source.
*
* This library does not only contain file from us, but also from
* third party developers. Look at the source file headers to get
* detailed information.
*
* If you have further questions visit our homepage
*
*    http://www.ToolsCenter.org
*
********************************************************************/

/********************************************************************
*
* $Id$
*
********************************************************************/

#include "stdafx.h"
#include "FileVersionInfo.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//-------------------------------------------------------------------
// CFileVersionInfo
//-------------------------------------------------------------------

CFileVersionInfo::CFileVersionInfo()
{
	Reset();
}


CFileVersionInfo::CFileVersionInfo(HMODULE hModule, DWORD dwLanguageCodepageId /* = (DWORD)-1 */)
{
	Create(hModule, dwLanguageCodepageId);
}


CFileVersionInfo::CFileVersionInfo(LPCTSTR lpszFileName, DWORD dwLanguageCodepageId /* = (DWORD)-1 */)
{
	Create(lpszFileName, dwLanguageCodepageId);
}


CFileVersionInfo::~CFileVersionInfo()
{}


BOOL CFileVersionInfo::GetTranslationId(LPVOID lpData, UINT unBlockSize, WORD wLangId, DWORD &dwId, BOOL bPrimaryEnough/*= FALSE*/)
{
	LPWORD lpwData = (LPWORD)lpData;
	for (; (LPBYTE)lpwData < ((LPBYTE)lpData)+unBlockSize; lpwData+=2)
	{
		if (*lpwData == wLangId)
		{
			dwId = *((DWORD*)lpwData);
			return TRUE;
		}
	}

	if (!bPrimaryEnough)
		return FALSE;

	for (lpwData = (LPWORD)lpData; (LPBYTE)lpwData < ((LPBYTE)lpData)+unBlockSize; lpwData+=2)
	{
		if (((*lpwData)&0x00FF) == (wLangId&0x00FF))
		{
			dwId = *((DWORD*)lpwData);
			return TRUE;
		}
	}

	return FALSE;
}


BOOL CFileVersionInfo::Create(HMODULE hModule /*= NULL*/, DWORD dwLanguageCodepageId /* = (DWORD)-1 */)
{
	CString	strPath;

	GetModuleFileName(hModule, strPath.GetBuffer(_MAX_PATH), _MAX_PATH);
	strPath.ReleaseBuffer();
	return Create(strPath, dwLanguageCodepageId);
}


BOOL CFileVersionInfo::Create(LPCTSTR lpszFileName, DWORD dwLanguageCodepageId /* = (DWORD)-1 */)
{
	Reset();

	DWORD	dwHandle;
	DWORD	dwFileVersionInfoSize = GetFileVersionInfoSize((LPTSTR)lpszFileName, &dwHandle);
	if (!dwFileVersionInfoSize)
		return FALSE;

	LPVOID	lpData = (LPVOID)new BYTE[dwFileVersionInfoSize];
	if (!lpData)
		return FALSE;

	try
	{
		if (!GetFileVersionInfo((LPTSTR)lpszFileName, dwHandle, dwFileVersionInfoSize, lpData))
			throw FALSE;

		// catch default information
		LPVOID	lpInfo;
		UINT		unInfoLen;
		if (VerQueryValue(lpData, _T("\\"), &lpInfo, &unInfoLen))
		{
			ASSERT(unInfoLen == sizeof(m_FileInfo));
			if (unInfoLen == sizeof(m_FileInfo))
				memcpy(&m_FileInfo, lpInfo, unInfoLen);
		}

		// find best matching language and codepage
		VerQueryValue(lpData, _T("\\VarFileInfo\\Translation"), &lpInfo, &unInfoLen);
		
		DWORD	dwLangCode = 0;
		if (dwLanguageCodepageId==(DWORD)-1)
		{
			if (!GetTranslationId(lpInfo, unInfoLen, LANGIDFROMLCID(GetThreadLocale()), dwLangCode, FALSE))
			{
				if (!GetTranslationId(lpInfo, unInfoLen, LANGIDFROMLCID(GetThreadLocale()), dwLangCode, TRUE))
				{
					if (!GetTranslationId(lpInfo, unInfoLen, GetUserDefaultLangID(), dwLangCode, FALSE))
					{
						if (!GetTranslationId(lpInfo, unInfoLen, GetUserDefaultLangID(), dwLangCode, TRUE))
						{
							if (!GetTranslationId(lpInfo, unInfoLen, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), dwLangCode, TRUE))
							{
								if (!GetTranslationId(lpInfo, unInfoLen, MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL), dwLangCode, TRUE))
									// use the first one we can get
									dwLangCode = *((DWORD*)lpInfo);
							}
						}
					}
				}
			}
		}
		else
			dwLangCode = dwLanguageCodepageId;

		m_wLanguageId = (WORD)(dwLangCode&0x0000FFFF);
		m_wCodePageId = (WORD)((dwLangCode&0xFFFF0000)>>16);
		

		CString	strSubBlock;
		strSubBlock.Format(_T("\\StringFileInfo\\%04X%04X\\"), m_wLanguageId, m_wCodePageId);
		
		// catch string table
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("CompanyName")), &lpInfo, &unInfoLen))
			m_strCompanyName = CString((LPCTSTR)lpInfo);
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("FileDescription")), &lpInfo, &unInfoLen))
			m_strFileDescription = CString((LPCTSTR)lpInfo);
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("FileVersion")), &lpInfo, &unInfoLen))
			m_strFileVersion = CString((LPCTSTR)lpInfo);
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("InternalName")), &lpInfo, &unInfoLen))
			m_strInternalName = CString((LPCTSTR)lpInfo);
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("LegalCopyright")), &lpInfo, &unInfoLen))
			m_strLegalCopyright = CString((LPCTSTR)lpInfo);
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("OriginalFileName")), &lpInfo, &unInfoLen))
			m_strOriginalFileName = CString((LPCTSTR)lpInfo);
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("ProductName")), &lpInfo, &unInfoLen))
			m_strProductName = CString((LPCTSTR)lpInfo);
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("ProductVersion")), &lpInfo, &unInfoLen))
			m_strProductVersion = CString((LPCTSTR)lpInfo);
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("Comments")), &lpInfo, &unInfoLen))
			m_strComments = CString((LPCTSTR)lpInfo);
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("LegalTrademarks")), &lpInfo, &unInfoLen))
			m_strLegalTrademarks = CString((LPCTSTR)lpInfo);
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("PrivateBuild")), &lpInfo, &unInfoLen))
			m_strPrivateBuild = CString((LPCTSTR)lpInfo);
		if (VerQueryValue(lpData, (LPTSTR)(LPCTSTR)(strSubBlock+_T("SpecialBuild")), &lpInfo, &unInfoLen))
			m_strSpecialBuild = CString((LPCTSTR)lpInfo);

		delete[] lpData;
	}
	catch (BOOL)
	{
		delete[] lpData;
		return FALSE;
	}

	return TRUE;
}


WORD CFileVersionInfo::GetFileVersion(int nIndex) const
{
	if (nIndex == 0)
		return (WORD)(m_FileInfo.dwFileVersionLS & 0x0000FFFF);
	else if (nIndex == 1)
		return (WORD)((m_FileInfo.dwFileVersionLS & 0xFFFF0000) >> 16);
	else if (nIndex == 2)
		return (WORD)(m_FileInfo.dwFileVersionMS & 0x0000FFFF);
	else if (nIndex == 3)
		return (WORD)((m_FileInfo.dwFileVersionMS & 0xFFFF0000) >> 16);
	else
		return 0;
}


WORD CFileVersionInfo::GetProductVersion(int nIndex) const
{
	if (nIndex == 0)
		return (WORD)(m_FileInfo.dwProductVersionLS & 0x0000FFFF);
	else if (nIndex == 1)
		return (WORD)((m_FileInfo.dwProductVersionLS & 0xFFFF0000) >> 16);
	else if (nIndex == 2)
		return (WORD)(m_FileInfo.dwProductVersionMS & 0x0000FFFF);
	else if (nIndex == 3)
		return (WORD)((m_FileInfo.dwProductVersionMS & 0xFFFF0000) >> 16);
	else
		return 0;
}


DWORD CFileVersionInfo::GetFileFlagsMask() const
{
	return m_FileInfo.dwFileFlagsMask;
}


DWORD CFileVersionInfo::GetFileFlags() const
{
	return m_FileInfo.dwFileFlags;
}


DWORD CFileVersionInfo::GetFileOs() const
{
	return m_FileInfo.dwFileOS;
}


DWORD CFileVersionInfo::GetFileType() const
{
	return m_FileInfo.dwFileType;
}


DWORD CFileVersionInfo::GetFileSubtype() const
{
	return m_FileInfo.dwFileSubtype;
}


CTime CFileVersionInfo::GetFileDate() const
{
	FILETIME	ft;
	ft.dwLowDateTime = m_FileInfo.dwFileDateLS;
	ft.dwHighDateTime = m_FileInfo.dwFileDateMS;
	return CTime(ft);
}


CString CFileVersionInfo::GetCompanyName() const
{
	return m_strCompanyName;
}


CString CFileVersionInfo::GetFileDescription() const
{
	return m_strFileDescription;
}


CString CFileVersionInfo::GetFileVersion() const
{
	return m_strFileVersion;
}


CString CFileVersionInfo::GetInternalName() const
{
	return m_strInternalName;
}


CString CFileVersionInfo::GetLegalCopyright() const
{
	return m_strLegalCopyright;
}


CString CFileVersionInfo::GetOriginalFileName() const
{
	return m_strOriginalFileName;
}


CString CFileVersionInfo::GetProductName() const
{
	return m_strProductName;
}


CString CFileVersionInfo::GetProductVersion() const
{
	return m_strProductVersion;
}


CString CFileVersionInfo::GetComments() const
{
	return m_strComments;
}


CString CFileVersionInfo::GetLegalTrademarks() const
{
	return m_strLegalTrademarks;
}


CString CFileVersionInfo::GetPrivateBuild() const
{
	return m_strPrivateBuild;
}


CString CFileVersionInfo::GetSpecialBuild() const
{
	return m_strSpecialBuild;
}


LANGID CFileVersionInfo::GetLanguageId() const
{
	return m_wLanguageId;
}


WORD CFileVersionInfo::GetCodePageId() const
{
	return m_wCodePageId;
}


void CFileVersionInfo::Reset()
{
	m_wLanguageId = -1;
	m_wCodePageId = -1;

	ZeroMemory(&m_FileInfo, sizeof(m_FileInfo));
	m_strCompanyName.Empty();
	m_strFileDescription.Empty();
	m_strFileVersion.Empty();
	m_strInternalName.Empty();
	m_strLegalCopyright.Empty();
	m_strOriginalFileName.Empty();
	m_strProductName.Empty();
	m_strProductVersion.Empty();
	m_strComments.Empty();
	m_strLegalTrademarks.Empty();
	m_strPrivateBuild.Empty();
	m_strSpecialBuild.Empty();
}


