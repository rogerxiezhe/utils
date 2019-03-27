#ifndef __SYS_LOG_H__
#define __SYS_LOG_H__

#include <string>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32
	#define FX_SYSTEM_WINDOWS
#endif

#ifdef FX_SYSTEM_WINDOWS
	#include <windows.h>
#else
	#include <stdlib.h>
	#include <locale.h>
	#include <wchar.h>
	#include <stdio.h>
#endif//FX_SYSTEM_WINDOWS

// default log system buffer size
#define LOG_DEF_BUF 512
// predefined macro for namespace, you can set another value at project compile option
//#define _LOG_NAMESAPCE_ LogSys

namespace _LOG_NAMESAPCE_
{
	// write log function callback
	typedef void (*log_callback)(void* context, const char* log);

	// these parameters must be define the implementation at the executable
	extern log_callback g_fnLog;
	extern void* g_fnLogCtx;
	extern size_t g_nLogLevel;

	enum LOG_LEVEL
	{
		LOG_LEVEL_DEBUG = 0x01,
		LOG_LEVEL_INFO  = 0x02,
		LOG_LEVEL_WARN  = 0x04,
		LOG_LEVEL_ERROR = 0x08,
		LOG_LEVEL_TEST  = 0x10,
	};

	inline void enable_log_level(size_t level)
	{
		g_nLogLevel |= level;
	}

	inline void disable_log_level(size_t level)
	{
		g_nLogLevel &= ~level;
	}

	inline bool exists_log_level(size_t level)
	{
		return ((g_nLogLevel & level) == level);
	}

	inline void safe_copy_string(char* buf, size_t byte_size, const char* str)
	{
		const size_t SIZE1 = strlen(str) + 1;

		if (SIZE1 <= byte_size)
		{
			memcpy(buf, str, SIZE1);
		}
		else
		{
			memcpy(buf, str, byte_size - 1);
			buf[byte_size - 1] = 0;
		}
	}

	inline size_t safe_sprintf(char* buf, size_t byte_size, const char* info, ...)
	{
		va_list args;

		va_start(args, info);

		size_t size_1 = byte_size - 1;

#ifdef FX_SYSTEM_WINDOWS
		int res = _vsnprintf(buf, size_1, info, args);
#else
		int res = vsnprintf(buf, size_1, info, args);
#endif //FX_SYSTEM_WINDOWS

		if ((size_t)res >= size_1)
		{
			buf[size_1] = 0;
			return size_1;
		}

		return res;
	}

	inline const char* safe_widestr_to_string(const wchar_t* info, char* buf, 
		size_t byte_size)
	{
#ifdef FX_SYSTEM_WINDOWS
		int res = WideCharToMultiByte(CP_ACP, 0, info, -1, buf, int(byte_size), 
			NULL, NULL);

		if (0 == res)
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				buf[byte_size - 1] = 0;
			}
			else
			{
				buf[0] = 0;
			}
		}

		return buf;
#else
		setlocale(LC_ALL, "");

		size_t res = wcstombs(buf, info, byte_size);

		if (res == (size_t)(-1))
		{
			// 无法转换
			buf[0] = 0;
		}
		else if (res == byte_size)
		{
			buf[byte_size - 1] = 0;
		}

		return buf;
