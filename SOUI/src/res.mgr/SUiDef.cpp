﻿#include "include\souistd.h"
#include "res.mgr\SUiDef.h"
#include "helper\SplitString.h"
#include "helper\SAutoBuf.h"

namespace SOUI{

	const static WCHAR KNodeUidef[]     = L"uidef" ;
	const static WCHAR KNodeString[]    = L"string";
	const static WCHAR KNodeColor[]     = L"color";
	const static WCHAR KNodeDim[]       = L"dim";
	const static WCHAR KNodeSkin[]      = L"skin";
	const static WCHAR KNodeStyle[]     = L"style";
	const static WCHAR KNodeTemplate[] = L"template";
	const static WCHAR KNodeObjAttr[]   = L"objattr";
	const static TCHAR KDefFontFace[]   = _T("宋体");


	static pugi::xml_node GetSourceXmlNode(pugi::xml_node nodeRoot,pugi::xml_document &docInit,IResProvider *pResProvider, const wchar_t * pszName)
	{
		pugi::xml_node     nodeData = nodeRoot.child(pszName,false);
		if(nodeData)
		{
			pugi::xml_attribute attrSrc = nodeData.attribute(L"src",false);
			if(attrSrc)
			{//优先从src属性里获取数据
				SStringT strSrc = S_CW2T(attrSrc.value());
				SStringTList strList;
				if(2==ParseResID(strSrc,strList))
				{
					SAutoBuf strXml;
					size_t dwSize = pResProvider->GetRawBufferSize(strList[0],strList[1]);

					strXml.Allocate(dwSize);
					pResProvider->GetRawBuffer(strList[0],strList[1],strXml,dwSize);
					pugi::xml_parse_result result= docInit.load_buffer(strXml,strXml.size(),pugi::parse_default,pugi::encoding_utf8);
					if(result) nodeData = docInit.child(pszName);
				}
			}
		}
		return nodeData;
	}

	class SUiDefInfo : public TObjRefImpl<IUiDefInfo>
	{
	public:
		SUiDefInfo() {}

		BOOL Init(IResProvider *pResProvide,LPCTSTR pszUidef);

		virtual SSkinPool * GetSkinPool()  override;
		virtual SStylePool * GetStylePool() override;
		virtual SNamedColor & GetNamedColor()  override;
		virtual SNamedString & GetNamedString()  override;
		virtual SNamedDimension & GetNamedDimension() override;
		virtual SObjDefAttr * GetObjDefAttr() override;
		virtual FontInfo & GetDefFontInfo()  override;
		virtual pugi::xml_node  GetCaretInfo()  override;
		virtual void SetSkinPool(SSkinPool * pSkinPool)  override;
		virtual void SetStylePool(SStylePool * pStylePool) override;
		virtual void SetObjDefAttr(SObjDefAttr * pObjDefAttr) override;
		virtual STemplatePool * GetTemplatePool() override;
		virtual void SetTemplatePool(STemplatePool * pPool) override;
	protected:

		SAutoRefPtr<SSkinPool>    pSkinPool;
		SAutoRefPtr<SStylePool>   pStylePool;
		SAutoRefPtr<SObjDefAttr>  objDefAttr;
		SAutoRefPtr<STemplatePool> templatePool;

		SNamedColor   namedColor;
		SNamedString  namedString;
		SNamedDimension namedDim;

		FontInfo	  defFontInfo;
		pugi::xml_document xmlCaret;
	};


