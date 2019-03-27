#ifndef __SYS_CONFIG_H__
#define __SYS_CONFIG_H__

#include <string>
using namespace std;

#define MAX_FILE_PATH 256

class CConfig
{
public:
	struct SystemConfig
	{
		int nLogLevel;
		char[MAX_FILE_PATH] szlogPath;	
		int nLogSplitMinute;	
	};

	struct ServerConfig
	{
		char[32] szIp;
		int nPort;
	};
	
public:
	static CConfig* GetInstance() 
	{
		if (NULL == m_pInstance)
		{
			m_pInstance = new CConfig();
		}

		return m_pInstance;
	}
	
	bool LoadConfig(const string& file_path);
	

private:
	CConfig();
	CConfig(const CConfig&);
	CConfig oprerator=(const CConfig&);
	
private:
	static CConfig* m_pInstance;
	SystemConfig m_sysConfig;
	ServerConfig m_servConfig;
};

CConfig* CConfig::m_pInstance = NULL;

#endif