#endif//FX_SYSTEM_WINDOWS
	}
	
	inline const char* get_file_name(const char* file_name)
	{
#ifdef FX_SYSTEM_WINDOWS
		const char * file = strrchr(file_name, '\\');
#else
		const char * file = strrchr(file_name, '/');
#endif //endif FX_SYSTEM_WINDOWS

		if (NULL == file)
		{
			file = file_name;
		}
		else
		{
			file++;
		}

		return file;
	}
	
	typedef struct hex_data_1_t
	{
		hex_data_1_t(unsigned char nVal):val(nVal){}
		hex_data_1_t(char nVal):val(nVal){}
		unsigned char val;
	}Hex1;

	typedef struct hex_data_2_t
	{
		hex_data_2_t(unsigned short nVal):val(nVal){}
		hex_data_2_t(short nVal):val(nVal){}
		hex_data_2_t(unsigned int nVal):val((unsigned short)nVal){}
		hex_data_2_t(int nVal):val((short)nVal){}
		unsigned short val;
	}Hex2;

	typedef struct hex_data_4_t
	{
		hex_data_4_t(unsigned int nVal):val(nVal){}
		hex_data_4_t(int nVal):val(nVal){}
		unsigned int val;
	}Hex4;

	typedef struct hex_pointer_t
	{
		hex_pointer_t(void* pVal):val(pVal){}
		void* val;
	}Pointer;

	template<size_t SIZE>
	class Logxx
	{
	public:
		explicit Logxx(const char* type)
		{
			safe_copy_string(m_Stack, SIZE, type);
			m_nSize = strlen(m_Stack);
		}

		~Logxx()
		{
			if ((g_fnLog != NULL) && (m_nSize > 0))
			{
				g_fnLog(g_fnLogCtx, m_Stack);
			}
		}

		const char* c_str() const
		{
			return m_Stack;
		}

		void clear()
		{
			m_nSize	   = 0;
			m_Stack[0] = 0;
		}

		inline Logxx& operator<<(Hex1 value)
		{
			if (SIZE > m_nSize)
			{
				m_nSize += safe_sprintf(&m_Stack[m_nSize], SIZE- m_nSize, "0x%02X", value.val);
			}

			return *this; 
		}
		inline Logxx& operator<<(Hex2 value)
		{
			if (SIZE > m_nSize)
			{
				m_nSize += safe_sprintf(&m_Stack[m_nSize], SIZE- m_nSize, "0x%04X", value.val);
			}

			return *this; 
		}
		inline Logxx& operator<<(Hex4 value)
		{
			if (SIZE > m_nSize)
			{
				m_nSize += safe_sprintf(&m_Stack[m_nSize], SIZE- m_nSize, "0x%08X", value.val);
			}

			return *this; 
		}
		inline Logxx& operator<<(Pointer value)
		{
			if (SIZE > m_nSize)
			{
				m_nSize += safe_sprintf(&m_Stack[m_nSize], SIZE- m_nSize, "0x%p", value.val);
			}

			return *this; 
		}
		inline Logxx& operator<<(bool value)
		{
			if (SIZE > m_nSize)
			{
				m_nSize += safe_sprintf(&m_Stack[m_nSize], SIZE- m_nSize, "%d", (int)value);
			}

			return *this; 
		}
		inline Logxx& operator<<(char value)
		{
			if (SIZE > m_nSize)
			{
				m_nSize += safe_sprintf(&m_Stack[m_nSize], SIZE- m_nSize, "%c", value);
			}

			return *this; 
		}
		inline Logxx& operator<<(unsigned char value)
		{ 
			if (SIZE > m_nSize)
			{
				m_nSize += safe_sprintf(&m_Stack[m_nSize], SIZE- m_nSize, "%d", (int)value);
			}

			return *this; 
		}
		inline Logxx& operator<<(short value)
		{ 
			if (SIZE > m_nSize)
			{
				m_nSize += safe_sprintf(&m_Stack[m_nSize], SIZE- m_nSize, "%d", (int)value);
			}

			return *this; 
		}
		inline Logxx& operator<<(unsigned short value)
		{ 
			if (SIZE > m_nSize)
			{
				m_nSize += safe_sprintf(&m_Stack[m_nSize], SIZE- m_nSize, "%d", (int)value);
			}

			return *this; 
		}
		inline Logxx& operator<<(int value)
		{ 
			if (SIZE > m_nSize)
			{
				m_nSize += safe_sprintf(&m_Stack[m_nSize], SIZE- m_nSize, "%d", value);
			}

			return *this; 
		}
		inline Logxx& operator<<(unsigned int value)
		{ 
			if (SIZE > m_nSize)
			{
				m_nSize += safe_sprintf(&m_Stack[m_nSize], SIZE- m_nSize, "%u", value);
			}

			return *this; 
		}
		inline Logxx& operator<<(long value)
		{ 
			if (SIZE > m_nSize)
			{
#if defined(FX_SYSTEM_WINDOWS) || defined(FX_SYSTEM_32BIT)
				m_nSize += safe_sprintf(&m_Stack[m_nSize], SIZE- m_nSize, "%d", (int)value);
#else
				m_nSize += safe_sprintf(&m_Stack[m_nSize], SIZE- m_nSize, "%lld", value);
#endif
			}

			return *this; 
		}
		inline Logxx& operator<<(unsigned long value)
		{ 
			if (SIZE > m_nSize)
			{
#if defined(FX_SYSTEM_WINDOWS) || defined(FX_SYSTEM_32BIT)
				m_nSize += safe_sprintf(&m_Stack[m_nSize], SIZE- m_nSize, "%u", value);
#else
				m_nSize += safe_sprintf(&m_Stack[m_nSize], SIZE- m_nSize, "%llu", value);
#endif
			}

			return *this; 
		}
#if defined(FX_SYSTEM_WINDOWS) || defined(FX_SYSTEM_32BIT)
		inline Logxx& operator<<(int64_t value) 
		{ 
			if (SIZE > m_nSize)
			{
				m_nSize += safe_sprintf(&m_Stack[m_nSize], SIZE- m_nSize, "%lld", value);
			}

			return *this; 
		}
		inline Logxx& operator<<(uint64_t value) 
		{ 
			if (SIZE > m_nSize)
			{
				m_nSize += safe_sprintf(&m_Stack[m_nSize], SIZE- m_nSize, "%llu", value);
			}

			return *this; 
		}
#endif
		inline Logxx& operator<<(float value) 
		{ 
			if (SIZE > m_nSize)
			{
				m_nSize += safe_sprintf(&m_Stack[m_nSize], SIZE- m_nSize, "%.02f", value);
			}

			return *this; 
		}
		inline Logxx& operator<<(double value) 
		{ 
			if (SIZE > m_nSize)
			{
				m_nSize += safe_sprintf(&m_Stack[m_nSize], SIZE- m_nSize, "%.02f", (float)value);
			}

			return *this; 
		}
		inline Logxx& operator<<(const char* value) 
		{ 
			if (SIZE > m_nSize)
			{
				m_nSize += safe_sprintf(&m_Stack[m_nSize], SIZE- m_nSize, "%s", value);
			}

			return *this; 
		}
		inline Logxx& operator<<(const std::string& value) 
		{ 
			if (SIZE > m_nSize)
			{
				m_nSize += safe_sprintf(&m_Stack[m_nSize], SIZE- m_nSize, "%s", value.c_str());
			}

			return *this; 
		}
		inline Logxx& operator<<(const wchar_t* value) 
		{ 
			if (SIZE > m_nSize)
			{
				const char* log = safe_widestr_to_string(value, &m_Stack[m_nSize], SIZE- m_nSize);
				m_nSize += strlen(log);
			}

			return *this; 
		}
		inline Logxx& operator<<(const std::wstring& value) 
		{ 
			if (SIZE > m_nSize)
			{
				const char* log = safe_widestr_to_string(value.c_str(), &m_Stack[m_nSize], SIZE- m_nSize);
				m_nSize += strlen(log);
			}

			return *this; 
		}
		 
	private:
		char   m_Stack[SIZE];
		size_t m_nSize;
	};

}//_LOG_NAMESAPCE_