	BOOL SUiDefInfo::Init(IResProvider *pResProvider,LPCTSTR pszUidef)
	{
		BOOL bRet = FALSE;
		SStringTList strUiDef;
		if(2!=ParseResID(pszUidef,strUiDef))
		{
			SLOGFMTD(_T("warning!!!! Add ResProvider Error."));
		}

		size_t dwSize=pResProvider->GetRawBufferSize(strUiDef[0],strUiDef[1]);
		if(dwSize==0)
		{
			SLOGFMTD(_T("warning!!!! uidef was not found in the specified resprovider"));
		}else
		{
			pugi::xml_document docInit;
			SAutoBuf strXml;
			strXml.Allocate(dwSize);

			pResProvider->GetRawBuffer(strUiDef[0],strUiDef[1],strXml,dwSize);

			pugi::xml_parse_result result= docInit.load_buffer(strXml,strXml.size(),pugi::parse_default,pugi::encoding_utf8);

			if(!result)
			{//load xml failed
				SLOGFMTD(_T("warning!!! load uidef as xml document failed"));
			}else
			{//init named objects
				pugi::xml_node root = docInit.child(KNodeUidef,false);
				if(!root)
				{
					SLOGFMTD(_T("warning!!! \"uidef\" element is not the root element of uidef xml"));
				}else
				{
					//parse default font
					pugi::xml_node xmlFont;
					xmlFont=root.child(L"font",false);

					FONTSTYLE fontStyle(0);
					if(xmlFont)
					{
						fontStyle.attr.cSize=xmlFont.attribute(L"size").as_int(12);
						fontStyle.attr.byCharset =(BYTE)xmlFont.attribute(L"charset").as_int(DEFAULT_CHARSET);
						fontStyle.attr.fBold = xmlFont.attribute(L"bold").as_bool(false);
						fontStyle.attr.fUnderline = xmlFont.attribute(L"underline").as_bool(false);
						fontStyle.attr.fStrike = xmlFont.attribute(L"strike").as_bool(false);
						fontStyle.attr.fItalic = xmlFont.attribute(L"italic").as_bool(false);
						fontStyle.attr.byWeight = (xmlFont.attribute(L"weight").as_uint(0) + 2) / 4; //scale weight from [0-1000] to [0,250].
						defFontInfo.strName = xmlFont.attribute(L"name").as_string(L"default");
						defFontInfo.style = fontStyle;
						defFontInfo.strFaceName = S_CW2T(xmlFont.attribute(L"face").value());

						if(defFontInfo.strFaceName.IsEmpty() || !SUiDef::CheckFont(defFontInfo.strFaceName))
						{
							defFontInfo.strFaceName = KDefFontFace;
						}
					}else
					{
						fontStyle.attr.cSize = 12;
						fontStyle.attr.byCharset =DEFAULT_CHARSET;
						

						defFontInfo.strName = L"default";
						defFontInfo.style = fontStyle;
						defFontInfo.strFaceName = KDefFontFace;
					}

					//parse default Unit
					pugi::xml_node xmlUnit;
					xmlUnit = root.child(L"unit");
					if (xmlUnit)
					{
						SStringW unit = xmlUnit.attribute(L"defUnit").as_string(L"dp");
						SLayoutSize::setDefUnit(unit);
					}

					xmlCaret.reset();
					pugi::xml_node xmlCaretNode = root.child(L"caret");
					if(xmlCaretNode)
						xmlCaret.append_copy(xmlCaretNode);

					//load named string
					{
						pugi::xml_document docData;
						pugi::xml_node     nodeData = GetSourceXmlNode(root,docData,pResProvider,KNodeString);
						if(nodeData)
						{
							namedString.Init(nodeData);
						}
					}

					//load named color
					{
						pugi::xml_document docData;
						pugi::xml_node     nodeData = GetSourceXmlNode(root,docData,pResProvider,KNodeColor);
						if(nodeData)
						{
							namedColor.Init(nodeData);
						}
					}

					//load named dimension
					{
						pugi::xml_document docData;
						pugi::xml_node     nodeData = GetSourceXmlNode(root, docData, pResProvider, KNodeDim);
						if (nodeData)
						{
							namedDim.Init(nodeData);
						}
					}
					//load named skin
					{
						pugi::xml_document docData;
						pugi::xml_node     nodeData = GetSourceXmlNode(root,docData,pResProvider,KNodeSkin);
						if(nodeData)
						{
							pSkinPool.Attach(new SSkinPool);
							pSkinPool->LoadSkins(nodeData);
						}
					}
					//load named style
					{
						pugi::xml_document docData;
						pugi::xml_node     nodeData = GetSourceXmlNode(root,docData,pResProvider,KNodeStyle);
						if(nodeData)
						{
							pStylePool.Attach(new SStylePool);
							pStylePool->Init(nodeData);
						}
					}
					//load named template
					{
						pugi::xml_document docData;
						pugi::xml_node     nodeData = GetSourceXmlNode(root, docData, pResProvider, KNodeTemplate);
						if (nodeData)
						{
							templatePool.Attach(new STemplatePool);
							templatePool->Init(nodeData);
						}
					}
					//load SWindow default attribute
					{
						pugi::xml_document docData;
						pugi::xml_node     nodeData = GetSourceXmlNode(root,docData,pResProvider,KNodeObjAttr);
						if(nodeData)
						{
							objDefAttr.Attach(new SObjDefAttr);
							objDefAttr->Init(nodeData);
						}
					}					
					bRet = TRUE;
				}
			}
		}
		return bRet;
	}

