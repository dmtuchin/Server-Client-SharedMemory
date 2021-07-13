

#include "ConfigMaster.h"

ConfigMaster::ConfigMaster(const char* pszFile)
{
	errno_t err = fopen_s(&cfg, pszFile, "rt");
	if (err == 0)
	{
	}
}


ConfigMaster::~ConfigMaster(void)
{
	if(cfg)
	{
		fclose(cfg);
		cfg = nullptr;
	}
}
void ConfigMaster::AssociateFlatItem(ConfigMaster::ConfigItem* pAscFlatItem)
{
	//
	theAssociatedFlatItems.push_back(*pAscFlatItem);
}

void ConfigMaster::AssociateItem(ConfigMaster::ConfigItem* pAscItem)
{
	//
	theAssociatedItems.push_back(*pAscItem);
}
bool ConfigMaster::ReadBlock()
{
	int iRecords = 0;
	char sCfg[512] = {0};
	// parsing
	std::string str(sCfg), wrk;

	if (cfg)
	{
		while (!feof(cfg))
		{
			if (NULL != fgets(sCfg, 511, cfg))
			{
				str = sCfg;
				bool bKeywordFound = false;
				for (const auto& aItem : theAssociatedItems)
				{
					wrk = str;
					if (-1 != str.find(aItem.szKeyword))
					{
						strcpy_s(sCfg, wrk.substr(strlen(aItem.szKeyword) + 1).c_str());
						FillZeroCRLN(sCfg);
						std::string sValue = sCfg;
						sValue.erase(remove_if(sValue.begin(), sValue.end(), [&](char c) {return c == ' ' || c == '='; }), sValue.end());
						WriteValue(aItem.pValue, sValue.size() + 1, aItem.aType, sValue.c_str());
						++iRecords;
						break;
					}
				}
				// record block readed
				if (iRecords == theAssociatedItems.size())
					break;
			}
		}
	}
	return iRecords;
}
void ConfigMaster::ReUse()
{
	fseek(cfg, 0, SEEK_SET);
}
int ConfigMaster::ReadFlat()
{
	char sCfg[512] = {0};

	// parsing
	std::string str(sCfg), wrk;

	int iRecords = 0;
	if(cfg != NULL)
		while(!feof(cfg))
		{
			if(NULL != fgets(sCfg, 511, cfg))
			{
				str = sCfg;

				std::vector<ConfigMaster::ConfigItem>::iterator it = theAssociatedFlatItems.begin();
				for (const auto& aItem : theAssociatedFlatItems)
				{
					wrk = str;
					if (-1 != str.find(aItem.szKeyword))
					{
						strcpy_s(sCfg, wrk.substr(strlen(aItem.szKeyword) + 1).c_str());
						FillZeroCRLN(sCfg);
						std::string sValue = sCfg;
						sValue.erase(remove_if(sValue.begin(), sValue.end(), [&](char c) {return c == ' ' || c == '='; }), sValue.end());
						WriteValue(aItem.pValue, sValue.size(), aItem.aType, sValue.c_str());
						iRecords++;
						break;
					}
				}
			}
		}
	return iRecords;
}
void ConfigMaster::FillZeroCRLN(char* pszStr)
{
	int iSz = strlen(pszStr);
	for (int i = 0 ; i < iSz ; i++)
		if (pszStr[i] == 0x0d || pszStr[i] == 0x0a)
			pszStr[i] = 0;
}
ConfigMaster::ConfigItem ConfigMaster::GetRecordByKeyname(const char* pszKeyname)
{
	unsigned int i = 0;
	bool bFound = false;
	for(auto it: theAssociatedItems)
	{
		if (strcmp(it.szKeyword, pszKeyname) != 0)
		{
			return it;
		}
	}
	return ConfigMaster::ConfigItem();
}
void ConfigMaster::WriteValue(OUT void* pVal, IN int pValSz, IN CONFIGTYPE aType, IN const char* pszValue)
{
	switch (aType)
	{
	case TheString:
		memcpy((char*)pVal, pszValue, pValSz);
		break;
	case TheInteger:
		*(int*)pVal = atoi(pszValue);
		break;
	case TheFloat:
		*(double*)pVal = atof(pszValue);
		break;
	case TheCommand:
		break;
	default:
		pVal = nullptr;
	}
}