// macro defines for logic

typedef _LOG_NAMESAPCE_::Hex1 LogHex1;
typedef _LOG_NAMESAPCE_::Hex2 LogHex2;
typedef _LOG_NAMESAPCE_::Hex4 LogHex4;
typedef _LOG_NAMESAPCE_::Pointer LogPointer;

#define LOG_DEBUG_OPEN() _LOG_NAMESAPCE_::enable_log_level(_LOG_NAMESAPCE_::LOG_LEVEL_DEBUG)
#define LOG_DEBUG_CLOSE() _LOG_NAMESAPCE_::disable_log_level(_LOG_NAMESAPCE_::LOG_LEVEL_DEBUG)
#define LOG_DEBUG_EXISTS() _LOG_NAMESAPCE_::exists_log_level(_LOG_NAMESAPCE_::LOG_LEVEL_DEBUG)
#define LOG_DEBUG_EX(log_data) \
	if ((_LOG_NAMESAPCE_::g_nLogLevel & _LOG_NAMESAPCE_::LOG_LEVEL_DEBUG) && (_LOG_NAMESAPCE_::g_fnLog != NULL)) { \
	_LOG_NAMESAPCE_::Logxx<LOG_DEF_BUF> log_xx("DEBUG, ("); \
	log_xx << __FUNCTION__ << ") " << log_data << " " << _LOG_NAMESAPCE_::get_file_name(__FILE__) << ":" << __LINE__; \
	} \

#define LOG_MESSAGE_OPEN() _LOG_NAMESAPCE_::enable_log_level(_LOG_NAMESAPCE_::LOG_LEVEL_INFO)
#define LOG_MESSAGE_CLOSE() _LOG_NAMESAPCE_::disable_log_level(_LOG_NAMESAPCE_::LOG_LEVEL_INFO)
#define LOG_MESSAGE_EXISTS() _LOG_NAMESAPCE_::exists_log_level(_LOG_NAMESAPCE_::LOG_LEVEL_INFO)
#define LOG_MESSAGE_EX(log_data) \
	if ((_LOG_NAMESAPCE_::g_nLogLevel & _LOG_NAMESAPCE_::LOG_LEVEL_INFO) && (_LOG_NAMESAPCE_::g_fnLog != NULL)) { \
	_LOG_NAMESAPCE_::Logxx<LOG_DEF_BUF> log_xx("INFO , ("); \
	log_xx << __FUNCTION__ << ") " << log_data << " " << _LOG_NAMESAPCE_::get_file_name(__FILE__) << ":" << __LINE__; \
	} \