	SSkinPool * SUiDefInfo::GetSkinPool()   { return pSkinPool; }
	SStylePool * SUiDefInfo::GetStylePool()  { return pStylePool; }
	SNamedColor & SUiDefInfo::GetNamedColor()   { return namedColor; }
	SNamedString & SUiDefInfo::GetNamedString()   { return namedString; }
	SNamedDimension & SUiDefInfo::GetNamedDimension()   { return namedDim; }
	SObjDefAttr * SUiDefInfo::GetObjDefAttr()  { return objDefAttr; }
	FontInfo & SUiDefInfo::GetDefFontInfo()   { return defFontInfo; }

	pugi::xml_node  SUiDefInfo::GetCaretInfo()
	{
		return xmlCaret.child(L"caret");
	}

	void SUiDefInfo::SetSkinPool(SSkinPool * pSkinPool)
	{
		this->pSkinPool = pSkinPool;
	}
	void SUiDefInfo::SetStylePool(SStylePool * pStylePool)
	{
		this->pStylePool = pStylePool;
	}
	void SUiDefInfo::SetObjDefAttr(SObjDefAttr * pObjDefAttr)
	{
		this->objDefAttr = pObjDefAttr;
	}
	STemplatePool * SUiDefInfo::GetTemplatePool()
	{
		return templatePool;
	}
	void SUiDefInfo::SetTemplatePool(STemplatePool * pPool)
	{
		templatePool = pPool;
	}
	//////////////////////////////////////////////////////////////////////////

	#define HASFONT 2
	int CALLBACK DefFontsEnumProc(  CONST LOGFONT *lplf,     // logical-font data
		CONST TEXTMETRIC *lptm,  // physical-font data
		DWORD dwType,            // font type
		LPARAM lpData            // application-defined data
		)
	{
		return HASFONT;
	}

	static BOOL DefFontCheck(const SStringT & strFontName)
	{
		//确保字体存在
		HDC hdc = GetDC(NULL);
		int hasFont = EnumFonts(hdc,strFontName,DefFontsEnumProc,0);
		ReleaseDC(NULL,hdc);
		return hasFont == HASFONT;
	}

	FunFontCheck SUiDef::s_funFontCheck = DefFontCheck;


	SUiDef::SUiDef(void)
	{
	}

	SUiDef::~SUiDef(void)
	{
	}

	IUiDefInfo * SUiDef::CreateUiDefInfo(IResProvider *pResProvider, LPCTSTR pszUiDef)
	{
		IUiDefInfo * pUiDefInfo = CreateUiDefInfo2(pResProvider, pszUiDef);
		if (pUiDefInfo->GetSkinPool())
		{
			SSkinPoolMgr::getSingletonPtr()->PushSkinPool(pUiDefInfo->GetSkinPool());
		}
		if (pUiDefInfo->GetStylePool())
		{
			SStylePoolMgr::getSingletonPtr()->PushStylePool(pUiDefInfo->GetStylePool());
		}
		if(pUiDefInfo->GetTemplatePool())
		{
			STemplatePoolMgr::getSingletonPtr()->PushTemplatePool(pUiDefInfo->GetTemplatePool());
		}
		return pUiDefInfo;
	}

	IUiDefInfo * SUiDef::CreateUiDefInfo2(IResProvider *pResProvider, LPCTSTR pszUiDef)
	{
		SUiDefInfo *pRet = new SUiDefInfo();
		//将新uidef设置到系统中，在皮肤初始化的时候可以引用当前定义的颜色表。
		SAutoRefPtr<IUiDefInfo> pOldUiDef = SUiDef::getSingleton().GetUiDef();
		SUiDef::getSingleton().SetUiDef(pRet);
		pRet->Init(pResProvider, pszUiDef);
		SUiDef::getSingleton().SetUiDef(pOldUiDef);
		return pRet;
	}

	void SUiDef::SetUiDef( IUiDefInfo* pUiDefInfo )
	{
		m_pCurUiDef = pUiDefInfo;
	}

	void SUiDef::SetFontChecker(FunFontCheck fontCheck)
	{
		s_funFontCheck = fontCheck;
	}

	BOOL SUiDef::CheckFont(const SStringT & strFontName)
	{
		return s_funFontCheck(strFontName);
	}

}
