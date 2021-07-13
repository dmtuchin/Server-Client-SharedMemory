#include <vector>
#include <string>
#include <stdlib.h>

#define IN
#define OUT

class ConfigMaster
{
public:
	constexpr ConfigMaster() = default;
	ConfigMaster(const char* pszFile);
	~ConfigMaster(void);

	typedef enum CONFIGTYPE
	{
		TheString,
		TheInteger,
		TheFloat,
		TheCommand
	};

	class ConfigItem
	{
	public:
		constexpr ConfigItem() = default ;
		ConfigItem(CONFIGTYPE theType, void* theDstValue, const char* theKeyword) : aType(theType), pValue(theDstValue) 
		{
			strcpy_s(szKeyword, theKeyword);
		}
		void operator = (ConfigItem& obj)
		{
			aType = obj.aType;
			pValue = obj.pValue;
			strcpy_s(szKeyword, obj.szKeyword);
		};
		char       szKeyword[256]{}; // config file keyword:  KEYWORD = 
		CONFIGTYPE aType{};
		void* pValue{};
	};

	void AssociateFlatItem(ConfigMaster::ConfigItem* pAscItem);
	void AssociateItem(ConfigMaster::ConfigItem* pAscItem);

	void ReUse();
	bool ReadBlock();
	int  ReadFlat();

private:
	void WriteValue(OUT void* pVal, IN int pValSz, IN CONFIGTYPE aType, IN const char* pszValue);
	void FillZeroCRLN(char* pszStr);
	ConfigItem GetRecordByKeyname(const char* pszKeyname);

	FILE* cfg{};
	std::vector<ConfigItem> theAssociatedItems;
	std::vector<ConfigItem> theAssociatedFlatItems;
};
