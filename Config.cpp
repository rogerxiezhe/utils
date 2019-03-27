#include "Config.h"
#include "Log.h"
#include <vector>
#include <string.h>
#include <fstream>
#include "IniFile.h"
#include "LinuxPortable.h"

bool CConfig::LoadConfig(const string & file_path)
{
	bool bRet = false;
	CIniFile iniFile();
	if (file_path.empty())
	{
		char cwd[1024] = {0};
		if (getcwd(cwd, sizeof(cwd)) ==NULL) return false;

		iniFile.SetFile(cwd);
	}
	else
	{
		iniFile.SetFile(file_path);
	}
	
	if (!iniFile.LoadConfig()) return false;

	CopyString(m_sysConfig.szlogPath, sizeof(m_sysConfig.szlogPath), iniFile.ReadString("system", "log_path", "log.txt").c_str());
	m_sysConfig.nLogLevel = iniFile.ReadInt("system", "log_path", 0);
	m_sysConfig.nLogSplitMinute = iniFile.ReadInt("system", "log_path", 60);

	CopyString(m_servConfig.szIp, sizeof(m_servConfig.szIp), iniFile.ReadString("server", "ip", "127.0.0.1");
	m_servConfig.nPort = iniFile.ReadInt("server", "port", "8848");

	return true;
}





