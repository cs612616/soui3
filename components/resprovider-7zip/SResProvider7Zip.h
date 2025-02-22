﻿#pragma once

#include <interface/SResProvider-i.h>
#include <unknown/obj-ref-impl.hpp>
#include <string/tstring.h>
#include <string/strcpcvt.h>
#include <souicoll.h>
#define _COLL_NS SOUI
#define SOUI_EXP
#include <helper/SResID.h>
#include <interface/render-i.h>

#include "Zip7Archive.h"
#include "zip7resprovider-param.h"

namespace SOUI{

class SResProvider7Zip : public TObjRefImpl<IResProvider>
{
public:
	SResProvider7Zip();
	~SResProvider7Zip(void);
	
    virtual BOOL Init(WPARAM wParam,LPARAM lParam);

    virtual BOOL HasResource(LPCTSTR strType,LPCTSTR pszResName);
    virtual HICON   LoadIcon(LPCTSTR pszResName,int cx,int cy);
    virtual HBITMAP    LoadBitmap(LPCTSTR pszResName);
    virtual HCURSOR LoadCursor(LPCTSTR pszResName);
    virtual IBitmap * LoadImage(LPCTSTR strType,LPCTSTR pszResName);
    virtual IImgX   * LoadImgX(LPCTSTR strType,LPCTSTR pszResName);
    virtual size_t GetRawBufferSize(LPCTSTR strType,LPCTSTR pszResName);
    virtual BOOL GetRawBuffer(LPCTSTR strType,LPCTSTR pszResName,LPVOID pBuf,size_t size);

protected:
    BOOL _Init(LPCTSTR pszZipFile ,LPCSTR pszPsw);
    BOOL _Init(HINSTANCE hInst,LPCTSTR pszResName,LPCTSTR pszType ,LPCSTR pszPsw);
	BOOL _LoadSkin();
	SStringT _GetFilePath(LPCTSTR pszResName,LPCTSTR pszType);
	
	SMap<SResID,SStringT> m_mapFiles;
    SAutoRefPtr<IRenderFactory> m_renderFactory;
	CZipArchive m_zipFile;
	SStringT m_childDir;
};

namespace RESPROVIDER_7ZIP
{
    SOUI_COM_C BOOL SOUI_COM_API SCreateInstance(IObjRef ** ppObj);
}

}//namespace SOUI
