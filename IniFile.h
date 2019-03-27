#ifndef INIFILE_H
#define INIFILE_H

#include <vector>
#include <string>
using namespace std;

#ifdef _WIN32
#define  strcasecmp _stricmp
#endif

class CIniFile
{
private:
	struct ELEMT
	{
		string strName;
		string strValue;
	};

	struct SECTION
	{
		string strName;
		vector<ELEMT> elems;
	};

public:
	CIniFile() { m_bLoad = false; }
	CIniFile(const string& fileName) : m_fileName(fileName) { m_bLoad = false; }
	~CIniFile() { m_sections.clear(); }

	bool LoadConfig();
	void SetFile(const string& fileName) { m_fileName = fileName; }
	int ReadInt(const string& section, const string& key, int def = 0) const;
	string ReadString(const string& section, const string& key, string strdef = "") const;
	float ReadFloat(const string& section, const string& key, float def = 0.0) const;

private:
	const char* GetData(const string& sectionName, const string& keyName) const;
	const CIniFile::SECTION* GetSection(const string& sectionName) const;
	const CIniFile::ELEMT* GetElem(const string& sectionName, const string& keyName) const;

private:
	vector<SECTION> m_sections;
	string m_fileName;
	bool m_bLoad;
};
#endif // !INIFILE_H
