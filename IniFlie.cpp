#include "IniFile.h"
#include <fstream>
#include <stdlib.h>

string TrimLeftRight(const string& src)
{
	string strRet;
	for (size_t i = 0; i < src.length(); ++i)
	{
		if (src[i] != ' ' && src[i] != '\t')
		{
			strRet = src.substr(i);
			break;
		}
	}

	for (int i = strRet.length() - 1; i >= 0; --i)
	{
		if (strRet[i] != ' ' && strRet[i] != '\t')
		{
			strRet = strRet.substr(0, i + 1);
			break;
		}
	}

	return strRet;
}

void SplitString(const string& src, vector<string>& vecRet, char split = '=')
{	
	int len = src.length();
	if (len < 1) return;
	int nIndex = src.find(split);
	if (nIndex < 0) return;
	string subLeft, subRight ;
	if (0 == nIndex && len > 1)
	{
		subRight = src.substr(1);
	}
	else if (len - 1 == nIndex && len > 1)
	{
		subLeft = src.substr(0, nIndex);
	}
	else
	{
		subLeft = src.substr(0, nIndex);
		subRight = src.substr(nIndex + 1);
	}

	vecRet.push_back(subLeft);
	vecRet.push_back(subRight);
}


bool CIniFile::LoadConfig()
{
	ifstream fReader;
	fReader.open(m_fileName, ios::in);
	if (!fReader.is_open()) return false;

	vector<string> vec_line;

	char szLine[257] = { 0 };
	while (!fReader.eof())
	{
		fReader.getline(szLine, 256);
		vec_line.push_back(szLine);
	}

	fReader.close();

	SECTION* pSection = NULL;
	for (size_t i = 0; i < vec_line.size(); ++i)
	{
		string strLine = TrimLeftRight(vec_line[i]);
		if (strLine.length() < 2) continue;

		int len = strLine.length();
		if (strLine[0] == '[' && strLine[len - 1] == ']')
		{
			string strSecName = strLine.substr(1, len - 2);

			pSection = (SECTION*)GetSection(strSecName);
			if (NULL == pSection)
			{
				m_sections.push_back(SECTION());
				pSection = &m_sections.back();
				pSection->strName = strSecName;
			}
			continue;
		}

		if (strLine[0] == '/' && strLine[1] == '/') continue;

		if (NULL == pSection) continue;

		vector<string> vecRet;
		SplitString(strLine, vecRet);
		if (vecRet.size() < 2) continue;
		if (vecRet[0].length() == 0) continue; // key null

		ELEMT elemt;
		elemt.strName =  TrimLeftRight(vecRet[0]);
		elemt.strValue = TrimLeftRight(vecRet[1]);

		pSection->elems.push_back(elemt);
	}

	m_bLoad = true;
	return true;
}

const char* CIniFile::GetData(const string& sectionName, const string& keyName) const
{
	ELEMT* pElemt = (ELEMT*)GetElem(sectionName, keyName);
	if (NULL == pElemt) return NULL;

	return pElemt->strValue.c_str();
}

const CIniFile::ELEMT* CIniFile::GetElem(const string& sectionName, const string& keyName) const
{
	SECTION* pSection = (SECTION*)GetSection(sectionName);
	if (NULL == pSection)
	{
		return NULL;
	}

	vector<ELEMT>& vecElemts = pSection->elems;
	for (size_t i = 0; i < vecElemts.size(); ++i)
	{
		if (strcasecmp(keyName.c_str(), vecElemts[i].strName.c_str()) == 0)
		{
			return &vecElemts[i];
		}
	}

	return NULL;
}

const CIniFile::SECTION* CIniFile::GetSection(const string& sectionName) const
{
	for (size_t i = 0; i < m_sections.size(); ++i)
	{
		string strName = m_sections[i].strName;
		if (strcasecmp(sectionName.c_str(), strName.c_str()) == 0)
		{
			return &m_sections[i];
		}
	}

	return NULL;
}

int CIniFile::ReadInt(const string& section, const string& key, int def) const
{
	const char* ret = GetData(section, key);
	return (ret != NULL? atoi(ret) : def);
}

string CIniFile::ReadString(const string& section, const string& key, string strdef) const
{
	const char* ret = GetData(section, key);
	string strRet;
	return (ret != NULL ? (strRet = ret) : strdef);
}

float CIniFile::ReadFloat(const string& section, const string& key, float def) const
{
	const char* ret = GetData(section, key);
	return (ret != NULL ? (float)atof(ret) : def);
}