#define LOG_WARING_OPEN() _LOG_NAMESAPCE_::enable_log_level(_LOG_NAMESAPCE_::LOG_LEVEL_WARN)
#define LOG_WARING_CLOSE() _LOG_NAMESAPCE_::disable_log_level(_LOG_NAMESAPCE_::LOG_LEVEL_WARN)
#define LOG_WARING_EXISTS() _LOG_NAMESAPCE_::exists_log_level(_LOG_NAMESAPCE_::LOG_LEVEL_WARN)
#define LOG_WARING_EX(log_data) \
	if ((_LOG_NAMESAPCE_::g_nLogLevel & _LOG_NAMESAPCE_::LOG_LEVEL_WARN) && (_LOG_NAMESAPCE_::g_fnLog != NULL)) { \
	_LOG_NAMESAPCE_::Logxx<LOG_DEF_BUF> log_xx("WARN , ("); \
	log_xx << __FUNCTION__ << ") " << log_data << " " << _LOG_NAMESAPCE_::get_file_name(__FILE__) << ":" << __LINE__; \
	} \

#define LOG_ERROR_OPEN() _LOG_NAMESAPCE_::enable_log_level(_LOG_NAMESAPCE_::LOG_LEVEL_ERROR)
#define LOG_ERROR_CLOSE() _LOG_NAMESAPCE_::disable_log_level(_LOG_NAMESAPCE_::LOG_LEVEL_ERROR)
#define LOG_ERROR_EXISTS() _LOG_NAMESAPCE_::exists_log_level(_LOG_NAMESAPCE_::LOG_LEVEL_ERROR)
#define LOG_ERROR_EX(log_data) \
	if ((_LOG_NAMESAPCE_::g_nLogLevel & _LOG_NAMESAPCE_::LOG_LEVEL_ERROR) && (_LOG_NAMESAPCE_::g_fnLog != NULL)) { \
	_LOG_NAMESAPCE_::Logxx<LOG_DEF_BUF> log_xx("ERROR, ("); \
	log_xx << __FUNCTION__ << ") " << log_data << " " << _LOG_NAMESAPCE_::get_file_name(__FILE__) << ":" << __LINE__; \
	} \

#define LOG_TEST_OPEN() _LOG_NAMESAPCE_::enable_log_level(_LOG_NAMESAPCE_::LOG_LEVEL_TEST)
#define LOG_TEST_CLOSE() _LOG_NAMESAPCE_::disable_log_level(_LOG_NAMESAPCE_::LOG_LEVEL_TEST)
#define LOG_TEST_EXISTS() _LOG_NAMESAPCE_::exists_log_level(_LOG_NAMESAPCE_::LOG_LEVEL_TEST)
#define LOG_TEST_EX(log_data) \
if ((_LOG_NAMESAPCE_::g_nLogLevel & _LOG_NAMESAPCE_::LOG_LEVEL_TEST) && (_LOG_NAMESAPCE_::g_fnLog != NULL)) { \
\
_LOG_NAMESAPCE_::Logxx<LOG_DEF_BUF> log_xx("TEST , ("); \
log_xx << __FUNCTION__ << ") " << log_data << " " << _LOG_NAMESAPCE_::get_file_name(__FILE__) << ":" << __LINE__; \
} \

#define LOG_EX(log_data) \
	if (_LOG_NAMESAPCE_::g_fnLog != NULL) { \
	_LOG_NAMESAPCE_::Logxx<LOG_DEF_BUF> log_xx(""); \
	log_xx << log_data; \
	} \

#define ABORT_LOG(log_data) {\
	_LOG_NAMESAPCE_::Logxx<LOG_DEF_BUF> log_xx("ABORT, ("); \
	log_xx << __FUNCTION__ << ") " << log_data << " " << _LOG_NAMESAPCE_::get_file_name(__FILE__) << ":" << __LINE__; \
	FILE* fp = fopen("abort.log", "ab"); \
	if (fp) \
	{ \
		fprintf(fp, "%s\r\n", log_xx.c_str()); \
		fclose(fp); \
	} \
	fflush(stdout); \
	fprintf(stderr, "%s\r\n", log_xx.c_str()); \
	fflush(stderr); \
	log_xx.clear(); \
	} \

#endif
