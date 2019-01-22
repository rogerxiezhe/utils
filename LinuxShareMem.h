#ifndef UTILS_LINUXSHAREMEM_H
#define UTILS_LINUXSHAREMEM_H

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

class CShareMem
{
public:
	// 保证文件存在
	static bool ConfirmFile(const char* name)
	{
		int fd = open(name, 0_CREAT | 0_EXCL | 0_WRONLY, 0666);
		if (fd < 0)
		{
			if (errno != EEXIST)
			{
				return false;
			}
		}
		else
		{
			close(fd);
		}

		return true;
	}

	// 是否存在指定名字的共享内存
	static bool Exists(const char* name)
	{
		// 共享文件
		if (!ConfirmFile(name))
		{
			return false;
		}

		key_t key = ftok(name, 0);
		if (-1 == key)
		{
			return false;
		}

		if (shmget(key, 0, 0) == -1)
		{
			return false;
		}
		
		return true;
	}

	CShareMem()
	{
		m_pName = NULL;
		m_nSize = 0;
		m_pMem = NULL;
		m_shm_id = -1;
	}

	~CShareMem()
	{
		Destroy();
	}

	const char* GetName() const
	{
		if (NULL == m_pName)
		{
			return "";
		}

		return m_pName;
	}

	// 获得长度
	size_t GetSize() const
	{
		return m_nSize;
	}

	void* GetMem() const
	{
		return m_pMem;
	}

	// 是否有效
	bool IsValid() const
	{
		return (m_shm_id != -1) && (m_pMem != NULL);
	}

	bool Create(const char* name, size_t size, bool* exists = NULL)
	{
		size_t name_size = strlen(name) + 1;
		char* pName = new char[name_size];
		memcpy(pName, name, name_size);

		if (m_pName)
		{
			delete[] m_pName;
		}

		m_pName = pName;
		m_nSize = size;

		if (exists)
		{
			*exists = false;
		}

		if (!ConfirmFile(name))
		{
			return false;
		}

		key_t key = ftok(m_pName, 0);
		if (key == -1)
		{
			return false;
		}

		m_shm_id = shmget(key, size, IPC_CREAT | IPC_EXCL | 00666);
		if (m_shm_id == -1)
		{
			if (errorno != EEXIST)
			{
				return false;
			}

			if (exists)
			{
				*exists = true;
			}

			m_shm_id = shmget(key, size, IPC_CREAT | 00666);
			if (m_shm_id == -1)
			{
				return false;
			}
		}

		// 映射到进程空间
		m_pMem = shmat(m_shm_id, NULL, 0);
		if ((void*)-1 == m_pMem)
		{
			m_pMem = NULL;
			return false;
		}

		return true;
	}

	bool Destroy()
	{
		if (m_pMem)
		{
			shmdt(m_pMem);
			m_pMem = NULL;
		}

		if (m_shm_id != -1)
		{
			shmctl(m_shm_id, IPC_RMID, NULL);
			m_shm_id = -1;
		}

		if (m_pName)
		{
			delete[] m_pName;
			m_pName = NULL;
		}

		return true;
	}

private:
	CShareMem(const CShareMem&);
	CShareMem& operator=(const CShareMem&);

private:
	char* m_pName;
	size_t m_nSize;
	void* m_pMem;
	int m_shm_id;
};

#endif // !UTILS_LINUXSHAREMEM_H
